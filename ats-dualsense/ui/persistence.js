'use strict';
window.HaulSenseStorage={
 async database(){if(!window.indexedDB)throw Error('Local map storage is unavailable');return new Promise((resolve,reject)=>{const request=window.indexedDB.open('haulsense-local',1);request.onupgradeneeded=()=>request.result.createObjectStore('maps');request.onsuccess=()=>resolve(request.result);request.onerror=()=>reject(request.error);});},
 async readMap(){const db=await this.database();try{return await new Promise((resolve,reject)=>{const request=db.transaction('maps').objectStore('maps').get('active');request.onsuccess=()=>resolve(request.result||null);request.onerror=()=>reject(request.error);});}finally{db.close();}},
 async saveMap(blob,name){const db=await this.database();try{await new Promise((resolve,reject)=>{const transaction=db.transaction('maps','readwrite');transaction.objectStore('maps').put({blob,name,savedAt:Date.now()},'active');transaction.oncomplete=resolve;transaction.onerror=()=>reject(transaction.error);transaction.onabort=()=>reject(transaction.error);});}finally{db.close();}},
 async forgetMap(){const db=await this.database();try{await new Promise((resolve,reject)=>{const transaction=db.transaction('maps','readwrite');transaction.objectStore('maps').delete('active');transaction.oncomplete=resolve;transaction.onerror=()=>reject(transaction.error);});}finally{db.close();}},
 settings(){try{return JSON.parse(window.localStorage.getItem('haulsense-preferences'))||{};}catch{return {};}},
 remember(values){try{window.localStorage.setItem('haulsense-preferences',JSON.stringify({...this.settings(),...values}));}catch{}}
};
