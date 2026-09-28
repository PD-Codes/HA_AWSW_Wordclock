#pragma once
// Single-page web UI served from flash.
static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WordClock</title>
<style>
:root{--bg:#0f1216;--card:#181c22;--fg:#e7eaee;--mut:#8b94a1;--acc:#3ddc84;--bd:#262c35;--in:#20262e;--off:#2a3038}
@media (prefers-color-scheme:light){:root{--bg:#f3f5f8;--card:#fff;--fg:#1b1f24;--mut:#5f6874;--acc:#1f9d55;--bd:#dde2e8;--in:#f1f3f6;--off:#c9ced6}}
*{box-sizing:border-box}body{margin:0;font:15px/1.4 system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--fg)}
header{position:sticky;top:0;z-index:2;background:var(--bg);border-bottom:1px solid var(--bd)}
.top{max-width:960px;margin:0 auto;padding:12px 16px 0;display:flex;align-items:baseline;justify-content:space-between;gap:8px}
h1{font-size:19px;margin:0}#hdr{color:var(--mut);font-size:13px}
nav{max-width:960px;margin:0 auto;padding:8px 16px;display:flex;gap:6px;overflow-x:auto}
nav button{border:0;background:none;color:var(--mut);padding:6px 12px;border-radius:16px;white-space:nowrap}
nav button.on{background:var(--in);color:var(--fg)}
main{max-width:960px;margin:0 auto;padding:16px;display:grid;gap:16px}
section{display:none;gap:16px}section.on{display:grid}
.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:16px}
h2{font-size:13px;margin:0 0 10px;color:var(--mut);text-transform:uppercase;letter-spacing:.06em}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:7px 0;min-height:40px}
.row+.row{border-top:1px solid var(--bd)}
input,select,button{font:inherit;color:var(--fg);background:var(--in);border:1px solid var(--bd);border-radius:8px;padding:6px 10px}
input[type=range]{flex:1;max-width:260px;padding:0;accent-color:var(--acc)}
input[type=color]{padding:0;width:44px;height:30px;border-radius:6px}
input[type=checkbox]{width:20px;height:20px;accent-color:var(--acc)}
button{cursor:pointer}button.pri{background:var(--acc);color:#06140b;border:0;font-weight:600}
.ctl{display:flex;align-items:center;gap:10px;flex:1;justify-content:flex-end}
.val{min-width:32px;text-align:right;color:var(--mut);font-variant-numeric:tabular-nums}
.plate{background:#07090b;border-radius:10px;padding:14px;max-width:560px;margin:0 auto;width:100%}
.grid{display:grid;grid-template-columns:repeat(16,1fr);gap:1px;font:700 clamp(11px,3vw,20px) ui-monospace,monospace;text-align:center}
.grid span{color:#262b31;padding:2px 0;transition:color .3s}
.words{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:8px}
.word{display:flex;align-items:center;gap:10px;background:var(--in);border:1px solid var(--bd);border-radius:10px;padding:8px 10px}
.word b{flex:1;font-weight:600;font-size:14px}.word small{color:var(--mut)}
.btns{display:flex;flex-wrap:wrap;gap:8px;margin-top:8px}
small,.mut{color:var(--mut)}code{font-size:12px;word-break:break-all}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%);background:var(--card);border:1px solid var(--bd);padding:10px 16px;border-radius:10px;opacity:0;transition:opacity .2s;pointer-events:none}
#toast.show{opacity:1}
</style></head><body>
<header><div class="top"><h1>WordClock</h1><span id="hdr">connecting...</span></div>
<nav id="nav"></nav></header>
<main>
<section id="t-clock">
 <div class="card"><div class="plate"><div class="grid" id="grid"></div></div>
  <div class="row" style="border:0"><span class="mut" id="ptime"></span><button id="stopBtn" class="pri" style="display:none" onclick="act('stop')">Back to clock</button></div></div>
 <div class="card"><h2>Quick settings</h2>
  <div class="row">Brightness (day)<div class="ctl"><input type="range" data-k="timeBrightnessDay" min="0" max="50"><span class="val" data-v="timeBrightnessDay"></span></div></div>
  <div class="row">Brightness (night)<div class="ctl"><input type="range" data-k="timeBrightnessNight" min="0" max="50"><span class="val" data-v="timeBrightnessNight"></span></div></div>
  <div class="row">Time color<input type="color" data-k="timeColor"></div>
  <div class="row">Night mode<input type="checkbox" data-k="nightMode"></div>
 </div>
</section>

<section id="t-words">
 <div class="card"><h2>Extra words</h2><div class="words" id="words"></div>
  <div class="btns"><button onclick="act('resetExtraWords')">All off</button><button onclick="act('wordReset')">Reset colors</button><button onclick="act('wordCycle')">Preview all words</button></div></div>
</section>

<section id="t-colors">
 <div class="card"><h2>Day</h2>
  <div class="row">Time<div class="ctl"><input type="color" data-k="timeColor"><input type="range" data-k="timeBrightnessDay" min="0" max="50"><span class="val" data-v="timeBrightnessDay"></span></div></div>
  <div class="row">Background<div class="ctl"><input type="color" data-k="backColor"><input type="range" data-k="backBrightnessDay" min="0" max="50"><span class="val" data-v="backBrightnessDay"></span></div></div></div>
 <div class="card"><h2>Night</h2>
  <div class="row">Time<div class="ctl"><input type="color" data-k="timeColorNight"><input type="range" data-k="timeBrightnessNight" min="0" max="50"><span class="val" data-v="timeBrightnessNight"></span></div></div>
  <div class="row">Background<div class="ctl"><input type="color" data-k="backColorNight"><input type="range" data-k="backBrightnessNight" min="0" max="50"><span class="val" data-v="backBrightnessNight"></span></div></div></div>
 <div class="card"><h2>Ticker &amp; power</h2>
  <div class="row">Ticker<div class="ctl"><input type="color" data-k="tickerColor"><input type="range" data-k="tickerBrightness" min="0" max="50"><span class="val" data-v="tickerBrightness"></span></div></div>
  <div class="row">Max LED power (%)<div class="ctl"><input type="range" data-k="maxPower" min="5" max="100"><span class="val" data-v="maxPower"></span></div></div>
  <p><small>Caps the overall LED current. 256 LEDs at full white can draw well over 10 A – raise only if your power supply can handle it.</small></p></div>
</section>

<section id="t-options">
 <div class="card"><h2>Display</h2>
  <div class="row">Show "ES IST"<input type="checkbox" data-k="showItIs"></div>
  <div class="row">Show single minutes (+1..4)<input type="checkbox" data-k="singleMinutes"></div>
  <div class="row">Smooth transitions<input type="checkbox" data-k="smoothTransition"></div>
  <div class="row">Random color every day<input type="checkbox" data-k="randomDayColors"></div>
  <div class="row">Digital time on the full hour<input type="checkbox" data-k="digitalHourChime"></div>
  <div class="row">Startup animation<input type="checkbox" data-k="startupAnimation"></div>
  <div class="row">Show IP after startup<input type="checkbox" data-k="showIp"></div></div>
 <div class="card"><h2>Night mode</h2>
  <div class="row">Enabled<input type="checkbox" data-k="nightMode"></div>
  <div class="row">Day starts<input type="time" data-k="dayStart"></div>
  <div class="row">Day ends<input type="time" data-k="dayStop"></div>
  <div class="row">Current<span class="mut" data-v="nightStatus"></span></div></div>
</section>

<section id="t-ticker">
 <div class="card"><h2>Scrolling text</h2>
  <div class="row"><input id="tkText" placeholder="Message" style="flex:1" maxlength="200"><input type="color" id="tkColor"><button class="pri" onclick="ticker()">Send</button></div>
  <div class="btns"><button onclick="act('digitalTimeTest')">Show digital time</button></div></div>
</section>

<section id="t-system">
 <div class="card"><h2>Info</h2><div id="info"></div></div>
 <div class="card"><h2>Network &amp; time</h2>
  <div class="row">Hostname<input data-k="hostname" data-lazy></div>
  <div class="row">Timezone (POSIX)<input data-k="timeZone" data-lazy></div>
  <div class="row">Time server<input data-k="timeServer" data-lazy></div></div>
 <div class="card"><h2>LED wiring</h2>
  <p><small>"Calibrate": the top-left letter must light <b style="color:#f33">red</b>, the rest of the top row <b style="color:#48f">blue</b> and the left column <b style="color:#3c6">green</b>.</small></p>
  <div class="row">Strip starts at<select data-k="origin"><option value="0">Top left</option><option value="1">Top right</option><option value="2">Bottom left</option><option value="3">Bottom right</option></select></div>
  <div class="row">Zig-zag (serpentine)<input type="checkbox" data-k="serpentine"></div>
  <div class="row">Runs in columns<input type="checkbox" data-k="vertical"></div>
  <div class="btns"><button onclick="act('calibrate')">Calibrate</button><button onclick="act('chase')">Chase (raw order)</button><button onclick="act('test')">LED test</button><button onclick="act('allOn')">All on</button><button class="pri" onclick="act('stop')">Back to clock</button></div></div>
 <div class="card"><h2>Firmware</h2>
  <div class="row"><input type="file" id="fw" accept=".bin"><button onclick="upload()">Upload</button></div>
  <div class="btns"><button onclick="act('restart')">Restart</button><button onclick="if(confirm('Forget WiFi? The clock restarts into WPS mode (3 min), then opens the WordClock-Setup hotspot.'))act('wifiReset')">Reset WiFi</button></div></div>
 <div class="card"><h2>Smart home API</h2><div id="api" class="mut"></div></div>
</section>
</main>
<div id="toast"></div>
<script>
const $=id=>document.getElementById(id),Q=s=>document.querySelectorAll(s);
const TABS=[['clock','Clock'],['words','Words'],['colors','Colors'],['options','Options'],['ticker','Ticker'],['system','System']];
let st={},layout=[],timers={},wordsBuilt=false;
const LAYOUT=["ALARMGEBURTSTAGW","MÜLLAUTOFEIERTAG","AFORMEL1DOWNLOAD","WLANUPDATERAUSES","BRINGENISTGELBER","SACKZEITZWANZIGF","HALBGURLAUBGENAU","ZEHNWERKSTATTZUM","FÜNFRISEURZOCKEN","WORDCLOCKVIERTEL","VORNEUSTARTERMIN","NACHLHALBVSIEBEN","SECHSNEUNZEHNELF","EINSDREIVIERZWEI","ACHTZWÖLFÜNFUUHR","S+1234OKMINUTENW"];
function toast(t){const e=$('toast');e.textContent=t;e.classList.add('show');clearTimeout(timers.t);timers.t=setTimeout(()=>e.classList.remove('show'),1800)}
function api(p){return fetch(p).then(r=>r.json())}
function set(k,v){clearTimeout(timers[k]);timers[k]=setTimeout(()=>api('/api/set?'+k+'='+encodeURIComponent(v)).then(r=>{if(!r.ok)toast(r.message);loadStatus()}),200)}
function act(c){api('/api/action?cmd='+c).then(r=>toast(r.message)).then(()=>setTimeout(loadStatus,300))}
function ticker(){const t=$('tkText').value.trim();if(!t)return;api('/api/ticker?text='+encodeURIComponent(t)+'&color='+encodeURIComponent($('tkColor').value)).then(r=>toast(r.message))}
function upload(){const f=$('fw').files[0];if(!f)return;const d=new FormData();d.append('update',f);toast('Uploading...');
 fetch('/update',{method:'POST',body:d}).then(r=>r.json()).then(r=>toast(r.message)).catch(()=>toast('Upload failed'))}
function tab(id){TABS.forEach(([t])=>{$('t-'+t).classList.toggle('on',t==id);$('b-'+t).classList.toggle('on',t==id)});try{localStorage.setItem('wcTab',id)}catch(e){}}
function buildNav(){$('nav').innerHTML=TABS.map(([id,l])=>`<button id="b-${id}" onclick="tab('${id}')">${l}</button>`).join('');let t='clock';try{t=localStorage.getItem('wcTab')||t}catch(e){}tab(t)}
function buildGrid(){$('grid').innerHTML=LAYOUT.map(r=>[...r].map(c=>`<span>${c}</span>`).join('')).join('')}
function bindInputs(){Q('[data-k]').forEach(e=>{const k=e.dataset.k;
 const ev=e.type=='range'?'input':'change';
 e.addEventListener(ev,()=>{let v=e.type=='checkbox'?(e.checked?1:0):e.value;
  if(e.type=='range')Q(`[data-v="${k}"]`).forEach(x=>x.textContent=v);
  Q(`[data-k="${k}"]`).forEach(x=>{if(x!==e){if(x.type=='checkbox')x.checked=e.checked;else x.value=e.value}});
  set(k,v)})})}
function fill(){Q('[data-k]').forEach(e=>{const k=e.dataset.k,v=st[k];if(v===undefined||document.activeElement===e)return;
  if(e.type=='checkbox')e.checked=!!v;else e.value=v});
 Q('[data-v]').forEach(e=>{const v=st[e.dataset.v];if(v!==undefined)e.textContent=v})}
function buildWords(){const w=st.extraWords||[];
 $('words').innerHTML=w.map(x=>{const n=x.name.replace(/^\d+:\s*/,'');return `<label class="word"><input type="checkbox" data-w="${x.id}"><b>${n}</b><small>#${x.id}</small><input type="color" data-wc="${x.id}"></label>`}).join('');
 Q('[data-w]').forEach(e=>e.onchange=()=>set('ew'+e.dataset.w,e.checked?1:0));
 Q('[data-wc]').forEach(e=>e.onchange=()=>{set('ewColor'+e.dataset.wc,e.value);const c=$q(`[data-w="${e.dataset.wc}"]`);if(!c.checked){c.checked=true;set('ew'+e.dataset.wc,1)}});
 wordsBuilt=true}
const $q=s=>document.querySelector(s);
function fillWords(){(st.extraWords||[]).forEach(x=>{const c=$q(`[data-w="${x.id}"]`),k=$q(`[data-wc="${x.id}"]`);if(c)c.checked=x.active;if(k&&document.activeElement!==k)k.value=x.color.toLowerCase()})}
function info(){const i=[['Firmware',st.version],['IP',st.ip],['Hostname',st.hostname+'.local'],['WiFi',st.ssid+' ('+st.rssi+' dBm)'],['MAC',st.mac],['Time',st.ntpStatus],['Uptime',Math.floor(st.uptime/3600)+' h '+Math.floor(st.uptime%3600/60)+' min']];
 $('info').innerHTML=i.map(([a,b])=>`<div class="row">${a}<span class="mut">${b}</span></div>`).join('');
 const b='http://'+st.ip;$('api').innerHTML=[['Status','/api/status'],['Word on/off/toggle','/api/set?ew13=1'],['Word color','/api/set?ewColor13=%2300FF00'],['Setting','/api/set?timeBrightnessDay=30'],['Ticker','/api/ticker?text=Hello'],['Action','/api/action?cmd=test']].map(([a,p])=>`<div class="row">${a}<code>${b}${p}</code></div>`).join('')}
function loadStatus(){return api('/api/status').then(s=>{st=s;$('hdr').textContent=s.version+' · '+s.ip;
 if(!wordsBuilt)buildWords();fill();fillWords();info()}).catch(()=>$('hdr').textContent='offline')}
function preview(){api('/api/preview').then(p=>{const cells=$('grid').children;
 for(let i=0;i<256;i++){const h=p.px.substr(i*6,6),n=parseInt(h,16),e=cells[i];if(!e)continue;
  if(!n){e.style.color='';e.style.textShadow='';continue}
  let r=n>>16,g=n>>8&255,b=n&255,m=Math.max(r,g,b);r=r*255/m|0;g=g*255/m|0;b=b*255/m|0;const c=`rgb(${r},${g},${b})`;
  e.style.color=c;e.style.textShadow='0 0 8px '+c}
 $('ptime').textContent=p.time;$('stopBtn').style.display=p.mode&&p.mode!=1?'':'none'}).catch(()=>{})}
buildNav();buildGrid();bindInputs();loadStatus();preview();
setInterval(preview,1500);setInterval(loadStatus,10000);
</script></body></html>)rawliteral";
