// Native-size sleeves with real semibold typography, not a hand-made pixel font.
import { Resvg } from '@resvg/resvg-js';

export function smallIcon(label,color,variant,size,fontFile) {
  const short={LZMA:'LM',LZMA2:'L2',ZSTD:'ZS',CPIO:'CP',APFS:'AP',NTFS:'NT',SQFS:'SQ'};
  if(size===16) label=variant==='archive'?(short[label] || label):'7Z';
  const wrap=body=>`<svg xmlns="http://www.w3.org/2000/svg" width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">${body}</svg>`;
  const text=fontSize=>`<text y="${fontSize}" font-family="Segoe UI" font-weight="600" font-size="${fontSize}">${label}</text>`;
  // Measure on a roomy canvas so long labels are not clipped before fitting.
  const bounds=fontSize=>new Resvg(`<svg xmlns="http://www.w3.org/2000/svg" width="256" height="64">${text(fontSize)}</svg>`,{font:{loadSystemFonts:false,fontFiles:[fontFile]}}).innerBBox();
  let fontSize={16:8,20:9,24:10,32:12}[size];
  let box=bounds(fontSize);
  fontSize*=Math.min(1,(size-2)/box.width);
  box=bounds(fontSize);
  const plateWidth=Math.min(size,Math.ceil(box.width)+2),plateHeight=Math.ceil(box.height)+2;
  let b='';
  const rect=(x,y,w,h,fill)=>{b+=`<rect x="${x}" y="${y}" width="${w}" height="${h}" fill="${fill}"/>`;};
  const left=Math.round(size*.25),right=size-1,top=1,bottom=size-1;
  rect(left,top,right-left,bottom-top,color);
  rect(right-1,top,1,bottom-top,'#00000018');
  const zipper=right-(size>=24?5:3),zw=size>=24?3:2;
  rect(zipper,top,zw,bottom-top,'#82908b');
  rect(zipper,top,1,bottom-top,'#d8dedb');
  // At this scale a quiet metal track reads better than a dense checkerboard.
  for(let y=top+5;y<bottom;y+=3) rect(zipper+zw-1,y,1,1,'#e8eeeb');
  rect(zipper,top,zw,3,'#cbd3d0');
  rect(zipper,top,1,2,'#f1f4f2');
  rect(zipper+zw-1,top+2,1,2,'#687970');
  const plateY=bottom-plateHeight-(size>=24?2:0);
  rect(0,plateY,plateWidth,plateHeight,'#c1c8cc');
  rect(0,plateY,plateWidth-1,plateHeight-1,'#fff');
  b+=`<g fill="#27333b" transform="translate(${1-box.x} ${plateY+1-box.y})">${text(fontSize)}</g>`;
  if(variant!=='archive'&&variant!=='app') {
    const radius=size===16?3:4,c=size-radius;
    b+=`<circle cx="${c}" cy="${c}" r="${radius}" fill="${variant==='uninstall'?'#b42f39':variant==='sfx'?'#11745a':'#075ba4'}" stroke="white" stroke-width="1"/>`;
    if(variant==='uninstall') b+=`<path d="m${c-1} ${c-1} 2 2m0-2-2 2" stroke="white"/>`;
    else if(variant==='sfx') b+=`<path d="M${c-1} ${c-1.5}l2.5 1.5-2.5 1.5Z" fill="white"/>`;
    else b+=`<path d="M${c} ${c-2}v4m-1.5-1.5 1.5 1.5 1.5-1.5" fill="none" stroke="white"/>`;
  }
  return wrap(b);
}
