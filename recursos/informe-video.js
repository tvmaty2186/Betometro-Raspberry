// ---------- Video de la cámara (solo Raspberry) ----------
// C.videos: [{archivo, largada, desdeLargadaMs, girar, espejo}] (girar → 180°, espejo → izquierda/derecha; solo el video, no los datos); cada vuelta con video trae video (cuál) y videoMs (dónde empieza).
// Un punto de la traza (t, segundos desde que empezó la vuelta) está en el video en videoMs + t·1000.
(()=>{const VS=C.videos,caja=$('video');if(!VS||!VS.length)return;
 if(!/^https?:$/.test(location.protocol)){caja.innerHTML='<div class="card aviso">Esta carrera tiene video. El video se ve desde Betómetro en la red.</div>';return}
 const DATOS=[['chip','Vuelta y tipo'],['tiempo','Tiempo de la vuelta'],['ade','Adelanto'],['vel','Velocidad'],['dist','Distancia'],['pista','Pista']];
 const ID={chip:'ovChip',tiempo:'ovTiempo',ade:'ovAde',vel:'ovVel',dist:'ovDist',pista:'ovPista'};
 const NOMBRE={chip:'Vuelta',tiempo:'Vuelta',ade:'Adelanto',vel:'Velocidad',dist:'Distancia',pista:'Pista'};
 let guardado={ocultos:[],pos:{}};
 try{const g=JSON.parse(localStorage.getItem('betoVideoDatos')||'null');if(g&&typeof g==='object')guardado={ocultos:g.ocultos||[],pos:g.pos||{}}}catch(e){}
 const guardar=()=>{try{localStorage.setItem('betoVideoDatos',JSON.stringify(guardado))}catch(e){}};
 const PLAY='<svg viewBox="0 0 26 26"><path d="M5 2l19 11L5 24z" fill="#fff"/></svg>',PAUSA='<svg viewBox="0 0 26 26"><rect x="4" y="2" width="6" height="22" rx="1.5" fill="#fff"/><rect x="16" y="2" width="6" height="22" rx="1.5" fill="#fff"/></svg>';
 const ENG='<svg viewBox="0 0 64 64" fill="none" stroke="#111" stroke-width="6" stroke-linecap="round"><circle cx="32" cy="32" r="9"/><path d="M32 6v9M32 49v9M6 32h9M49 32h9M13.6 13.6l6.4 6.4M44 44l6.4 6.4M13.6 50.4l6.4-6.4M44 20l6.4-6.4"/></svg>';
 const url=a=>'/video/'+encodeURIComponent(a);
 const nombreVid=k=>VS.length===1?'':C.largadas>1?'Largada '+VS[k].largada+(VS.filter(x=>x.largada===VS[k].largada).length>1?' ('+(k+1)+')':''):'Video '+(k+1);
 caja.innerHTML=`<div class="card"><div class="t">Video<b id="vInfoV"></b></div><div class="chips" id="vSel"></div>
<div class="vw" id="vw"><div class="vid" id="vid"><video id="vv" playsinline preload="metadata"></video><div class="toque" id="vToque"></div>
<div class="ov" id="ovChip"></div>
<div class="ov" id="ovAde"><span class="n"></span><span class="e"></span></div>
<div class="ov" id="ovTiempo"><span class="e">Vuelta</span><span class="n"></span></div>
<div class="ov" id="ovDist"><span class="e">Dist.</span><span class="n"></span><span class="u"></span></div>
<div class="ov" id="ovVel"><span class="e">Vel.</span><span class="n"></span><span class="u">km/h</span><span class="vbarra"><i></i></span><span class="e sug">sug. <span class="y"></span></span></div>
<div class="ov" id="ovPista"><svg></svg></div>
<div class="vgrande" id="vGrande"><i></i></div>
<div class="vbar"><button class="pp" id="vPP" aria-label="Reproducir"></button><span class="hora" id="vHora">0:00</span>
<div class="vlinea" id="vLinea"><div class="avance" id="vAvance"></div></div><span class="hora tot" id="vTot">–</span>
<button class="eng no" id="vEng" aria-label="Configurar los datos"></button></div>
<div class="vpanel no" id="vPanel"></div><div class="vmsj no" id="vMsj"></div></div></div>
<div class="vbajar" id="vBajar"></div><div class="pie">doble toque: pantalla completa · tocá una vuelta o el mapa y el video va ahí</div></div>`;
 const vv=$('vv'),vid=$('vid'),vw=$('vw');
 let cur=0,lps=[];
 // ---- de la hora del video a la vuelta y el punto de la traza ----
 const enVideo=ms=>{let b=-1;for(const i of lps)if(C.vueltas[i].videoMs<=ms)b=i;if(b<0)return null;const v=C.vueltas[b];return{i:b,t:Math.min((ms-v.videoMs)/1000,v.realMs/1000),fin:ms-v.videoMs>v.realMs+500}};
 const punto=(i,t)=>{const M=C.vueltas[i].m;if(!M||!M.length)return null;let a=0,b=M.length-1;if(t<=M[0][0])return{p:M[0],ok:!!M[0][7]};if(t>=M[b][0])return{p:M[b],ok:!!M[b][7]};
  while(b-a>1){const c=(a+b)>>1;if(M[c][0]<=t)a=c;else b=c}const P=M[a],Q=M[b],f=(t-P[0])/Math.max(1e-6,Q[0]-P[0]);
  return{p:P.map((x,k)=>k===7?x:x+(Q[k]-x)*f),ok:!!(P[7]&&Q[7])}};
 // ---- mini pista ----
 const mini=()=>{const s=$('ovPista').querySelector('svg');if(!C.pista||C.pista.length<2){$('ovPista').dataset.sin=1;return}
  let x0=1e9,x1=-1e9,y0=1e9,y1=-1e9;C.pista.forEach(([x,y])=>{x0=Math.min(x0,x);x1=Math.max(x1,x);y0=Math.min(y0,y);y1=Math.max(y1,y)});
  const sc=Math.min(204/Math.max(1,x1-x0),124/Math.max(1,y1-y0)),ox=(230-(x1-x0)*sc)/2,oy=(150-(y1-y0)*sc)/2;
  s.setAttribute('viewBox',`${x0-ox/sc} ${y0-oy/sc} ${230/sc} ${150/sc}`);
  const pts=C.pista.map(p=>p.join(',')).join(' ')+(C.cerrada?' '+C.pista[0].join(','):'');
  E(s,'polyline',{points:pts,fill:'none',stroke:'#3a3f45','stroke-width':13/sc,'stroke-linejoin':'round','stroke-linecap':'round'});
  E(s,'polyline',{points:pts,fill:'none',stroke:'#cfd4d8','stroke-width':4/sc,'stroke-linejoin':'round','stroke-linecap':'round'});
  E(s,'circle',{cx:C.pista[0][0],cy:C.pista[0][1],r:5/sc,fill:'#fff'});
  E(s,'circle',{id:'vAuto',r:11/sc,fill:'#f4c531',stroke:'#111','stroke-width':3/sc,cx:-999,cy:-999})};
 mini();
 // ---- poner los datos ----
 const tx=(id,sel,t)=>{const e=$(id).querySelector(sel);if(e.textContent!==t)e.textContent=t};
 function actualizar(){
  const ms=vv.currentTime*1000,dur=duracion(),r=enVideo(ms);
  $('vHora').textContent=hms(ms);$('vAvance').style.width=(dur?Math.min(100,ms/dur*100):0)+'%';
  const v=r&&C.vueltas[r.i],q=r&&!r.fin?punto(r.i,r.t):null,p=q&&q.p;
  // El mapa y los gráficos del informe siguen al video: la vuelta que se ve y la raya en el punto exacto (Matías, 06/11/2026)
  if(r&&!r.fin&&C.vueltas[r.i].m&&C.vueltas[r.i].m.length){if(lap!==r.i){lap=r.i;sel=null;dibujar()}
   const M=C.vueltas[lap].m;let a=0,b=M.length-1;while(b-a>1){const c=(a+b)>>1;if(M[c][0]<=r.t)a=c;else b=c}
   const k=Math.abs(M[b][0]-r.t)<Math.abs(M[a][0]-r.t)?b:a;if(k!==sel)marcar(k)}
  $('ovChip').textContent=!r?'ANTES DE LARGAR':r.fin&&r.i===lps[lps.length-1]?'FIN':nombreV(v)+' · '+(v.interrumpida?'INTERR.':v.medida?'MEDIDA':'LIBRE');
  tx('ovTiempo','.n',r?tfmt(r.t*1000):'–');
  const tot=C.distanciaM||(v&&v.L)||0;
  tx('ovDist','.n',p?Math.round(p[1]).toLocaleString('es-AR'):'–');tx('ovDist','.u',tot?'/ '+Math.round(tot).toLocaleString('es-AR')+' m':'m');
  tx('ovVel','.n',p&&q.ok?String(Math.round(p[2])):'–');
  $('ovVel').querySelector('.vbarra').classList.toggle('no',!vSug);$('ovVel').querySelector('.sug').classList.toggle('no',!vSug);
  if(vSug){tx('ovVel','.y',nf(vSug,1));$('ovVel').querySelector('.vbarra i').style.left=(p&&q.ok?Math.max(0,Math.min(1,(p[2]-vSug)/20+.5))*100:50)+'%'}
  // adelanto: con el mismo signo y colores que la lectura del informe (+ adelantado, − atrasado; como la app)
  const a=$('ovAde');
  if(p&&C.distanciaM){const s=p[4],c=colAde(s);tx('ovAde','.n',(s>0?'+':s<0?'−':'±')+nf(Math.abs(s),1)+' s');tx('ovAde','.e',s>=0?'adelantado':'atrasado');a.querySelector('.n').style.color=c;a.querySelector('.e').style.color=c}
  else{tx('ovAde','.n','–');tx('ovAde','.e','');a.querySelector('.n').style.color=''}
  const au=$('vAuto');if(au){au.setAttribute('cx',p?p[5]:-999);au.setAttribute('cy',p?p[6]:-999)}
 }
 const hms=ms=>{const s=Math.max(0,Math.floor(ms/1000)),h=Math.floor(s/3600),m=Math.floor(s%3600/60),r=String(s%60).padStart(2,'0');return h?h+':'+String(m).padStart(2,'0')+':'+r:m+':'+r};
 const duracion=()=>{if(isFinite(vv.duration)&&vv.duration>0)return vv.duration*1000;const l=lps[lps.length-1];return l===undefined?0:C.vueltas[l].videoMs+C.vueltas[l].realMs};
 // ---- rayitas de las vueltas en la línea ----
 // con muchas vueltas, una rayita cada 2, 5 o 10 (que no se peguen ni tapen la línea)
 function rayas(){const L=$('vLinea'),d=duracion();L.querySelectorAll('.raya').forEach(e=>e.remove());if(!d)return;
  const n=lps.length,sep=n>1?L.clientWidth*(C.vueltas[lps[n-1]].videoMs-C.vueltas[lps[0]].videoMs)/(n-1)/d:1e9,
   paso=[1,2,5,10,20].find(p=>sep*p>=30)||Math.ceil(30/Math.max(sep,.01));
  lps.forEach((i,k)=>{if(k%paso)return;const ms=C.vueltas[i].videoMs;if(ms<0||ms>d)return;const e=document.createElement('div');e.className='raya';e.style.left=ms/d*100+'%';e.title=nombreV(C.vueltas[i]);
   e.addEventListener('pointerdown',ev=>{ev.stopPropagation();ir(ms)});L.append(e)});$('vTot').textContent=hms(d)}
 if(window.ResizeObserver){let anchoL=0;new ResizeObserver(()=>{const w=$('vLinea').clientWidth;if(w&&Math.abs(w-anchoL)>2){anchoL=w;rayas()}}).observe($('vLinea'))}
 const ir=ms=>{const d=duracion();vv.currentTime=Math.max(0,Math.min(d?d/1000-0.05:1e9,ms/1000));actualizar()};
 (()=>{const L=$('vLinea');let tomado=false;const a=ev=>{const r=L.getBoundingClientRect();ir(Math.max(0,Math.min(1,(ev.clientX-r.left)/r.width))*duracion())};
  L.addEventListener('pointerdown',ev=>{tomado=true;L.setPointerCapture(ev.pointerId);a(ev)});L.addEventListener('pointermove',ev=>{if(tomado)a(ev)});
  L.addEventListener('pointerup',()=>tomado=false);L.addEventListener('pointercancel',()=>tomado=false)})();
 // ---- elegir video ----
 function elegir(k){cur=k;const x=VS[k];lps=C.vueltas.map((v,i)=>i).filter(i=>C.vueltas[i].video===k).sort((a,b)=>C.vueltas[a].videoMs-C.vueltas[b].videoMs);
  $('vMsj').classList.add('no');vv.src=url(x.archivo);vv.style.transform=x.girar&&x.espejo?'scaleY(-1)':x.girar?'rotate(180deg)':x.espejo?'scaleX(-1)':'';
  document.querySelectorAll('#vSel .chip').forEach(b=>b.classList.toggle('on',+b.dataset.k===k));$('vInfoV').textContent=nombreVid(k);rayas();actualizar()}
 if(VS.length>1)VS.forEach((x,k)=>{const b=document.createElement('button');b.className='chip';b.dataset.k=k;b.textContent=nombreVid(k);b.onclick=()=>elegir(k);$('vSel').append(b)});
 $('vBajar').innerHTML=VS.map((x,k)=>`<a href="${url(x.archivo)}?descargar=1" download="${x.archivo.replace(/"/g,'')}">Descargar video${VS.length>1?' · '+nombreVid(k):''}</a>`).join('');
 // ---- reproducir, pausa, pantalla completa ----
 let anim=0;const lazo=()=>{actualizar();anim=vv.paused?0:requestAnimationFrame(lazo)};
 const estado=()=>{const p=vv.paused;vid.classList.toggle('pausa',p);$('vPP').innerHTML=p?PLAY:PAUSA;$('vGrande').classList.toggle('no',!p);$('vEng').classList.toggle('no',!p);if(!p){panel(false);if(!anim)anim=requestAnimationFrame(lazo)}};
 const alternar=()=>{if(vv.paused){const pr=vv.play();if(pr&&pr.catch)pr.catch(()=>{})}else vv.pause()};
 ['play','pause','ended'].forEach(e=>vv.addEventListener(e,estado));
 ['timeupdate','seeked','loadeddata'].forEach(e=>vv.addEventListener(e,actualizar));
 vv.addEventListener('loadedmetadata',()=>{rayas();actualizar()});
 vv.addEventListener('error',()=>{const m=$('vMsj');m.classList.remove('no');m.textContent=vv.error&&vv.error.code===4?(/\.mkv(\?|$)/i.test(vv.currentSrc||'')?'El video todavía se está preparando para el celu. Probá en un rato, o descargalo.':'Este navegador no puede abrir el video. Probá con Chrome, o descargalo.'):'No se pudo abrir el video (¿se borró?).'});
 $('vPP').onclick=alternar;
 const completa=()=>{const fs=document.fullscreenElement||document.webkitFullscreenElement;
  if(fs){(document.exitFullscreen||document.webkitExitFullscreen).call(document);return}
  if(vw.classList.contains('fs')){vw.classList.remove('fs');return}
  const f=vw.requestFullscreen||vw.webkitRequestFullscreen;
  if(f){const r=f.call(vw);if(r&&r.then)r.then(()=>{try{screen.orientation.lock('landscape').catch(()=>{})}catch(e){}}).catch(()=>vw.classList.add('fs'))}else vw.classList.add('fs')};
 let toque=null;
 $('vToque').addEventListener('click',()=>{if(toque){clearTimeout(toque);toque=null;completa();return}
  toque=setTimeout(()=>{toque=null;if(!$('vPanel').classList.contains('no'))panel(false);else alternar()},280)});
 document.addEventListener('keydown',ev=>{if(ev.key==='Escape')vw.classList.remove('fs')});
 // ---- configuración (solo en pausa): prender/apagar cada dato y moverlos ----
 const mostrar=()=>DATOS.forEach(([k])=>$(ID[k]).classList.toggle('no',guardado.ocultos.includes(k)||(k==='ade'&&!C.distanciaM)||(k==='pista'&&!!$('ovPista').dataset.sin)));
 function panel(abrir){const P=$('vPanel'),b=$('vEng');P.classList.toggle('no',!abrir);b.classList.toggle('abierto',abrir);b.innerHTML=abrir?'✎ Datos':ENG;
  if(!abrir)return;
  P.innerHTML='<h3>Datos sobre el video</h3>'+DATOS.filter(([k])=>!(k==='ade'&&!C.distanciaM)).map(([k,t])=>`<label>${t}<input type="checkbox" data-k="${k}" ${guardado.ocultos.includes(k)?'':'checked'}><span class="sw"></span></label>`).join('')+
   '<p>Mantené apretado un dato para moverlo · se pega a los otros y a los bordes. <a id="vReset">Volver a como venía</a></p>';
  P.querySelectorAll('input').forEach(i=>i.onchange=()=>{const k=i.dataset.k;guardado.ocultos=guardado.ocultos.filter(x=>x!==k);if(!i.checked)guardado.ocultos.push(k);guardar();mostrar()});
  $('vReset').onclick=()=>{guardado={ocultos:[],pos:{}};guardar();DATOS.forEach(([k])=>{const e=$(ID[k]);e.style.left=e.style.top=e.style.right=e.style.bottom=''});mostrar();panel(true)}}
 $('vEng').onclick=()=>panel($('vPanel').classList.contains('no'));
 const ponerPos=()=>{const W=vid.clientWidth,H=vid.clientHeight;DATOS.forEach(([k])=>{const p=guardado.pos[k],e=$(ID[k]);if(!p)return;
  e.style.right=e.style.bottom='auto';e.style.left=Math.max(0,Math.min(W-e.offsetWidth,p.x*W))+'px';e.style.top=Math.max(0,Math.min(H-e.offsetHeight,p.y*H))+'px'})};
 addEventListener('resize',ponerPos);document.addEventListener('fullscreenchange',()=>setTimeout(ponerPos,60));
 DATOS.forEach(([k])=>{const e=$(ID[k]);let reloj=null,ini=null,mov=false,dx=0,dy=0;
  const limpiar=()=>vid.querySelectorAll('.vguia,.vpega').forEach(g=>g.remove());
  e.addEventListener('pointerdown',ev=>{if(!vv.paused)return;ini={x:ev.clientX,y:ev.clientY};ev.preventDefault();
   reloj=setTimeout(()=>{reloj=null;mov=true;e.classList.add('mov');try{e.setPointerCapture(ev.pointerId)}catch(_){}const r=e.getBoundingClientRect();dx=ini.x-r.left;dy=ini.y-r.top},420)});
  e.addEventListener('pointermove',ev=>{if(reloj&&ini&&Math.hypot(ev.clientX-ini.x,ev.clientY-ini.y)>8){clearTimeout(reloj);reloj=null}
   if(!mov)return;const R=vid.getBoundingClientRect(),W=R.width,H=R.height,w=e.offsetWidth,h=e.offsetHeight,u=parseFloat(getComputedStyle(vid).getPropertyValue('--u'))||W/1280;
   let x=ev.clientX-R.left-dx,y=ev.clientY-R.top-dy;const um=Math.max(8,14*u),mg=22*u,sep=10*u,barra=(18+52+14)*u;
   // imán: bordes del video y los otros datos (alinear bordes o ponerse al lado)
   const otros=DATOS.map(([j])=>j).filter(j=>j!==k&&!$(ID[j]).classList.contains('no')).map(j=>{const o=$(ID[j]),q=o.getBoundingClientRect();return{j,l:q.left-R.left,t:q.top-R.top,r:q.right-R.left,b:q.bottom-R.top}});
   let bx=null,by=null;const prueba=(pos,ref,de,es,eje)=>{const d=Math.abs(pos-ref);const m=eje==='x'?bx:by;if(d<um&&(!m||d<m.d)){const n={d,v:ref-(pos-(eje==='x'?x:y)),linea:ref,de};if(eje==='x')bx=n;else by=n}};
   prueba(x,mg,null,0,'x');prueba(x+w,W-mg,null,0,'x');prueba(y,mg,null,0,'y');prueba(y+h,H-barra,null,0,'y');
   otros.forEach(o=>{prueba(x,o.l,o.j,0,'x');prueba(x+w,o.r,o.j,0,'x');prueba(x,o.r+sep,o.j,0,'x');prueba(x+w,o.l-sep,o.j,0,'x');
    prueba(y,o.t,o.j,0,'y');prueba(y+h,o.b,o.j,0,'y');prueba(y,o.b+sep,o.j,0,'y');prueba(y+h,o.t-sep,o.j,0,'y')});
   if(bx)x=bx.v;if(by)y=by.v;x=Math.max(0,Math.min(W-w,x));y=Math.max(0,Math.min(H-h,y));
   e.style.right=e.style.bottom='auto';e.style.left=x+'px';e.style.top=y+'px';
   limpiar();const g=(c,v)=>{const d=document.createElement('div');d.className='vguia '+c;d.style[c==='x'?'left':'top']=v+'px';vid.append(d)};
   if(bx)g('x',bx.linea);if(by)g('y',by.linea);const de=(by&&by.de)||(bx&&bx.de);
   if(de){const l=document.createElement('div');l.className='vpega';l.textContent='se pega a «'+NOMBRE[de]+'»';l.style.left=x+'px';l.style.top=(y+h+8*u>H-barra-20*u?y-26*u:y+h+8*u)+'px';vid.append(l)}});
  const fin=()=>{if(reloj){clearTimeout(reloj);reloj=null}ini=null;if(!mov)return;mov=false;e.classList.remove('mov');limpiar();
   guardado.pos[k]={x:e.offsetLeft/vid.clientWidth,y:e.offsetTop/vid.clientHeight};guardar()};
  e.addEventListener('pointerup',fin);e.addEventListener('pointercancel',fin);e.addEventListener('contextmenu',ev=>ev.preventDefault())});
 // ---- desde el informe: una vuelta de la tabla o un punto del mapa llevan el video ahí ----
 irVideo=(i,t)=>{const v=C.vueltas[i];if(!v||v.video==null||!VS[v.video])return;if(v.video!==cur)elegir(v.video);
  const ms=v.videoMs+(t||0)*1000;if(vv.readyState>=1)ir(ms);else vv.addEventListener('loadedmetadata',()=>ir(ms),{once:true})};
 $('vEng').innerHTML=ENG;mostrar();elegir(0);estado();setTimeout(ponerPos,0);
})();
