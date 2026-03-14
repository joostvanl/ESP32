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
</style>
</head>
<body>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a href="/" class="active">Dashboard</a>
    <a href="/live">Live</a>
    <a href="/videos">Video's</a>
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
      <div class="card-label">Opgenomen video's</div>
      <div class="card-value blue" id="videoCount">—</div>
    </div>
  </div>

  <h2>Acties</h2>
  <div class="actions">
    <a href="/live" class="btn btn-primary">&#9654; Live bekijken</a>
    <a href="/videos" class="btn btn-secondary">&#128249; Video's</a>
  </div>
</main>
<footer>NestboxCam &mdash; ESP32-CAM &bull; <span id="ipAddr"></span></footer>
<script>
function fetchStatus(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('camStatus').textContent = d.camera ? 'OK' : 'Fout';
    document.getElementById('camStatus').className = 'card-value ' + (d.camera ? '' : 'warn');
    document.getElementById('freeSpace').textContent = d.free_mb + ' MB';
    document.getElementById('storageDetail').textContent = d.used_mb + ' MB gebruikt van ' + d.total_mb + ' MB';
    document.getElementById('videoCount').textContent = d.video_count;
    document.getElementById('ipAddr').textContent = d.ip;
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
  }).catch(()=>{});
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
  nav a{color:var(--muted);text-decoration:none;padding:.4rem .8rem;border-radius:.4rem;font-size:.9rem;transition:all .2s}
  nav a:hover,nav a.active{color:var(--text);background:rgba(255,255,255,.06)}
  nav{display:flex;gap:.25rem;margin-left:auto}
  .stream-wrap{flex:1;display:flex;align-items:center;justify-content:center;padding:1.5rem;flex-direction:column;gap:1rem}
  .stream-container{position:relative;border:2px solid var(--border);border-radius:.75rem;overflow:hidden;max-width:800px;width:100%}
  .stream-container img{width:100%;display:block;background:#000}
  .live-badge{position:absolute;top:.75rem;left:.75rem;background:rgba(248,113,113,.9);color:#fff;padding:.25rem .6rem;border-radius:.3rem;font-size:.75rem;font-weight:700;letter-spacing:.05em;animation:pulse 1.5s ease-in-out infinite}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.6}}
  .hint{color:var(--muted);font-size:.85rem;text-align:center}
  .warn-box{background:rgba(251,191,36,.1);border:1px solid rgba(251,191,36,.3);color:#fbbf24;padding:.75rem 1rem;border-radius:.5rem;font-size:.85rem;max-width:600px;text-align:center}
</style>
</head>
<body>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a href="/">Dashboard</a>
    <a href="/live" class="active">Live</a>
    <a href="/videos">Video's</a>
  </nav>
</header>
<div class="stream-wrap">
  <div class="warn-box">&#9888; Live stream en gelijktijdige opname zijn niet mogelijk. Opname stopt automatisch tijdens live bekijken.</div>
  <div class="stream-container">
    <img src="/stream" id="stream" alt="Live stream">
    <div class="live-badge">LIVE</div>
  </div>
  <p class="hint">Stream wordt automatisch vernieuwd. Sluit dit tabblad om te stoppen.</p>
</div>
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
  main{padding:1.5rem;max-width:960px;margin:0 auto}
  h2{font-size:1.1rem;color:var(--muted);margin-bottom:1.2rem;text-transform:uppercase;letter-spacing:.05em;font-weight:600}
  .empty{color:var(--muted);text-align:center;padding:3rem;font-size:.95rem}
  table{width:100%;border-collapse:collapse}
  th{text-align:left;padding:.6rem .75rem;font-size:.75rem;color:var(--muted);text-transform:uppercase;letter-spacing:.05em;border-bottom:1px solid var(--border)}
  td{padding:.75rem;border-bottom:1px solid rgba(45,49,72,.5);font-size:.9rem;vertical-align:middle}
  tr:hover td{background:rgba(255,255,255,.02)}
  .fname{font-family:monospace;font-size:.82rem;color:var(--accent2)}
  .actions{display:flex;gap:.5rem}
  .btn{display:inline-flex;align-items:center;gap:.3rem;padding:.35rem .7rem;border-radius:.4rem;border:none;cursor:pointer;font-size:.8rem;font-weight:600;text-decoration:none;transition:all .2s}
  .btn-play{background:rgba(74,222,128,.15);color:var(--accent);border:1px solid rgba(74,222,128,.3)}
  .btn-play:hover{background:rgba(74,222,128,.25)}
  .btn-dl{background:rgba(34,211,238,.15);color:var(--accent2);border:1px solid rgba(34,211,238,.3)}
  .btn-dl:hover{background:rgba(34,211,238,.25)}
  .btn-del{background:rgba(248,113,113,.1);color:var(--danger);border:1px solid rgba(248,113,113,.25)}
  .btn-del:hover{background:rgba(248,113,113,.2)}
  .storage-info{display:flex;gap:1.5rem;margin-bottom:1.5rem;padding:.75rem 1rem;background:var(--card);border:1px solid var(--border);border-radius:.5rem;font-size:.85rem}
  .si-label{color:var(--muted);margin-right:.4rem}
</style>
</head>
<body>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a href="/">Dashboard</a>
    <a href="/live">Live</a>
    <a href="/videos" class="active">Video's</a>
  </nav>
</header>
<main>
  <h2>Opgeslagen video's</h2>
  <div class="storage-info" id="storageInfo">
    <span><span class="si-label">Vrij:</span><span id="siFreq">—</span></span>
    <span><span class="si-label">Totaal:</span><span id="siTotal">—</span></span>
    <span><span class="si-label">Video's:</span><span id="siCount">—</span></span>
  </div>
  <div id="videoList"><div class="empty">Laden...</div></div>
</main>
<script>
function loadVideos(){
  fetch('/api/videos').then(r=>r.json()).then(data=>{
    document.getElementById('siFreq').textContent = data.free_mb + ' MB';
    document.getElementById('siTotal').textContent = data.total_mb + ' MB';
    document.getElementById('siCount').textContent = data.videos.length;
    var el = document.getElementById('videoList');
    if(data.videos.length === 0){
      el.innerHTML = '<div class="empty">Geen video\'s opgeslagen.</div>';
      return;
    }
    var rows = data.videos.map(function(v){
      var fn = encodeURIComponent(v.name);
      return '<tr>' +
        '<td class="fname">' + v.name + '</td>' +
        '<td>' + v.size + '</td>' +
        '<td><div class="actions">' +
          '<a href="/play?file=' + fn + '" class="btn btn-play">&#9654; Afspelen</a>' +
          '<a href="/download?file=' + fn + '" class="btn btn-dl">&#8681; Download</a>' +
          '<button onclick="delVideo(\'' + v.name + '\')" class="btn btn-del">&#128465; Verwijder</button>' +
        '</div></td></tr>';
    }).join('');
    el.innerHTML = '<table><thead><tr><th>Bestandsnaam</th><th>Grootte</th><th>Acties</th></tr></thead><tbody>' + rows + '</tbody></table>';
  });
}
function delVideo(name){
  if(!confirm('Video verwijderen: ' + name + '?')) return;
  fetch('/delete?file=' + encodeURIComponent(name), {method:'GET'})
    .then(r=>r.json()).then(d=>{
      if(d.ok) loadVideos();
      else alert('Verwijderen mislukt');
    });
}
loadVideos();
</script>
</body>
</html>
)rawhtml";

// ─── Video player pagina ──────────────────────────────────────────────────────
static const char HTML_PLAYER[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NestboxCam – Afspelen</title>
<style>
  :root{--bg:#0f1117;--card:#1a1d27;--accent:#4ade80;--accent2:#22d3ee;--text:#e2e8f0;--muted:#64748b;--border:#2d3148}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh;display:flex;flex-direction:column}
  header{background:var(--card);border-bottom:1px solid var(--border);padding:1rem 1.5rem;display:flex;align-items:center;gap:1rem}
  .logo{font-size:1.4rem;font-weight:700;color:var(--accent)}
  .logo span{color:var(--accent2)}
  nav a{color:var(--muted);text-decoration:none;padding:.4rem .8rem;border-radius:.4rem;font-size:.9rem;transition:all .2s}
  nav a:hover{color:var(--text);background:rgba(255,255,255,.06)}
  nav{display:flex;gap:.25rem;margin-left:auto}
  .player-wrap{flex:1;display:flex;flex-direction:column;align-items:center;padding:1.5rem;gap:1rem}
  video{max-width:800px;width:100%;border-radius:.75rem;border:2px solid var(--border);background:#000}
  .meta{color:var(--muted);font-size:.85rem;font-family:monospace}
  .btn{display:inline-flex;align-items:center;gap:.4rem;padding:.5rem 1rem;border-radius:.5rem;border:none;cursor:pointer;font-size:.85rem;font-weight:600;text-decoration:none;transition:all .2s}
  .btn-primary{background:var(--accent);color:#0f1117}
  .btn-primary:hover{background:#22c55e}
  .btn-secondary{background:var(--card);color:var(--text);border:1px solid var(--border)}
  .btn-secondary:hover{background:rgba(255,255,255,.06)}
  .actions{display:flex;gap:.75rem}
  .info-box{background:rgba(34,211,238,.08);border:1px solid rgba(34,211,238,.2);color:var(--accent2);padding:.75rem 1rem;border-radius:.5rem;font-size:.82rem;max-width:600px;width:100%}
</style>
</head>
<body>
<header>
  <div class="logo">Nestbox<span>Cam</span></div>
  <nav>
    <a href="/">Dashboard</a>
    <a href="/live">Live</a>
    <a href="/videos">Video's</a>
  </nav>
</header>
<div class="player-wrap">
  <div class="info-box">&#8505; AVI/MJPEG bestanden worden afgespeeld via de browser. Als de video niet afspeelt, gebruik dan de downloadknop en open met VLC.</div>
  <video id="player" controls autoplay>
    <source id="vsrc" src="" type="video/avi">
    Uw browser ondersteunt deze video niet. <a href="#" id="dllink">Download de video</a>.
  </video>
  <div class="meta" id="fname">—</div>
  <div class="actions">
    <a href="#" id="dlbtn" class="btn btn-primary">&#8681; Download</a>
    <a href="/videos" class="btn btn-secondary">&#8592; Terug</a>
  </div>
</div>
<script>
var params = new URLSearchParams(window.location.search);
var file = params.get('file') || '';
var enc = encodeURIComponent(file);
document.getElementById('vsrc').src = '/download?file=' + enc;
document.getElementById('player').load();
document.getElementById('fname').textContent = file;
document.getElementById('dlbtn').href = '/download?file=' + enc;
document.getElementById('dllink').href = '/download?file=' + enc;
</script>
</body>
</html>
)rawhtml";
