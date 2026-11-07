// ---------- Gráficos a pantalla completa con zoom (solo Raspberry, 1.0.30) ----------
// Doble toque sobre el gráfico de velocidad o el de aceleración: se abre acostado a pantalla completa (si el celu
// está de pie, la página se gira sola) con toda la carrera a lo largo de la distancia: cuadrícula cada 100 m y
// cada 5 km/h, todas las lecturas sin suavizar con color por tramo, la sugerida, una raya por vuelta con su tiempo
// y error, máx/mín/prom de lo visible, los botones Velocidad/Aceleración/Adelanto, la X. Toque = punto (globito);
// doble toque = acercar; pellizco = zoom; arrastrar = recorrer; abajo la barrita con toda la carrera.
// Muestra aprobada: docs/muestras/1.0.30/graficos/3-velocidad-celu.png.
(()=>{if(lap<0)return;
 const COL={rojo:'#d7263d',ama:'#f4c531',verde:'#2ecc71',azul:'#3d8bfd',cel:'#7fd3ff',sub:'#b9cdb8',crema:'#f3ead2',ink:'#f7f7f2'};
 const tramos=[];let acum=0;conT.forEach(i=>{const v=C.vueltas[i];tramos.push({i,v,d0:acum,L:v.L||1});acum+=v.L||1});const total=acum||1;
 let sumP=0,nP=0;tramos.forEach(t=>t.v.m.forEach(p=>{if(p[7]){sumP+=p[2];nP++}}));const prom=nP?sumP/nP:0,ref=vSug||prom;
 const Z={modo:'vel',de:0,a:total,sel:null,abierto:false,esc:{yA:0,yB:1,fino:5,medio:10},gira:false};
 const km=(m,d)=>nf(m/1000,d),sg=(v,d)=>(v>0?'+':v<0?'−':'±')+nf(Math.abs(v),d);
 const valor=p=>Z.modo==='vel'?p[2]:Z.modo==='ace'?p[3]:p[4];
 const colZ=p=>{if(!p[7])return'#555';if(Z.modo==='ace')return colAce(p[3]);if(Z.modo==='ade')return colAde(p[4]);const d=p[2]-ref;return d>6?COL.rojo:d>2?COL.ama:d<-6?COL.azul:d<-2?COL.cel:COL.verde};
 const cssCol=c=>c.startsWith('var(')?COL[c.slice(6,-1)]||c:c;   // colAce/colAde devuelven var(--…): el canvas quiere el color
 // ---- la pantalla ----
 const gz=document.createElement('div');gz.id='gz';gz.className='gz no';
 gz.innerHTML=`<canvas id="gzC"></canvas><div class="t" id="gzT"></div><div class="chips" id="gzModos"></div><button class="gzx" id="gzX" aria-label="Cerrar"><svg viewBox="0 0 64 64" fill="none" stroke="#f7f7f2" stroke-width="6" stroke-linecap="round"><path d="M16 16l32 32M48 16L16 48"/></svg></button>
<div class="t t2" id="gzT2"></div><div class="gzG no" id="gzG"></div><div class="pie" id="gzPie">doble toque: acercar · arrastrá para recorrer · X para cerrar</div>`;
 document.body.append(gz);
 const cv=$('gzC'),g=cv.getContext('2d'),glob=$('gzG');
 for(const k in MODOS){if(k==='ade'&&!C.distanciaM)continue;const b=document.createElement('button');b.className='chip';b.dataset.m=k;b.textContent=MODOS[k];b.onclick=()=>{Z.modo=k;recalcular()};$('gzModos').append(b)}
 $('gzX').onclick=cerrar;
 document.addEventListener('keydown',ev=>{if(ev.key==='Escape'&&Z.abierto)cerrar()});
 let W=0,H=0,L={};
 function medir(){
  Z.gira=innerHeight>innerWidth;gz.classList.toggle('gira',Z.gira);
  W=Z.gira?innerHeight:innerWidth;H=Z.gira?innerWidth:innerHeight;
  gz.style.width=W+'px';gz.style.height=H+'px';
  const dpr=Math.min(3,devicePixelRatio||1);cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);cv.style.width=W+'px';cv.style.height=H+'px';g.setTransform(dpr,0,0,dpr,0,0);
  L={x0:40,y0:54,x1:W-24,y1:H-100,bx0:40,by0:H-76,bx1:W-24,by1:H-42};
  $('gzPie').style.top=(H-34)+'px'}
 const X=d=>L.x0+(L.x1-L.x0)*(d-Z.de)/(Z.a-Z.de),D=x=>Z.de+(x-L.x0)/(L.x1-L.x0)*(Z.a-Z.de),Y=v=>L.y1-(L.y1-L.y0)*(v-Z.esc.yA)/(Z.esc.yB-Z.esc.yA);
 function visibles(m){const r=[];for(const t of tramos){if(t.d0+t.L<Z.de-m||t.d0>Z.a+m)continue;const M=t.v.m;for(let k=0;k<M.length;k++){const d=t.d0+M[k][1];if(d<Z.de-m)continue;if(d>Z.a+m)break;r.push({t,p:M[k],d})}}return r}
 function ver(nDe,nSpan){const s=Math.max(Math.min(200,total),Math.min(total,nSpan));Z.de=Math.max(0,Math.min(total-s,nDe));Z.a=Z.de+s;recalcular()}
 function acercar(x,f){const dC=D(x),s=(Z.a-Z.de)/f;ver(dC-(x-L.x0)/(L.x1-L.x0)*s,s)}
 function abrir(modo,lapIni){
  Z.modo=modo;Z.sel=null;const t=tramos.find(x=>x.i===lapIni);
  if(t&&tramos.length>1){Z.de=t.d0;Z.a=t.d0+t.L}else{Z.de=0;Z.a=total}
  Z.abierto=true;gz.classList.remove('no');document.body.style.overflow='hidden';medir();recalcular()}
 function cerrar(){Z.abierto=false;gz.classList.add('no');document.body.style.overflow=''}
 addEventListener('resize',()=>{if(Z.abierto){medir();recalcular()}});
 function recalcular(){
  const vis=visibles(0).filter(q=>q.p[7]);let vmin=Infinity,vmax=-Infinity,s=0;
  for(const q of vis){const v=valor(q.p);vmin=Math.min(vmin,v);vmax=Math.max(vmax,v);s+=v}
  if(!vis.length){vmin=0;vmax=Z.modo==='vel'?100:1}const pr=vis.length?s/vis.length:0;
  let yA,yB,fino,medio,stats;
  if(Z.modo==='vel'){yA=Math.max(0,Math.floor((vmin-4)/5)*5);yB=Math.ceil((vmax+4)/5)*5;if(yB-yA<20)yB=yA+20;fino=5;medio=10;if(yB-yA>80){fino=10;medio=20}
   stats='máx '+Math.round(vmax)+' · mín '+Math.round(vmin)+' · prom '+nf(pr,1)}
  else if(Z.modo==='ace'){const am=Math.max(0.5,Math.ceil(Math.max(Math.abs(vmin),Math.abs(vmax))*2)/2);yA=-am;yB=am;fino=0.5;medio=1;if(am>4){fino=1;medio=2}
   stats='máx '+sg(vmax,1)+' · frenada '+sg(vmin,1)+' · prom '+sg(pr,2)}
  else{yA=Math.floor(vmin-1);yB=Math.ceil(vmax+1);if(yB-yA<4){yA-=1;yB+=1}fino=0.5;medio=1;if(yB-yA>12){fino=1;medio=5}if(yB-yA>60){fino=5;medio=10}
   stats='máx '+sg(vmax,1)+' · mín '+sg(vmin,1)+' · prom '+sg(pr,1)}
  Z.esc={yA,yB,fino,medio};
  const tol=(Z.a-Z.de)*0.02,vs=tramos.filter(t=>t.d0+t.L>Z.de+tol&&t.d0<Z.a-tol).map(t=>nombreV(t.v));
  const tit={vel:'Velocidad km/h',ace:'Aceleración m/s²',ade:'Adelanto s'}[Z.modo];
  $('gzT').innerHTML=tit+(Z.modo==='vel'?'<b>'+(vSug?'sugerida '+nf(vSug,1):prom?'promedio '+nf(prom,1):'')+'</b>':'');
  $('gzT2').innerHTML='Tramo visible · km '+km(Z.de,1)+' a '+km(Z.a,1)+(C.recorrido?'':vs.length===1?' · '+vs[0]:vs.length===2?' · '+vs[0]+' y '+vs[1]:vs.length?' · '+vs[0]+' a '+vs[vs.length-1]:'')+'<b>'+stats+'</b>';
  document.querySelectorAll('#gzModos .chip').forEach(b=>b.classList.toggle('on',b.dataset.m===Z.modo));
  dibujar()}
 function dibujar(){
  g.clearRect(0,0,W,H);const {x0,y0,x1,y1}=L,e=Z.esc;
  // cuadrícula
  const pasos=[100,200,500,1000,2000,5000,10000];let paso=100;for(const p of pasos){paso=p;if((Z.a-Z.de)/p<=60)break}
  g.lineWidth=1;g.fillStyle=COL.sub;g.font='10px "Barlow",sans-serif';g.textAlign='center';
  for(let d=Math.ceil(Z.de/paso)*paso;d<=Z.a;d+=paso){const f=d%(paso*5)===0,x=X(d);g.strokeStyle=f?'#363b41':'#ffffff0d';g.beginPath();g.moveTo(x,y0);g.lineTo(x,y1);g.stroke();if(f)g.fillText(d%1000===0?km(d,0)+' km':km(d,1),x,y1+12)}
  g.textAlign='end';const dec=e.fino<1?1:0;
  for(let v=e.yA;v<=e.yB+1e-6;v+=e.fino){const f=Math.abs(v/e.medio-Math.round(v/e.medio))<1e-6,y=Y(v);g.strokeStyle=f?'#363b41':'#ffffff0d';g.beginPath();g.moveTo(x0,y);g.lineTo(x1,y);g.stroke();if(f)g.fillText(Z.modo==='vel'?String(Math.round(v)):sg(v,dec),x0-5,y+3)}
  // sugerida (velocidad) o el cero
  if(Z.modo==='vel'&&ref>e.yA&&ref<e.yB){g.strokeStyle='#fff';g.setLineDash([4,4]);g.beginPath();g.moveTo(x0,Y(ref));g.lineTo(x1,Y(ref));g.stroke();g.setLineDash([]);g.fillStyle='#fff';g.font='700 10px "Barlow",sans-serif';g.textAlign='left';g.fillText((vSug?'sugerida ':'promedio ')+nf(ref,1),x0+8,Y(ref)+12)}
  else if(Z.modo!=='vel'&&0>e.yA&&0<e.yB){g.strokeStyle=Z.modo==='ade'?COL.ama:'#ffffff73';if(Z.modo==='ade')g.setLineDash([4,4]);g.beginPath();g.moveTo(x0,Y(0));g.lineTo(x1,Y(0));g.stroke();g.setLineDash([]);
   if(Z.modo==='ade'){g.fillStyle=COL.ama;g.font='700 10px "Barlow",sans-serif';g.textAlign='left';g.fillText('en hora',x0+8,Y(0)+12)}}
  // vueltas (o marcas del recorrido libre)
  g.setLineDash([3,3]);g.strokeStyle=COL.ama;g.font='italic 800 10px "Barlow",sans-serif';g.textAlign='left';
  if(C.recorrido){(C.marcas||[]).forEach(k=>{const d=k.m||0;if(d<Z.de||d>Z.a)return;g.beginPath();g.moveTo(X(d),y0);g.lineTo(X(d),y1);g.stroke();g.fillStyle=COL.ama;g.fillText('Marca '+k.n+' · '+tfmt(k.ms).replace(/:\d$/,''),X(d)+4,y0+11)})}
  else for(const t of tramos){if(t.d0<Z.de-1||t.d0>Z.a+1)continue;const x=X(t.d0);g.beginPath();g.moveTo(x,y0);g.lineTo(x,y1);g.stroke();if(t.d0>=Z.a-1)continue;const v=t.v;
   const txt=nombreV(v)+' · '+(v.interrumpida?'interrumpida':v.medida?'medida':'libre')+' · '+tfmt(v.realMs)+' · ';g.fillStyle=COL.ama;g.fillText(txt,x+4,y0+11);
   if(v.difMs!=null){g.fillStyle=cssCol(colAde(v.difMs/1000));g.fillText(dif(v.difMs),x+4+g.measureText(txt).width,y0+11)}}
  g.setLineDash([]);
  // la traza, lectura por lectura, con el color de cada tramo (solo lo visible)
  g.save();g.beginPath();g.rect(x0,y0,x1-x0,y1-y0);g.clip();
  const pts=visibles((Z.a-Z.de)*0.01+20),salto=Math.max(1,Math.floor(pts.length/((x1-x0)*2)));
  g.lineWidth=2;g.lineJoin='round';g.lineCap='round';let col=null,ab=false;
  for(let i=0;i<pts.length;i+=salto){const q=pts[i];if(!q.p[7]){if(ab)g.stroke();ab=false;col=null;continue}const c=cssCol(colZ(q.p)),px=X(q.d),py=Y(valor(q.p));
   if(c!==col){if(ab){g.lineTo(px,py);g.stroke()}g.strokeStyle=c;g.beginPath();g.moveTo(px,py);col=c;ab=true}else g.lineTo(px,py)}
  if(ab)g.stroke();g.restore();
  // barrita: toda la carrera y el recuadro de lo que se ve
  const {bx0,by0,bx1,by1}=L,XB=d=>bx0+(bx1-bx0)*d/total;
  g.fillStyle='#00000059';g.strokeStyle='#cfd4d880';g.lineWidth=1;g.beginPath();g.roundRect?g.roundRect(bx0,by0,bx1-bx0,by1-by0,6):g.rect(bx0,by0,bx1-bx0,by1-by0);g.fill();g.stroke();
  let vmin=Infinity,vmax=-Infinity;tramos.forEach(t=>t.v.m.forEach(p=>{if(p[7]){vmin=Math.min(vmin,p[2]);vmax=Math.max(vmax,p[2])}}));if(!(vmax>vmin)){vmin=0;vmax=100}
  const YB=v=>by1-4-(by1-by0-8)*(v-vmin+3)/(vmax-vmin+6);
  g.setLineDash([3,3]);g.strokeStyle=COL.ama;g.fillStyle=COL.ama;g.font='italic 800 8px "Barlow",sans-serif';g.textAlign='left';
  for(const t of tramos){if(t.d0>0){g.beginPath();g.moveTo(XB(t.d0),by0+2);g.lineTo(XB(t.d0),by1-2);g.stroke()}if(!C.recorrido&&(bx1-bx0)/tramos.length>22)g.fillText(nombreV(t.v),XB(t.d0)+5,by0+10)}
  if(C.recorrido)(C.marcas||[]).forEach(k=>{g.beginPath();g.moveTo(XB(k.m||0),by0+2);g.lineTo(XB(k.m||0),by1-2);g.stroke();g.fillText(String(k.n),XB(k.m||0)+3,by0+10)});
  g.setLineDash([]);g.strokeStyle='#b9cdb899';g.lineWidth=1;const sb=Math.max(1,Math.floor(tramos.reduce((s,t)=>s+t.v.m.length,0)/(bx1-bx0)));
  g.beginPath();ab=false;for(const t of tramos)for(let k=0;k<t.v.m.length;k+=sb){const p=t.v.m[k];if(!p[7]){ab=false;continue}if(!ab){g.moveTo(XB(t.d0+p[1]),YB(p[2]));ab=true}else g.lineTo(XB(t.d0+p[1]),YB(p[2]))}g.stroke();
  const rx=XB(Z.de),rw=Math.max(6,XB(Z.a)-rx);g.fillStyle='#f4c53129';g.strokeStyle=COL.ama;g.lineWidth=2;g.beginPath();g.roundRect?g.roundRect(rx,by0-2,rw,by1-by0+4,5):g.rect(rx,by0-2,rw,by1-by0+4);g.fill();g.stroke();
  g.fillStyle=COL.ama;g.strokeStyle='#111';g.lineWidth=1;for(const xx of[rx,rx+rw]){g.beginPath();g.roundRect?g.roundRect(xx-3,(by0+by1)/2-8,6,16,3):g.rect(xx-3,(by0+by1)/2-8,6,16);g.fill();g.stroke()}
  // el punto elegido y su globito
  const s=Z.sel;glob.classList.toggle('no',!s);if(!s)return;
  const px=X(s.d),py=Y(valor(s.p));g.strokeStyle='#ffffffcc';g.lineWidth=1.5;g.beginPath();g.moveTo(px,y0);g.lineTo(px,y1);g.stroke();
  const p=s.p,ade=p[4],gw=176,izq=px-gw-20>=x0-10,gx=izq?px-gw-20:px+20,gy=Math.max(y0-6,Math.min(y1-66,py-26));
  const grande=Z.modo==='vel'?Math.round(p[2])+' <small>km/h</small>':Z.modo==='ace'?sg(p[3],1)+' <small>m/s²</small>':sg(p[4],1)+' <small>s</small>';
  const linea3=Z.modo!=='vel'?Math.round(p[2])+' km/h en ese punto':C.distanciaM?sg(ade,1)+' s '+(ade>=0?'adelantado':'atrasado'):sg(p[3],1)+' m/s²';
  glob.style.left=gx+'px';glob.style.top=gy+'px';glob.style.width=gw+'px';
  glob.innerHTML=`<div class="f1"><b>${grande}</b>${C.recorrido?'':'<span class="chip on">'+nombreV(s.t.v)+'</span>'}</div><div class="f2">km ${km(s.d,2)} · ${C.recorrido?tfmt(p[0]*1000).replace(/:\d$/,''):tfmt(p[0]*1000)+' de vuelta'}</div><div class="f3" style="color:${Z.modo==='vel'&&C.distanciaM?cssCol(colAde(ade)):'var(--ink)'}">${linea3}</div>`;
  g.strokeStyle='#cfd4d8';g.lineWidth=1.5;g.beginPath();g.moveTo(izq?gx+gw:gx,gy+30);g.lineTo(px+(izq?-6:6),py);g.stroke();
  g.beginPath();g.arc(px,py,5,0,Math.PI*2);g.fillStyle='#fff';g.fill();g.strokeStyle='#111';g.lineWidth=2;g.stroke()}
 function elegir(x){const d=D(x),vis=visibles(0);let q=null;for(const w of vis)if(!q||Math.abs(w.d-d)<Math.abs(q.d-d))q=w;Z.sel=q;dibujar()}
 // ---- gestos: un dedo arrastra, dos hacen zoom; toque = punto; doble toque = acercar; la barrita se arrastra ----
 const loc=ev=>Z.gira?{x:ev.clientY,y:innerWidth-ev.clientX}:{x:ev.clientX,y:ev.clientY};
 const dedos=new Map();let ini=null,movio=false,ultToque=null,relojToque=null,pin=null,enBarra=false,dB=0;
 cv.addEventListener('pointerdown',ev=>{const q=loc(ev);cv.setPointerCapture(ev.pointerId);dedos.set(ev.pointerId,q);ev.preventDefault();
  if(dedos.size===2){const[a,b]=[...dedos.values()];pin={d:Math.max(10,Math.abs(a.x-b.x)),span:Z.a-Z.de,de:Z.de,cx:(a.x+b.x)/2};ini=null;return}
  enBarra=q.y>=L.by0-12&&q.y<=L.by1+12;
  if(enBarra){const d=(q.x-L.bx0)/(L.bx1-L.bx0)*total;if(d>=Z.de-total*.01&&d<=Z.a+total*.01)dB=d-Z.de;else{dB=(Z.a-Z.de)/2;ver(d-dB,Z.a-Z.de)}return}
  ini={x:q.x,y:q.y,de:Z.de};movio=false});
 cv.addEventListener('pointermove',ev=>{if(!dedos.has(ev.pointerId))return;const q=loc(ev);dedos.set(ev.pointerId,q);
  if(pin&&dedos.size===2){const[a,b]=[...dedos.values()],d=Math.max(10,Math.abs(a.x-b.x)),s=pin.span*pin.d/d,dC=pin.de+(pin.cx-L.x0)/(L.x1-L.x0)*pin.span;ver(dC-((a.x+b.x)/2-L.x0)/(L.x1-L.x0)*s,s);return}
  if(enBarra){ver((q.x-L.bx0)/(L.bx1-L.bx0)*total-dB,Z.a-Z.de);return}
  if(!ini)return;if(!movio&&Math.abs(q.x-ini.x)<8)return;movio=true;ver(ini.de-(q.x-ini.x)/(L.x1-L.x0)*(Z.a-Z.de),Z.a-Z.de)});
 const fin=ev=>{const q=loc(ev);dedos.delete(ev.pointerId);if(pin){if(dedos.size<2)pin=null;ini=null;return}if(enBarra){enBarra=false;return}if(!ini)return;
  const fue=ini;ini=null;if(movio||q.y<L.y0-20||q.y>L.y1+20)return;
  if(ultToque&&Date.now()-ultToque.t<300&&Math.abs(q.x-ultToque.x)<30){clearTimeout(relojToque);relojToque=null;ultToque=null;acercar(q.x,2);return}
  ultToque={t:Date.now(),x:q.x};relojToque=setTimeout(()=>{relojToque=null;elegir(q.x)},280)};
 cv.addEventListener('pointerup',fin);cv.addEventListener('pointercancel',ev=>{dedos.delete(ev.pointerId);pin=null;ini=null;enBarra=false});
 cv.addEventListener('touchmove',ev=>ev.preventDefault(),{passive:false});
 // ---- doble toque sobre los gráficos del informe: se abren ----
 [['gVel','vel'],['gAce','ace']].forEach(([id,m])=>{let ult=null;$(id).addEventListener('pointerup',ev=>{const ahora=Date.now();
  if(ult&&ahora-ult.t<320&&Math.hypot(ev.clientX-ult.x,ev.clientY-ult.y)<30){ult=null;abrir(m,lap);return}ult={t:ahora,x:ev.clientX,y:ev.clientY}})});
 abrirZoom=(m,punto)=>{abrir(m,lap);if(punto!==undefined){Z.de=punto.de;Z.a=punto.a;recalcular();elegir(X(punto.d))}};   // para probar desde afuera (capturas)
})();
