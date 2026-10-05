#!/usr/bin/python3
"""Small optional desktop HUD for the loopback HaulSense telemetry service."""
import argparse
import json
import os
import math
import threading
from pathlib import Path
from urllib.request import urlopen

# On GNOME Wayland, Xwayland permits a normal movable/above window. Native
# Wayland fallback remains useful on another monitor, but cannot force above.
os.environ.setdefault("GDK_BACKEND", "x11,wayland")
import gi  # noqa: E402

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
gi.require_version("Pango", "1.0")
gi.require_version("GLib", "2.0")
from gi.repository import Gdk, GLib, Gtk, Pango  # noqa: E402

STATE_URL = "http://127.0.0.1:39056/api/state"
SETTINGS = Path(os.environ.get("XDG_CONFIG_HOME", str(Path.home() / ".config"))) / "haulsense" / "hud.json"
DEFAULTS = {"monitor": 0, "corner": "top-right", "units": "metric", "opacity": 0.70, "scale": 1.0}


def finite(value):
    return type(value) in (int, float) and math.isfinite(value)


def ordered_monitors(display):
    """Use physical left-to-right order; GDK's enumeration varies by session."""
    return sorted((display.get_monitor(i) for i in range(display.get_n_monitors())),
                  key=lambda monitor: (monitor.get_geometry().x, monitor.get_geometry().y))


def load_settings():
    try:
        source = json.loads(SETTINGS.read_text())
    except (OSError, ValueError):
        source = {}
    result = DEFAULTS.copy()
    for key in result:
        if key in source and type(source[key]) is type(result[key]):
            result[key] = source[key]
    result["monitor"] = max(0, min(15, result["monitor"]))
    result["opacity"] = max(0.35, min(0.95, result["opacity"]))
    result["scale"] = max(0.8, min(1.3, result["scale"]))
    if result["corner"] not in ("top-left", "top-right", "bottom-left", "bottom-right"):
        result["corner"] = "top-right"
    if result["units"] not in ("metric", "us"):
        result["units"] = "metric"
    position = source.get("position")
    if isinstance(position, list) and len(position) == 2 and all(type(value) is int for value in position):
        result["position"] = position
    return result


