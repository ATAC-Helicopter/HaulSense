#include "job_journal.hpp"
#include "../../third_party/json/json.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>
using Json=nlohmann::json;
namespace fs=std::filesystem;
namespace {
long long epoch_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::string identity(const AtsTelemetryPacket&t){return std::to_string(t.game)+"|"+t.origin+"|"+t.destination+"|"+t.cargo+"|"+std::to_string(static_cast<double>(t.cargo_mass));}
bool id_ok(const std::string& id){return !id.empty()&&id.size()<=24&&id.find_first_not_of("0123456789")==std::string::npos;}
Json read_bounded(const fs::path& path){if(!fs::exists(path)||fs::is_symlink(path)||fs::file_size(path)>1024*1024)return nullptr;std::ifstream f(path);auto j=Json::parse(f,nullptr,false);return j.is_discarded()?Json(nullptr):j;}
void atomic_write(const fs::path& path,const Json& value){auto tmp=path;tmp+=".tmp";if(fs::is_symlink(tmp))throw std::runtime_error("Journal temporary file is a symlink");{std::ofstream f(tmp,std::ios::trunc);f<<value.dump();f.flush();if(!f)throw std::runtime_error("Journal write failed");}chmod(tmp.c_str(),0600);fs::rename(tmp,path);}
}
struct JobJournal::Impl {
    struct Sample {AtsTelemetryPacket t;unsigned protocol;long long at;};
    fs::path directory;
    mutable std::mutex mutex;std::condition_variable wake;
    std::deque<Sample> queue;bool stop=false;std::atomic<unsigned> dropped{0};
    std::string list_cache="{\"reports\":[],\"recording\":null,\"error\":null}";
    std::unordered_map<std::string,std::string> reports;
    Json active=nullptr,history=Json::array();
    bool dirty=false;unsigned event_seq=0;unsigned long long frame_seq=0;
    long long last_at=0,last_write=0;std::string error,finished_identity;unsigned finished_job_sequence=0;
    double speed_integral=0,previous_fuel=-1,previous_odo=-1;bool route_gap=true;
    std::thread worker;
    explicit Impl(std::string path):directory(std::move(path)),worker([this]{run();}){}
    ~Impl(){{std::lock_guard lock(mutex);stop=true;}wake.notify_all();worker.join();}
    void publish(){
        Json recording=active.is_object()?Json{{"id",active["id"]},{"cargo",active["cargo"]},{"origin",active["origin"]},{"destination",active["destination"]},{"distance_km",active["distance_km"]},{"observed_seconds",active["observed_seconds"]}}:Json(nullptr);
        std::lock_guard lock(mutex);list_cache=Json{{"reports",history},{"recording",recording},{"error",error.empty()?Json(nullptr):Json(error)},{"dropped_frames",dropped.load()}}.dump();
    }
    void finish(const std::string& outcome,const Sample& sample){
        if(!active.is_object())return;
        active["outcome"]=outcome;active["finished_at_ms"]=sample.at;active["dropped_frames"]=dropped.load();
        active["average_speed_kmh"]=active["observed_seconds"].get<double>()>0?speed_integral/active["observed_seconds"].get<double>():0;
        const auto&t=sample.t;
        Json official=nullptr;
        if(outcome=="delivered"||outcome=="cancelled"){
            official=Json::object();
            if(t.event_attributes&EVENT_MONEY)official[outcome=="delivered"?"revenue":"cancel_penalty"]=static_cast<long long>(t.event_money);
            if(t.event_attributes&EVENT_XP)official["earned_xp"]=static_cast<int>(t.event_xp);
            if(t.event_attributes&EVENT_DISTANCE)official["distance_km"]=static_cast<double>(t.event_distance);
            if(t.event_attributes&EVENT_CARGO_DAMAGE)official["cargo_damage"]=static_cast<double>(t.event_cargo_damage);
            if(t.event_attributes&EVENT_GAME_MINUTES)official["game_minutes"]=static_cast<unsigned>(t.event_game_minutes);
            if(t.event_attributes&EVENT_AUTOPARK)official["autopark"]=bool(t.event_autopark);
            if(t.event_attributes&EVENT_AUTOLOAD)official["autoload"]=bool(t.event_autoload);
        }
        active["official"]=official;finished_identity=active["identity"].get<std::string>();finished_job_sequence=sample.t.job_sequence;
        std::string id=active["id"].get<std::string>();atomic_write(directory/(id+".json"),active);
        Json summary={{"id",id},{"outcome",outcome},{"origin",active["origin"]},{"destination",active["destination"]},{"cargo",active["cargo"]},{"finished_at_ms",sample.at},{"distance_km",active["distance_km"]}};
        history.insert(history.begin(),summary);{std::lock_guard lock(mutex);reports[id]=active.dump();}
        while(history.size()>50){std::string old=history.back()["id"];history.erase(history.size()-1);fs::remove(directory/(old+".json"));std::lock_guard lock(mutex);reports.erase(old);}
        atomic_write(directory/"index.json",history);fs::remove(directory/"active.json");active=nullptr;dirty=false;route_gap=true;previous_fuel=previous_odo=-1;last_at=0;publish();
    }
    void process(const Sample& s){
        const auto&t=s.t;
        bool job=s.protocol>=7?t.job_active:bool(t.cargo[0]);
        if(!job)finished_identity.clear();
        if(job&&!active.is_object()&&identity(t)==finished_identity&&t.job_sequence==finished_job_sequence){frame_seq=t.sequence;event_seq=t.event_sequence;return;}
        bool event=t.event_sequence!=event_seq;
        if(frame_seq&&t.sequence<frame_seq){event=false;if(active.is_object())active["gaps"]=active["gaps"].get<unsigned>()+1;route_gap=true;}
        frame_seq=t.sequence;event_seq=t.event_sequence;
        // Configuration can clear immediately after delivery; consume each received frame, not UI polls.
        if(active.is_object()&&identity(t)!=active["identity"].get<std::string>()&&job){finish("interrupted",s);}
        if(!active.is_object()&&job&&!t.paused){
            active={{"schema",1},{"id",std::to_string(s.at)},{"identity",identity(t)},{"game",t.game==1?"ats":t.game==2?"ets2":"unknown"},{"cargo",t.cargo},{"cargo_mass_kg",static_cast<double>(t.cargo_mass)},{"truck",std::string(t.truck_brand)+" "+t.truck_name},{"origin",t.origin},{"destination",t.destination},{"started_at_ms",s.at},{"observed_seconds",0.0},{"moving_seconds",0.0},{"idle_seconds",0.0},{"overspeed_seconds",0.0},{"distance_km",0.0},{"fuel_used_l",0.0},{"refuelled_l",0.0},{"peak_speed_kmh",0.0},{"cargo_damage_start",channel_available(t,CHANNEL_cargo_damage)?Json(static_cast<double>(t.cargo_damage)):Json(nullptr)},{"cargo_damage_end",nullptr},{"wear_start",static_cast<double>(t.max_wear)},{"wear_end",static_cast<double>(t.max_wear)},{"fines",Json::array()},{"tolls",Json::array()},{"transport_costs",Json::array()},{"route",Json::array()},{"gaps",0},{"route_samples_dropped",0},{"distance_available",channel_available(t,CHANNEL_odometer)},{"fuel_available",channel_available(t,CHANNEL_fuel)},{"speed_available",channel_available(t,CHANNEL_speed_mps)},{"speed_limit_available",channel_available(t,CHANNEL_nav_speed_limit)},{"coverage","Observed segment only; recording may begin after job acceptance."}};
            speed_integral=0;previous_fuel=previous_odo=-1;last_at=0;route_gap=true;dirty=true;
        }
        if(!active.is_object())return;
        auto& route=active["route"];
        if(t.paused){route_gap=true;last_at=0;previous_fuel=previous_odo=-1;}
        else {
            double dt=last_at?double(s.at-last_at)/1000:0;
            if(dt<0||dt>2){dt=0;route_gap=true;previous_fuel=previous_odo=-1;active["gaps"]=active["gaps"].get<unsigned>()+1;}
            last_at=s.at;double kmh=std::abs(t.speed_mps)*3.6;
            active["observed_seconds"]=active["observed_seconds"].get<double>()+dt;speed_integral+=kmh*dt;
            const char* timer=kmh>3.6?"moving_seconds":"idle_seconds";active[timer]=active[timer].get<double>()+dt;
            active["peak_speed_kmh"]=std::max(active["peak_speed_kmh"].get<double>(),kmh);
            if(channel_available(t,CHANNEL_nav_speed_limit)&&t.nav_speed_limit>0&&std::abs(t.speed_mps)>t.nav_speed_limit+1)active["overspeed_seconds"]=active["overspeed_seconds"].get<double>()+dt;
            if(channel_available(t,CHANNEL_odometer)){if(previous_odo>=0&&t.odometer>=previous_odo&&t.odometer-previous_odo<5)active["distance_km"]=active["distance_km"].get<double>()+t.odometer-previous_odo;previous_odo=t.odometer;}
            if(channel_available(t,CHANNEL_fuel)){if(previous_fuel>=0){double delta=t.fuel-previous_fuel;const char* key=delta>0?"refuelled_l":"fuel_used_l";active[key]=active[key].get<double>()+std::abs(delta);}previous_fuel=t.fuel;}
            if(channel_available(t,CHANNEL_cargo_damage))active["cargo_damage_end"]=static_cast<double>(t.cargo_damage);
            active["wear_end"]=static_cast<double>(t.max_wear);
            if(t.placement_available){
                bool append=route.empty()||route.back().is_null();
                if(!append){double distance=std::hypot(static_cast<double>(t.world_x)-route.back()[0].get<double>(),static_cast<double>(t.world_z)-route.back()[1].get<double>());if(distance>500){route_gap=true;active["gaps"]=active["gaps"].get<unsigned>()+1;}append=distance>=5;}
                if(route_gap&&!route.empty()&&!route.back().is_null())route.push_back(nullptr);
                if(append||route_gap){route.push_back({static_cast<double>(t.world_x),static_cast<double>(t.world_z),static_cast<double>(t.world_y),s.at});route_gap=false;}
                // Downsample entire history rather than dropping the start of the travelled route.
                if(route.size()>10000){Json reduced=Json::array();for(size_t i=0;i<route.size();i++)if(i%2==0||route[i].is_null())reduced.push_back(route[i]);if(reduced.back()!=route.back())reduced.push_back(route.back());active["route_samples_dropped"]=active["route_samples_dropped"].get<unsigned>()+route.size()-reduced.size();route=std::move(reduced);}
            }else route_gap=true;
            dirty=true;
        }
        if(event){
            std::string name=t.last_event;
            if(name=="job.delivered"||name=="job.cancelled"){finish(name=="job.delivered"?"delivered":"cancelled",s);return;}
            const char* key=name=="player.fined"?"fines":name=="player.tollgate.paid"?"tolls":name=="player.use.ferry"||name=="player.use.train"?"transport_costs":nullptr;
            if(key&&active[key].size()<500)active[key].push_back({{"at_ms",s.at},{"event",name},{"amount",t.event_attributes&EVENT_MONEY?Json(static_cast<long long>(t.event_money)):Json(nullptr)}});
        }
        if(!job){finish("interrupted",s);return;}
    }
    void run(){
        try{
            if(fs::is_symlink(directory))throw std::runtime_error("Journal directory is a symlink");
            fs::create_directories(directory);chmod(directory.c_str(),0700);
            Json index=read_bounded(directory/"index.json");
            if(index.is_array()&&index.size()<=50)for(const auto& entry:index){if(!entry.is_object()||!entry.contains("id")||!entry["id"].is_string())continue;std::string id=entry["id"];if(!id_ok(id))continue;Json report=read_bounded(directory/(id+".json"));if(report.is_object()&&report.value("schema",0)==1){history.push_back(entry);reports[id]=report.dump();}}
            Json saved=read_bounded(directory/"active.json");
            if(saved.is_object()&&saved.value("schema",0)==1&&saved.contains("route")&&saved["route"].is_array()&&saved["route"].size()<=10000&&saved.contains("identity")){
                active=saved;active["gaps"]=active.value("gaps",0u)+1;speed_integral=active.value("speed_integral",0.0);route_gap=true;
            }
            publish();
            for(;;){
                Sample sample{};bool got=false,ending=false;
                {std::unique_lock lock(mutex);wake.wait_for(lock,std::chrono::milliseconds(250),[this]{return stop||!queue.empty();});ending=stop&&queue.empty();if(!queue.empty()){sample=queue.front();queue.pop_front();got=true;}}
                if(got)process(sample);
                if(dirty&&(ending||epoch_ms()-last_write>=5000)){active["speed_integral"]=speed_integral;atomic_write(directory/"active.json",active);last_write=epoch_ms();dirty=false;publish();}
                if(ending)break;
            }
        }catch(const std::exception& e){error=std::string("Job journal unavailable: ")+e.what();publish();}
    }
};
JobJournal::JobJournal(std::string directory):impl_(std::make_unique<Impl>(std::move(directory))){}
JobJournal::~JobJournal()=default;
void JobJournal::observe(const AtsTelemetryPacket&t,unsigned protocol){std::lock_guard lock(impl_->mutex);if(impl_->queue.size()>=256){++impl_->dropped;return;}impl_->queue.push_back({t,protocol,epoch_ms()});impl_->wake.notify_one();}
std::string JobJournal::list()const{std::lock_guard lock(impl_->mutex);return impl_->list_cache;}
std::string JobJournal::report(const std::string&id)const{if(!id_ok(id))return "null";std::lock_guard lock(impl_->mutex);auto found=impl_->reports.find(id);return found==impl_->reports.end()?"null":found->second;}
