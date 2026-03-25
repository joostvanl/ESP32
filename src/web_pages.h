#pragma once

// Dashboard pagina – wordt inline meegecompileerd om SD-lees overhead te vermijden
// Minimale HTML/CSS zonder externe dependencies

static const char HTML_DASHBOARD[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NestboxCam</title>
<style>
  :root{--bg:#0f1117;--card:#1a1d27;--accent:#4ade80;--accent2:#22d3ee;--text:#e2e8f0;--muted:#64748b;--danger:#f87171;--border:#2d3148}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh}
  header{background:var(--card);border-bottom:1px solid var(--border);padding:1rem 1.5rem;display:flex;align-items:center;gap:1rem}
  .logo{font-size:1.4rem;font-weight:700;color:var(--accent)}
  .logo span{color:var(--accent2)}
  nav a{color:var(--muted);text-decoration:none;padding:.4rem .8rem;border-radius:.4rem;font-size:.9rem;transition:all .2s}
  nav a:hover,nav a.active{color:var(--text);background:rgba(255,255,255,.06)}
  nav{display:flex;gap:.25rem;margin-left:auto}
  main{padding:1.5rem;max-width:960px;margin:0 auto}
  h2{font-size:1.1rem;color:var(--muted);margin-bottom:1.2rem;text-transform:uppercase;letter-spacing:.05em;font-weight:600}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:1rem;margin-bottom:2rem}
  .card{background:var(--card);border:1px solid var(--border);border-radius:.75rem;padding:1.25rem}
  .card-label{font-size:.75rem;color:var(--muted);text-transform:uppercase;letter-spacing:.05em;margin-bottom:.4rem}
  .card-value{font-size:1.8rem;font-weight:700;color:var(--accent)}
  .card-value.blue{color:var(--accent2)}
  .card-value.warn{color:#fbbf24}
  .status-dot{display:inline-block;width:.6rem;height:.6rem;border-radius:50%;margin-right:.4rem}
  .dot-green{background:var(--accent);box-shadow:0 0 6px var(--accent)}
  .dot-red{background:var(--danger)}
  .dot-yellow{background:#fbbf24}
  .section{margin-bottom:2rem}
  .btn{display:inline-flex;align-items:center;gap:.4rem;padding:.5rem 1rem;border-radius:.5rem;border:none;cursor:pointer;font-size:.85rem;font-weight:600;text-decoration:none;transition:all .2s}
  .btn-primary{background:var(--accent);color:#0f1117}
  .btn-primary:hover{background:#22c55e}
  .btn-secondary{background:var(--card);color:var(--text);border:1px solid var(--border)}
  .btn-secondary:hover{background:rgba(255,255,255,.06)}
  .btn-danger{background:rgba(248,113,113,.15);color:var(--danger);border:1px solid rgba(248,113,113,.3)}
  .btn-danger:hover{background:rgba(248,113,113,.25)}
  .storage-bar{height:8px;background:var(--border);border-radius:4px;overflow:hidden;margin:.6rem 0}
  .storage-fill{height:100%;background:linear-gradient(90deg,var(--accent),var(--accent2));border-radius:4px;transition:width .6s ease}
  .storage-fill.warn{background:linear-gradient(90deg,#fbbf24,#f97316)}
  .storage-fill.danger{background:var(--danger)}
  .recording-badge{display:inline-flex;align-items:center;gap:.4rem;background:rgba(248,113,113,.15);color:var(--danger);border:1px solid rgba(248,113,113,.3);padding:.3rem .7rem;border-radius:2rem;font-size:.8rem;font-weight:600;animation:pulse 1.5s ease-in-out infinite}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
  .idle-badge{display:inline-flex;align-items:center;gap:.4rem;background:rgba(74,222,128,.1);color:var(--accent);border:1px solid rgba(74,222,128,.25);padding:.3rem .7rem;border-radius:2rem;font-size:.8rem;font-weight:600}
  .actions{display:flex;gap:.75rem;flex-wrap:wrap}
  footer{text-align:center;color:var(--muted);font-size:.75rem;padding:2rem;border-top:1px solid var(--border)}
  .thumb-card{background:var(--card);border:1px solid var(--border);border-radius:.75rem;overflow:hidden;max-width:240px}
  .thumb-card .card-label{padding:.6rem 1rem .3rem;margin:0}
  .thumb-wrap{position:relative;width:100%;background:#000;aspect-ratio:4/3;overflow:hidden}
  .thumb-wrap img{width:100%;height:100%;object-fit:cover;display:block;opacity:.85;transition:opacity .3s}
  .thumb-wrap img:hover{opacity:1}
  .thumb-overlay{position:absolute;bottom:0;left:0;right:0;padding:.3rem .5rem;background:linear-gradient(transparent,rgba(0,0,0,.7));font-size:.65rem;color:rgba(255,255,255,.8);font-family:monospace;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .thumb-card > .thumb-empty{display:flex;align-items:center;justify-content:center;width:100%;aspect-ratio:4/3;background:#000;color:var(--muted);font-size:.8rem}
  .thumb-actions{padding:.4rem .75rem .6rem;display:flex;gap:.4rem}
  .dl-toast{position:fixed;bottom:1.5rem;left:50%;transform:translateX(-50%) translateY(150%);opacity:0;transition:transform .35s ease,opacity .35s ease;z-index:9999;max-width:min(22rem,calc(100% - 2rem));padding:.85rem 1.25rem;background:var(--card);border:1px solid var(--accent2);border-radius:.5rem;box-shadow:0 12px 40px rgba(0,0,0,.45);font-size:.88rem;text-align:center;color:var(--text);pointer-events:none}
  .dl-toast.show{transform:translateX(-50%) translateY(0);opacity:1}
</style>
</head>
<body>
<div id="dlToast" class="dl-toast" role="status" aria-live="polite"></div>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a href="/" class="active">Dashboard</a>
    <a href="/live">Live</a>
    <a href="/videos">Galerij</a>
  </nav>
</header>
<main>
  <h2>Systeemstatus</h2>
  <div class="grid">
    <div class="card">
      <div class="card-label">Camera</div>
      <div class="card-value" id="camStatus">—</div>
    </div>
    <div class="card">
      <div class="card-label">Status</div>
      <div id="recStatus" class="idle-badge"><span class="status-dot dot-green"></span>Inactief</div>
    </div>
    <div class="card">
      <div class="card-label">Vrije opslag</div>
      <div class="card-value blue" id="freeSpace">—</div>
      <div class="storage-bar"><div class="storage-fill" id="storageFill" style="width:0%"></div></div>
      <div style="font-size:.75rem;color:var(--muted)" id="storageDetail">— / —</div>
    </div>
    <div class="card">
      <div class="card-label">Bestanden op SD</div>
      <div class="card-value blue" id="videoCount">—</div>
    </div>
  </div>

  <h2>Camera &amp; opname</h2>
  <div class="card" style="margin-bottom:1.5rem">
    <div class="card-label">Resolutie</div>
    <div class="actions" style="margin-top:.5rem">
      <button type="button" class="btn btn-secondary" id="btnVga" onclick="setRes('vga')">VGA 640×480</button>
      <button type="button" class="btn btn-secondary" id="btnSvga" onclick="setRes('svga')">SVGA 800×600</button>
      <button type="button" class="btn btn-secondary" id="btnXga" onclick="setRes('xga')">XGA 1024×768</button>
    </div>
    <div class="card-label" style="margin-top:1rem">Opnameduur (PIR)</div>
    <div class="actions" style="margin-top:.5rem">
      <button type="button" class="btn btn-secondary" id="btnD10" onclick="setDur(10)">10 s</button>
      <button type="button" class="btn btn-secondary" id="btnD20" onclick="setDur(20)">20 s</button>
      <button type="button" class="btn btn-secondary" id="btnD30" onclick="setDur(30)">30 s</button>
      <button type="button" class="btn btn-secondary" id="btnD60" onclick="setDur(60)">60 s</button>
    </div>
    <div class="actions" style="margin-top:1rem">
      <button type="button" class="btn btn-primary" onclick="doCapture()">&#128247; Foto maken</button>
    </div>
    <p style="font-size:.75rem;color:var(--muted);margin-top:.75rem">Resolutie wijzigen gaat niet tijdens live stream of opname.</p>
  </div>

  <h2>Laatste bestand</h2>
  <div class="thumb-card" id="thumbCard">
    <div class="card-label">Meest recente video of foto</div>
    <div class="thumb-empty" id="thumbEmpty">Nog geen bestanden opgeslagen</div>
    <div class="thumb-wrap" id="thumbWrap" style="display:none"></div>
    <div class="thumb-actions" id="thumbActions" style="display:none">
      <button type="button" id="thumbDl" class="btn btn-secondary">&#8681; Download</button>
    </div>
  </div>

  <h2 style="margin-top:2rem">Acties</h2>
  <div class="actions">
    <a href="/live" class="btn btn-primary">&#9654; Live bekijken</a>
    <a href="/videos" class="btn btn-secondary">&#128249; Galerij</a>
  </div>
</main>
<footer>NestboxCam &mdash; ESP32-CAM &bull; <span id="ipAddr"></span></footer>
<script>
var lastVideo = '';
var lastFileCount = -1;
function isIosDevice(){
  return /iPhone|iPad|iPod/i.test(navigator.userAgent) ||
    (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
}
function isImageFile(name){ return /\.(jpe?g)$/i.test(name); }
function shareJpegIfPossible(name, blob){
  try {
    if (!isImageFile(name) || typeof File === 'undefined') return false;
    if (!navigator.share || !navigator.canShare) return false;
    var f = new File([blob], name, { type: 'image/jpeg' });
    if (navigator.canShare({ files: [f] })) {
      navigator.share({ files: [f], title: 'NestboxCam' });
      return true;
    }
  } catch(e) {}
  return false;
}
function setDlToast(text, autoHideMs){
  var el = document.getElementById('dlToast');
  if(!el) return;
  el.textContent = text;
  el.classList.add('show');
  clearTimeout(window.__dlT);
  if(autoHideMs) window.__dlT = setTimeout(function(){ el.classList.remove('show'); }, autoHideMs);
}
function hlRes(k){
  ['btnVga','btnSvga','btnXga'].forEach(function(id){
    var el = document.getElementById(id);
    if(el) el.className = 'btn btn-secondary';
  });
  var map = {vga:'btnVga',svga:'btnSvga',xga:'btnXga'};
  var bid = map[k];
  if(bid && document.getElementById(bid)) document.getElementById(bid).className = 'btn btn-primary';
}
function hlDur(s){
  ['btnD10','btnD20','btnD30','btnD60'].forEach(function(id){
    var el = document.getElementById(id);
    if(el) el.className = 'btn btn-secondary';
  });
  var map = {10:'btnD10',20:'btnD20',30:'btnD30',60:'btnD60'};
  var bid = map[s];
  if(bid && document.getElementById(bid)) document.getElementById(bid).className = 'btn btn-primary';
}
function setRes(key){
  fetch('/api/settings?resolution='+encodeURIComponent(key)).then(function(r){ return r.json(); })
    .then(function(d){
      if(d.ok){ hlRes(key); }
      else alert(d.error || 'Instellen mislukt');
    }).catch(function(){ alert('Netwerkfout'); });
}
function setDur(sec){
  fetch('/api/settings?record_sec='+sec).then(function(r){ return r.json(); })
    .then(function(d){
      if(d.ok){ hlDur(sec); }
      else alert(d.error || 'Instellen mislukt');
    }).catch(function(){ alert('Netwerkfout'); });
}
function doCapture(){
  fetch('/api/capture').then(function(r){ return r.json(); })
    .then(function(d){
      if(d.ok){
        fetchStatus();
        if (isIosDevice()) {
          setDlToast('Foto opgeslagen. Ga naar Galerij → downloadknop bij de JPEG → lang indrukken om in Foto’s te bewaren.', 7000);
        }
      }
      else alert(d.error || 'Foto mislukt');
    }).catch(function(){ alert('Netwerkfout'); });
}
function fetchStatus(){
  fetch('/api/status',{cache:'no-store'}).then(r=>r.json()).then(d=>{
    document.getElementById('camStatus').textContent = d.camera ? 'OK' : 'Fout';
    document.getElementById('camStatus').className = 'card-value ' + (d.camera ? '' : 'warn');
    document.getElementById('freeSpace').textContent = d.free_mb + ' MB';
    document.getElementById('storageDetail').textContent = d.used_mb + ' MB gebruikt van ' + d.total_mb + ' MB';
    document.getElementById('videoCount').textContent = d.video_count;
    document.getElementById('ipAddr').textContent = d.ip;
    if(d.resolution) hlRes(d.resolution);
    if(d.record_sec) hlDur(d.record_sec);
    var pct = d.total_mb > 0 ? (d.used_mb / d.total_mb * 100) : 0;
    var fill = document.getElementById('storageFill');
    fill.style.width = pct + '%';
    fill.className = 'storage-fill' + (pct > 90 ? ' danger' : pct > 70 ? ' warn' : '');
    var rb = document.getElementById('recStatus');
    if(d.recording){
      rb.className = 'recording-badge';
      rb.innerHTML = '<span class="status-dot dot-red"></span>Opname bezig';
    } else {
      rb.className = 'idle-badge';
      rb.innerHTML = '<span class="status-dot dot-green"></span>Inactief';
    }
    var cnt = typeof d.video_count === 'number' ? d.video_count : parseInt(d.video_count, 10) || 0;
    var fileChanged = d.last_video && (d.last_video !== lastVideo || cnt !== lastFileCount);
    if(!d.last_video){
      lastVideo = '';
      lastFileCount = cnt;
      clearThumbnail();
    } else if(fileChanged){
      lastVideo = d.last_video;
      lastFileCount = cnt;
      updateThumbnail(d.last_video);
    }
  }).catch(()=>{});
}
function clearThumbnail(){
  var empty = document.getElementById('thumbEmpty');
  var wrap = document.getElementById('thumbWrap');
  var act = document.getElementById('thumbActions');
  if(wrap){ wrap.innerHTML = ''; wrap.style.display = 'none'; }
  if(empty) empty.style.display = 'flex';
  if(act) act.style.display = 'none';
}
function updateThumbnail(name){
  var wrap = document.getElementById('thumbWrap');
  var empty = document.getElementById('thumbEmpty');
  var enc = encodeURIComponent(name);
  if(empty) empty.style.display = 'none';
  if(wrap){
    wrap.style.display = 'block';
    wrap.innerHTML =
      '<img src="/thumbnail?file=' + enc + '&t=' + Date.now() + '" alt="thumbnail" onerror="this.style.display=\'none\'">' +
      '<div class="thumb-overlay">' + name + '</div>';
  }
  document.getElementById('thumbActions').style.display = 'flex';
  var dlBtn = document.getElementById('thumbDl');
  dlBtn.onclick = function(){
    if (isImageFile(name) && isIosDevice()) {
      setDlToast('Foto openen…', 2000);
      window.open('/download?file=' + enc, '_blank');
      setTimeout(function(){
        setDlToast('Houd de foto ingedrukt → Bewaar naar Foto’s (of deelknop).', 6500);
      }, 400);
      return;
    }
    setDlToast('Bestand wordt nu gedownload, een ogenblik.');
    fetch('/download?file=' + enc).then(function(r){
      if(!r.ok) throw new Error('http');
      return r.blob();
    })
      .then(function(blob){
        if (shareJpegIfPossible(name, blob)) {
          setDlToast('Kies “Bewaar afbeelding” of sla op in Foto’s.', 5500);
          return;
        }
        setDlToast('Download gestart — controleer je downloadmap.', 4500);
        var a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = name || 'bestand';
        a.click();
        URL.revokeObjectURL(a.href);
      })
      .catch(function(){
        setDlToast('Download mislukt. Probeer het opnieuw.', 5000);
      });
  };
}
fetchStatus();
setInterval(fetchStatus, 3000);
</script>
</body>
</html>
)rawhtml";

// ─── Live stream pagina ────────────────────────────────────────────────────────
static const char HTML_LIVE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NestboxCam – Live</title>
<style>
  :root{--bg:#0f1117;--card:#1a1d27;--accent:#4ade80;--accent2:#22d3ee;--text:#e2e8f0;--muted:#64748b;--border:#2d3148}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh;display:flex;flex-direction:column}
  header{background:var(--card);border-bottom:1px solid var(--border);padding:1rem 1.5rem;display:flex;align-items:center;gap:1rem}
  .logo{font-size:1.4rem;font-weight:700;color:var(--accent)}
  .logo span{color:var(--accent2)}
  nav a{color:var(--muted);text-decoration:none;padding:.4rem .8rem;border-radius:.4rem;font-size:.9rem;transition:all .2s;cursor:pointer}
  nav a:hover,nav a.active{color:var(--text);background:rgba(255,255,255,.06)}
  nav{display:flex;gap:.25rem;margin-left:auto}
  .stream-wrap{flex:1;display:flex;align-items:center;justify-content:center;padding:1.5rem;flex-direction:column;gap:1rem}
  .stream-container{position:relative;border:2px solid var(--border);border-radius:.75rem;overflow:hidden;max-width:800px;width:100%}
  .stream-container img{width:100%;display:block;background:#000}
  .live-badge{position:absolute;top:.75rem;left:.75rem;background:rgba(248,113,113,.9);color:#fff;padding:.25rem .6rem;border-radius:.3rem;font-size:.75rem;font-weight:700;letter-spacing:.05em;animation:pulse 1.5s ease-in-out infinite}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.6}}
  .hint{color:var(--muted);font-size:.85rem;text-align:center}
  .warn-box{background:rgba(251,191,36,.1);border:1px solid rgba(251,191,36,.3);color:#fbbf24;padding:.75rem 1rem;border-radius:.5rem;font-size:.85rem;max-width:600px;text-align:center}
  .controls{display:flex;gap:.75rem;align-items:center;flex-wrap:wrap;justify-content:center}
  .led-btn{display:inline-flex;align-items:center;gap:.5rem;padding:.5rem 1.1rem;border-radius:.5rem;border:1px solid rgba(251,191,36,.35);background:rgba(251,191,36,.08);color:#fbbf24;font-size:.88rem;font-weight:600;cursor:pointer;transition:all .2s}
  .led-btn:hover{background:rgba(251,191,36,.18)}
  .led-btn.on{background:rgba(251,191,36,.22);border-color:rgba(251,191,36,.7);color:#fde68a;box-shadow:0 0 10px rgba(251,191,36,.25)}
</style>
</head>
<body>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a onclick="stopAndGo('/')">Dashboard</a>
    <a class="active">Live</a>
    <a onclick="stopAndGo('/videos')">Video's</a>
  </nav>
</header>
<div class="stream-wrap">
  <div class="warn-box">&#9888; Live stream en gelijktijdige opname zijn niet mogelijk. Opname stopt automatisch tijdens live bekijken.</div>
  <div class="stream-container">
    <img src="/stream" id="stream" alt="Live stream">
    <div class="live-badge">LIVE</div>
  </div>
  <div class="controls">
    <button class="led-btn" id="ledBtn" onclick="toggleLed()">&#128294; LED uit</button>
    <p class="hint">Klik op een menuitem om de stream te stoppen en terug te navigeren.</p>
  </div>
</div>
<script>
var ledState = false;
var ledApi = (location.protocol === 'https:' ? '' : 'http://' + location.hostname + ':81') + '/api/led';

function updateLedBtn(){
  var btn = document.getElementById('ledBtn');
  if(ledState){
    btn.textContent = '\uD83D\uDCA1 LED aan';
    btn.className = 'led-btn on';
  } else {
    btn.textContent = '\uD83D\uDCA1 LED uit';
    btn.className = 'led-btn';
  }
}

function toggleLed(){
  fetch(ledApi + '?state=toggle')
    .then(function(r){ return r.json(); })
    .then(function(d){ ledState = d.led; updateLedBtn(); })
    .catch(function(){});
}

function stopAndGo(url) {
  if(ledState){ fetch(ledApi + '?state=off').catch(function(){}); }
  var img = document.getElementById('stream');
  img.src = '';
  setTimeout(function(){ window.location.href = url; }, 150);
}

fetch(ledApi).then(function(r){ return r.json(); })
  .then(function(d){ ledState = d.led; updateLedBtn(); }).catch(function(){});
</script>
</body>
</html>
)rawhtml";

// ─── Video lijst pagina ────────────────────────────────────────────────────────
static const char HTML_VIDEOS[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NestboxCam – Video's</title>
<style>
  :root{--bg:#0f1117;--card:#1a1d27;--accent:#4ade80;--accent2:#22d3ee;--text:#e2e8f0;--muted:#64748b;--danger:#f87171;--border:#2d3148}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh}
  header{background:var(--card);border-bottom:1px solid var(--border);padding:1rem 1.5rem;display:flex;align-items:center;gap:1rem}
  .logo{font-size:1.4rem;font-weight:700;color:var(--accent)}
  .logo span{color:var(--accent2)}
  nav a{color:var(--muted);text-decoration:none;padding:.4rem .8rem;border-radius:.4rem;font-size:.9rem;transition:all .2s}
  nav a:hover,nav a.active{color:var(--text);background:rgba(255,255,255,.06)}
  nav{display:flex;gap:.25rem;margin-left:auto}
  main{padding:1.5rem;max-width:1100px;margin:0 auto}
  h2{font-size:1.1rem;color:var(--muted);margin-bottom:1.2rem;text-transform:uppercase;letter-spacing:.05em;font-weight:600}
  .empty{color:var(--muted);text-align:center;padding:3rem;font-size:.95rem}
  .storage-info{display:flex;gap:1.5rem;margin-bottom:.75rem;padding:.75rem 1rem;background:var(--card);border:1px solid var(--border);border-radius:.5rem;font-size:.85rem}
  .si-label{color:var(--muted);margin-right:.4rem}
  /* Selectie-toolbar */
  .sel-bar{display:none;align-items:center;gap:.75rem;padding:.6rem 1rem;margin-bottom:.75rem;background:rgba(34,211,238,.07);border:1px solid rgba(34,211,238,.25);border-radius:.5rem;font-size:.85rem}
  .sel-bar.visible{display:flex}
  .sel-count{color:var(--accent2);font-weight:600;flex:1}
  .sel-btn{display:inline-flex;align-items:center;gap:.3rem;padding:.3rem .7rem;border-radius:.4rem;border:none;cursor:pointer;font-size:.8rem;font-weight:600;transition:all .2s}
  .sel-all{background:rgba(255,255,255,.07);color:var(--text)}
  .sel-all:hover{background:rgba(255,255,255,.12)}
  .sel-none{background:rgba(255,255,255,.04);color:var(--muted)}
  .sel-none:hover{background:rgba(255,255,255,.08)}
  .sel-del{background:rgba(248,113,113,.15);color:var(--danger);border:1px solid rgba(248,113,113,.3)}
  .sel-del:hover{background:rgba(248,113,113,.28)}
  /* Grid */
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:.6rem}
  .vid-card{background:var(--card);border:2px solid var(--border);border-radius:.5rem;overflow:hidden;transition:border-color .15s;cursor:pointer;user-select:none}
  .vid-card:hover{border-color:var(--accent2)}
  .vid-card.selected{border-color:var(--accent2);background:rgba(34,211,238,.06)}
  /* Checkbox overlay op de thumbnail */
  .thumb-wrap{position:relative;width:100%;aspect-ratio:4/3;background:#000;overflow:hidden}
  .thumb-wrap img{width:100%;height:100%;object-fit:cover;display:block}
  .thumb-wrap .no-thumb{display:flex;align-items:center;justify-content:center;width:100%;height:100%;color:var(--muted);font-size:.7rem}
  .card-check{position:absolute;top:.3rem;left:.3rem;width:18px;height:18px;border-radius:.25rem;border:2px solid rgba(255,255,255,.5);background:rgba(0,0,0,.45);display:flex;align-items:center;justify-content:center;transition:all .15s;font-size:.7rem;line-height:1}
  .vid-card.selected .card-check{background:var(--accent2);border-color:var(--accent2);color:#0f1117}
  .vid-info{padding:.35rem .5rem .2rem}
  .vid-name{font-family:monospace;font-size:.65rem;color:var(--accent2);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;margin-bottom:.1rem}
  .vid-size{font-size:.62rem;color:var(--muted)}
  .vid-actions{display:flex;gap:.25rem;padding:.25rem .5rem .4rem;flex-wrap:wrap}
  .btn{display:inline-flex;align-items:center;gap:.2rem;padding:.2rem .4rem;border-radius:.3rem;border:none;cursor:pointer;font-size:.7rem;font-weight:600;text-decoration:none;transition:all .2s}
  .btn-play{background:rgba(74,222,128,.15);color:var(--accent);border:1px solid rgba(74,222,128,.3)}
  .btn-play:hover{background:rgba(74,222,128,.25)}
  .btn-dl{background:rgba(34,211,238,.15);color:var(--accent2);border:1px solid rgba(34,211,238,.3)}
  .btn-dl:hover{background:rgba(34,211,238,.25)}
  .btn-del{background:rgba(248,113,113,.1);color:var(--danger);border:1px solid rgba(248,113,113,.25)}
  .btn-del:hover{background:rgba(248,113,113,.2)}
  .dl-toast{position:fixed;bottom:1.5rem;left:50%;transform:translateX(-50%) translateY(150%);opacity:0;transition:transform .35s ease,opacity .35s ease;z-index:9999;max-width:min(22rem,calc(100% - 2rem));padding:.85rem 1.25rem;background:var(--card);border:1px solid var(--accent2);border-radius:.5rem;box-shadow:0 12px 40px rgba(0,0,0,.45);font-size:.88rem;text-align:center;color:var(--text);pointer-events:none}
  .dl-toast.show{transform:translateX(-50%) translateY(0);opacity:1}
</style>
</head>
<body>
<div id="dlToast" class="dl-toast" role="status" aria-live="polite"></div>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a href="/">Dashboard</a>
    <a href="/live">Live</a>
    <a href="/videos" class="active">Galerij</a>
  </nav>
</header>
<main>
  <h2>Opgeslagen bestanden</h2>
  <div class="storage-info">
    <span><span class="si-label">Vrij:</span><span id="siFreq">—</span></span>
    <span><span class="si-label">Totaal:</span><span id="siTotal">—</span></span>
    <span><span class="si-label">Aantal:</span><span id="siCount">—</span></span>
  </div>
  <div id="iosVideoHint" style="display:none;font-size:.82rem;color:var(--muted);margin-bottom:.75rem;padding:.65rem .85rem;background:rgba(251,191,36,.08);border:1px solid rgba(251,191,36,.28);border-radius:.5rem;line-height:1.45">
    <strong>iPhone / iPad:</strong> Safari kan <strong>AVI</strong>-nestvideo niet inline afspelen (beperking iOS). Download het bestand naar <strong>Bestanden</strong> en open met <strong>VLC</strong> (gratis in de App Store). Foto’s zijn gewoon JPEG; tik op download en gebruik “Bewaar afbeelding” of open de foto in een nieuw tabblad (houd ingedrukt → Bewaar naar Foto’s).
  </div>
  <!-- Selectie-toolbar (verschijnt bij selectie) -->
  <div class="sel-bar" id="selBar">
    <span class="sel-count" id="selCount">0 geselecteerd</span>
    <button class="sel-btn sel-all"  onclick="selectAll()">Alles</button>
    <button class="sel-btn sel-none" onclick="selectNone()">Niets</button>
    <button class="sel-btn sel-del"  onclick="deleteSelected()">&#128465; Verwijder geselecteerde</button>
  </div>
  <div id="videoList"><div class="empty">Laden...</div></div>
</main>
<script>
var selected = {};
function isIosDevice(){
  return /iPhone|iPad|iPod/i.test(navigator.userAgent) ||
    (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
}
function isImageFile(name){ return /\.(jpe?g)$/i.test(name); }
function shareJpegIfPossible(name, blob){
  try {
    if (!isImageFile(name) || typeof File === 'undefined') return false;
    if (!navigator.share || !navigator.canShare) return false;
    var f = new File([blob], name, { type: 'image/jpeg' });
    if (navigator.canShare({ files: [f] })) {
      navigator.share({ files: [f], title: 'NestboxCam' });
      return true;
    }
  } catch(e) {}
  return false;
}
function setDlToast(text, autoHideMs){
  var el = document.getElementById('dlToast');
  if(!el) return;
  el.textContent = text;
  el.classList.add('show');
  clearTimeout(window.__dlT);
  if(autoHideMs) window.__dlT = setTimeout(function(){ el.classList.remove('show'); }, autoHideMs);
}

function updateSelBar(){
  var keys = Object.keys(selected);
  var n = keys.length;
  var bar = document.getElementById('selBar');
  bar.className = n > 0 ? 'sel-bar visible' : 'sel-bar';
  document.getElementById('selCount').textContent = n + ' geselecteerd';
}

function toggleSelect(name, cardEl){
  if(selected[name]){
    delete selected[name];
    cardEl.classList.remove('selected');
    cardEl.querySelector('.card-check').textContent = '';
  } else {
    selected[name] = true;
    cardEl.classList.add('selected');
    cardEl.querySelector('.card-check').textContent = '✓';
  }
  updateSelBar();
}

function selectAll(){
  document.querySelectorAll('.vid-card').forEach(function(c){
    var n = c.dataset.name;
    selected[n] = true;
    c.classList.add('selected');
    c.querySelector('.card-check').textContent = '✓';
  });
  updateSelBar();
}

function selectNone(){
  selected = {};
  document.querySelectorAll('.vid-card').forEach(function(c){
    c.classList.remove('selected');
    c.querySelector('.card-check').textContent = '';
  });
  updateSelBar();
}

function deleteSelected(){
  var names = Object.keys(selected);
  if(names.length === 0) return;
  if(!confirm(names.length + ' bestand(en) verwijderen?')) return;
  var todo = names.slice();
  var failed = 0;
  function next(){
    if(todo.length === 0){
      if(failed > 0) alert(failed + ' bestand(en) konden niet worden verwijderd.');
      selected = {};
      loadVideos();
      return;
    }
    var name = todo.shift();
    fetch('/delete?file=' + encodeURIComponent(name))
      .then(function(r){ return r.json(); })
      .then(function(d){ if(!d.ok) failed++; next(); })
      .catch(function(){ failed++; next(); });
  }
  next();
}

function loadVideos(){
  fetch('/api/videos').then(r=>r.json()).then(data=>{
    document.getElementById('siFreq').textContent = data.free_mb + ' MB';
    document.getElementById('siTotal').textContent = data.total_mb + ' MB';
    document.getElementById('siCount').textContent = data.videos.length;
    var el = document.getElementById('videoList');
    if(data.videos.length === 0){
      el.innerHTML = '<div class="empty">Geen bestanden opgeslagen.</div>';
      updateSelBar();
      return;
    }
    var cards = data.videos.map(function(v){
      var fn  = encodeURIComponent(v.name);
      var esc = v.name.replace(/\\/g,'\\\\').replace(/'/g,"\\'");
      var isSel = !!selected[v.name];
      var selCls = isSel ? ' selected' : '';
      var chk    = isSel ? '✓' : '';
      return '<div class="vid-card' + selCls + '" data-name="' + esc + '" onclick="toggleSelect(\'' + esc + '\',this)">' +
        '<div class="thumb-wrap">' +
          '<img src="/thumbnail?file=' + fn + '" alt="" ' +
               'onerror="this.outerHTML=\'<div class=no-thumb>geen preview</div>\'">' +
          '<div class="card-check">' + chk + '</div>' +
        '</div>' +
        '<div class="vid-info">' +
          '<div class="vid-name" title="' + v.name + '">' + v.name + '</div>' +
          '<div class="vid-size">' + v.size + '</div>' +
        '</div>' +
        '<div class="vid-actions" onclick="event.stopPropagation()">' +
          '<button type="button" onclick="downloadVideo(\'' + esc + '\')" class="btn btn-dl">&#8681;</button>' +
          '<button onclick="delVideo(\'' + esc + '\')" class="btn btn-del">&#128465;</button>' +
        '</div>' +
      '</div>';
    }).join('');
    el.innerHTML = '<div class="grid">' + cards + '</div>';
    updateSelBar();
  });
}

function downloadVideo(name){
  var enc = encodeURIComponent(name);
  if (isImageFile(name) && isIosDevice()) {
    setDlToast('Foto openen…', 2000);
    window.open('/download?file=' + enc, '_blank');
    setTimeout(function(){
      setDlToast('Houd de foto ingedrukt → Bewaar naar Foto’s.', 6500);
    }, 400);
    return;
  }
  if (/\.(avi|AVI)$/i.test(name) && isIosDevice()) {
    setDlToast('iPhone/iPad: daarna Bestanden → openen in VLC.', 5000);
  } else {
    setDlToast('Bestand wordt nu gedownload, een ogenblik.');
  }
  fetch('/download?file=' + enc)
    .then(function(r){
      if(!r.ok) throw new Error('http');
      return r.blob();
    })
    .then(function(blob){
      if (shareJpegIfPossible(name, blob)) {
        setDlToast('Kies “Bewaar afbeelding” of sla op in Foto’s.', 5500);
        return;
      }
      setDlToast('Download gestart — controleer je downloadmap.', 4500);
      var a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = name || 'video.avi';
      a.click();
      URL.revokeObjectURL(a.href);
    })
    .catch(function(){ setDlToast('Download mislukt. Probeer het opnieuw.', 5000); });
}

function delVideo(name){
  if(!confirm('Bestand verwijderen:\n' + name + '?')) return;
  fetch('/delete?file=' + encodeURIComponent(name))
    .then(r=>r.json()).then(d=>{
      if(d.ok){ delete selected[name]; loadVideos(); }
      else alert('Verwijderen mislukt');
    });
}

loadVideos();
(function(){
  var h = document.getElementById('iosVideoHint');
  if (h && isIosDevice()) h.style.display = 'block';
})();
</script>
</body>
</html>
)rawhtml";