class Hud:
    def __init__(self, settings):
        self.settings = settings
        self.display = Gdk.Display.get_default()
        if self.display is None:
            raise RuntimeError("No graphical display is available")
        self.window = Gtk.Window(title="HaulSense HUD")
        self.window.set_app_paintable(True)
        visual = self.window.get_screen().get_rgba_visual()
        if visual:
            self.window.set_visual(visual)
        self.window.set_decorated(False)
        self.window.set_keep_above(True)
        self.window.set_type_hint(Gdk.WindowTypeHint.UTILITY)
        self.window.set_resizable(False)
        self.window.connect("destroy", Gtk.main_quit)
        self.window.add_events(Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK)
        self.window.connect("button-press-event", self.on_press)
        self.window.connect("button-release-event", self.on_release)

        self.box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=1)
        self.box.get_style_context().add_class("panel")
        self.window.add(self.box)
        self.top = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.brand = self.label("HAULSENSE", "brand")
        self.left = self.label("◀", "signal")
        self.right = self.label("▶", "signal")
        self.status = self.label("WAITING", "status")
        self.top.pack_start(self.left, False, False, 0)
        self.top.pack_start(self.brand, True, True, 0)
        self.top.pack_end(self.right, False, False, 0)
        self.box.pack_start(self.top, False, False, 0)

        self.middle = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=16)
        speed_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.speed = self.label("—", "speed")
        self.units = self.label("km/h", "units")
        speed_box.pack_start(self.speed, False, False, 0)
        speed_box.pack_start(self.units, False, False, 0)
        self.middle.pack_start(speed_box, True, True, 0)
        limit_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        limit_box.pack_start(self.label("LIMITE", "caption"), False, False, 0)
        self.limit = self.label("—", "limit")
        self.limit.set_size_request(64, 64)
        limit_box.pack_start(self.limit, False, False, 0)
        self.middle.pack_start(limit_box, True, False, 0)
        self.box.pack_start(self.middle, False, False, 0)

        self.route_label = self.label("NAVIGAZIONE", "caption")
        self.box.pack_start(self.route_label, False, False, 0)
        self.city = self.label("—", "city")
        self.city.set_max_width_chars(28)
        self.city.set_ellipsize(Pango.EllipsizeMode.END)
        self.box.pack_start(self.city, False, False, 0)
        self.trip = self.label("—", "trip")
        self.box.pack_start(self.trip, False, False, 0)
        self.info = self.label("—", "secondary")
        self.info.set_max_width_chars(42)
        self.info.set_ellipsize(Pango.EllipsizeMode.END)
        self.box.pack_start(self.info, False, False, 0)
        self.warning = self.label("", "warning")
        self.warning.set_no_show_all(True)
        self.warning.set_line_wrap(True)
        self.warning.set_max_width_chars(36)
        self.box.pack_start(self.warning, False, False, 0)
        self.truck = self.label("In attesa del camion", "secondary")
        self.truck.set_max_width_chars(32)
        self.truck.set_ellipsize(Pango.EllipsizeMode.END)
        self.box.pack_start(self.truck, False, False, 0)
        self.box.pack_start(self.status, False, False, 0)
        self.apply_style()
        self.window.show_all()
        GLib.idle_add(self.place)
        self.latest_state = {}
        self.stopping = threading.Event()
        self.window.connect("destroy", lambda *_: self.stopping.set())
        threading.Thread(target=self.fetch_loop, daemon=True).start()
        GLib.timeout_add(200, self.poll)

    @staticmethod
    def label(value, css_class):
        widget = Gtk.Label(label=value)
        widget.set_xalign(0.5)
        widget.set_yalign(0.5)
        widget.set_justify(Gtk.Justification.CENTER)
        widget.get_style_context().add_class(css_class)
        return widget

    def apply_style(self):
        scale = self.settings["scale"]
        opacity = self.settings["opacity"]
        self.box.set_size_request(int(340 * scale), -1)
        self.limit.set_size_request(int(64 * scale), int(64 * scale))
        css = f"""
        window {{background-color: transparent;}}
        .panel {{background-color: rgba(8, 17, 23, {opacity});border:1px solid rgba(224,240,230,.27);
                 border-radius:16px;padding:{int(13*scale)}px {int(17*scale)}px;}}
        .brand {{color:#e3f39a;font-size:{int(11*scale)}px;font-weight:bold;letter-spacing:2px;}}
        .signal {{color:#a8b5b8;font-size:{int(17*scale)}px;}}
        .signal.on {{color:#ffb77f;}}
        .status,.units,.secondary {{color:#adbdc1;font-size:{int(12*scale)}px;}}
        .speed {{color:#f1f7f7;font-size:{int(69*scale)}px;font-weight:300;}}
        .speed.over {{color:#ffb77f;}}
        .limit {{color:#f1f7f7;font-size:{int(25*scale)}px;font-weight:bold;
                 border:3px solid #ef705c;border-radius:99px;padding:0;}}
        .caption {{color:#adbdc1;font-size:{int(10*scale)}px;letter-spacing:1px;margin-top:8px;}}
        .trip {{color:#e3f39a;font-size:{int(14*scale)}px;margin:5px 0;}}
        .warning {{color:#ffb77f;font-size:{int(11*scale)}px;margin-top:5px;}}
        .limit.empty {{border-color:#53616b;color:#adbdc1;}}
        .city {{color:#f1f7f7;font-size:{int(18*scale)}px;font-weight:600;}}
        """
        provider = Gtk.CssProvider()
        provider.load_from_data(css.encode())
        if getattr(self, "provider", None):
            Gtk.StyleContext.remove_provider_for_screen(self.window.get_screen(), self.provider)
        Gtk.StyleContext.add_provider_for_screen(self.window.get_screen(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        self.provider = provider

    def save(self):
        SETTINGS.parent.mkdir(parents=True, exist_ok=True)
        temporary = SETTINGS.with_suffix(".tmp")
        temporary.write_text(json.dumps(self.settings, indent=2) + "\n")
        temporary.chmod(0o600)
        temporary.replace(SETTINGS)

    def place(self):
        monitors = ordered_monitors(self.display)
        count = len(monitors)
        index = min(self.settings["monitor"], max(count - 1, 0))
        monitor = monitors[index]
        area = monitor.get_workarea()
        width, height = self.window.get_size()
        margin = 18
        corner = self.settings["corner"]
        position = self.settings.get("position")
        x = position[0] if position else area.x + margin if corner.endswith("left") else area.x + area.width - width - margin
        y = position[1] if position else area.y + margin if corner.startswith("top") else area.y + area.height - height - margin
        self.window.move(max(area.x, min(x, area.x + area.width - width)),
                         max(area.y, min(y, area.y + area.height - height)))
        return False

    def menu(self, event):
        menu = Gtk.Menu()

        def section(title, values, field):
            heading = Gtk.MenuItem(label=title)
            heading.set_sensitive(False)
            menu.append(heading)
            for label, value in values:
                item = Gtk.MenuItem(label="✓ " + label if self.settings[field] == value else "   " + label)
                item.connect("activate", lambda _item, key=field, selected=value: self.change(key, selected))
                menu.append(item)

        monitors = []
        for index, monitor in enumerate(ordered_monitors(self.display)):
            area = monitor.get_geometry()
            monitors.append((f"Schermo {index + 1} · {monitor.get_model() or ''} · {area.width}×{area.height}", index))
        section("Schermo", monitors, "monitor")
        section("Angolo", [("Alto sinistra", "top-left"), ("Alto destra", "top-right"),
                           ("Basso sinistra", "bottom-left"), ("Basso destra", "bottom-right")], "corner")
        section("Unità", [("km/h", "metric"), ("mph", "us")], "units")
        section("Trasparenza", [("35%", 0.35), ("50%", 0.50), ("70%", 0.70), ("85%", 0.85)], "opacity")
        section("Dimensione", [("Piccolo", 0.8), ("Normale", 1.0), ("Grande", 1.2)], "scale")
        quit_item = Gtk.MenuItem(label="Chiudi HUD")
        quit_item.connect("activate", lambda _item: Gtk.main_quit())
        menu.append(quit_item)
        menu.show_all()
        menu.popup_at_pointer(event)

    def change(self, field, value):
        self.settings[field] = value
        if field in ("monitor", "corner"):
            self.settings.pop("position", None)
        self.save()
        if field in ("scale", "opacity"):
            self.apply_style()
        GLib.idle_add(self.place)

    def on_press(self, _window, event):
        if event.button == 3:
            self.menu(event)
            return True
        if event.button == 1:
            self.window.begin_move_drag(event.button, int(event.x_root), int(event.y_root), event.time)
            return True
        return False

    def on_release(self, _window, event):
        if event.button == 1:
            x, y = self.window.get_position()
            self.settings["position"] = [x, y]
            monitor = self.display.get_monitor_at_point(x, y)
            for index, candidate in enumerate(ordered_monitors(self.display)):
                if candidate == monitor:
                    self.settings["monitor"] = index
                    break
            self.save()

    def fetch_loop(self):
        # Network waits never block dragging, menus or GTK painting.
        while not self.stopping.is_set():
            try:
                with urlopen(STATE_URL, timeout=0.6) as response:
                    state = json.load(response)
                if not isinstance(state, dict) or not isinstance(state.get("telemetry", {}), dict):
                    state = {}
            except (OSError, ValueError):
                state = {}
            self.latest_state = state
            self.stopping.wait(0.2 if state.get("active") else 0.7)

    def poll(self):
        state = self.latest_state
        live = bool(state.get("active"))
        telemetry = state.get("telemetry", {}) if live else {}
        metric = self.settings["units"] == "metric"
        factor = 3.6 if metric else 2.236936
        speed = telemetry.get("speed_mps")
        speed = round(abs(speed) * factor) if finite(speed) else None
        limit = telemetry.get("nav_speed_limit")
        limit = round(limit * factor) if finite(limit) and limit > 0 else None
        self.speed.set_text(str(speed) if speed is not None else "—")
        self.units.set_text("km/h" if metric else "mph")
        self.limit.set_text(str(limit) if limit is not None else "—")
        context = self.speed.get_style_context()
        (context.add_class if speed is not None and limit is not None and speed > limit + 2 else context.remove_class)("over")
        self.status.set_text(("DEMO" if state.get("demo") else "LIVE") if live else "PAUSA" if state.get("paused") else "ATTESA")
        destination = telemetry.get("destination") or ""
        distance = telemetry.get("nav_distance")
        seconds = telemetry.get("nav_time")
        routed = bool(destination) or (finite(distance) and distance > 0) or (finite(seconds) and seconds > 0)
        self.route_label.set_text("DESTINAZIONE INCARICO" if destination else "NAVIGAZIONE")
        self.city.set_text(destination or ("Percorso GPS" if routed else "Nessun percorso" if live else "—"))
        self.city.set_tooltip_text(destination or "Il SDK fornisce il nome della città soltanto per un incarico.")
        self.truck.set_text((str(telemetry.get("truck_brand") or "") + " " + str(telemetry.get("truck_name") or "")).strip() or "In attesa del camion")
        parts = []
        if routed and finite(distance):
            parts.append(f"{distance / (1000 if metric else 1609.344):.1f} {'km' if metric else 'mi'}")
        if routed and finite(seconds):
            minutes = round(seconds / 60)
            parts.append(f"{minutes // 60} h {minutes % 60:02d} min" if minutes >= 60 else f"{minutes} min")
        self.trip.set_text(" · ".join(parts) or "—")
        gear = telemetry.get("displayed_gear")
        gear_text = "—" if not finite(gear) else "N" if gear == 0 else "R" + str(abs(int(gear))) if gear < 0 else str(int(gear))
        info = ["Marcia " + gear_text]
        fuel_range = telemetry.get("fuel_range")
        if finite(fuel_range):
            info.append(f"Autonomia {round(fuel_range / (1 if metric else 1.609344))} {'km' if metric else 'mi'}")
        cruise = telemetry.get("cruise_speed")
        if finite(cruise) and cruise > 0:
            info.append(f"Cruise {round(cruise * factor)}")
        self.info.set_text(" · ".join(info) if live else "—")
        self.info.set_tooltip_text(self.info.get_text())
        warnings = [("air_emergency", "PRESSIONE ARIA"), ("fuel_warning", "RISERVA"),
                    ("water_warning", "TEMPERATURA"), ("oil_warning", "OLIO"),
                    ("battery_warning", "BATTERIA"), ("parking_brake", "FRENO A MANO")]
        message = " · ".join(label for key, label in warnings if telemetry.get(key))
        self.warning.set_text(message)
        self.warning.set_visible(bool(message))
        context = self.limit.get_style_context()
        (context.add_class if limit is None else context.remove_class)("empty")
        for widget, enabled in ((self.left, telemetry.get("left_blinker_light") if telemetry.get("left_blinker_light") is not None else telemetry.get("left_blinker") or telemetry.get("hazards")), (self.right, telemetry.get("right_blinker_light") if telemetry.get("right_blinker_light") is not None else telemetry.get("right_blinker") or telemetry.get("hazards"))):
            context = widget.get_style_context()
            (context.add_class if enabled else context.remove_class)("on")
        return True


def main():
    parser = argparse.ArgumentParser(description="Transparent HaulSense telemetry HUD")
    parser.add_argument("--list-monitors", action="store_true", help="show numbered monitors and exit")
    parser.add_argument("--monitor", type=int, help="zero-based monitor number for this launch")
    args = parser.parse_args()
    display = Gdk.Display.get_default()
    if display is None:
        parser.error("no graphical display is available")
    monitors = ordered_monitors(display)
    if args.list_monitors:
        for index, monitor in enumerate(monitors):
            area = monitor.get_geometry()
            print(f"{index}: {monitor.get_model() or ''} {area.width}×{area.height} at {area.x},{area.y}")
        return
    settings = load_settings()
    if args.monitor is not None:
        if args.monitor < 0 or args.monitor >= len(monitors):
            parser.error("monitor index does not exist")
        settings["monitor"] = args.monitor
    Hud(settings)
    Gtk.main()


if __name__ == "__main__":
    main()
