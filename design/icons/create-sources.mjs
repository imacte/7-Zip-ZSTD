// Design geometry. Run explicitly to reset SVG masters; build.mjs preserves edits.
// Labels are outlined once so normal resource builds require no installed fonts.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { Resvg } from '@resvg/resvg-js';

const dir = path.dirname(fileURLToPath(import.meta.url));
const fontFile = process.env.ICON_FONT || 'C:/Windows/Fonts/seguisb.ttf';
if (!fs.existsSync(fontFile)) throw new Error('Set ICON_FONT to a Segoe UI Semibold font file. Ordinary builds do not need it.');
const formats = [
  ['7z','7Z','#087bd9'], ['zip','ZIP','#b87808'], ['rar','RAR','#8253cd'],
  ['tar','TAR','#187f80'], ['gz','GZ','#27934e'], ['xz','XZ','#515bd1'],
  ['zst','ZST','#ce4c55'], ['zstd','ZSTD','#b93d56'], ['bz2','BZ2','#168b9c'],
  ['br','BR','#ae4a91'], ['lz4','LZ4','#2774b4'], ['lz5','LZ5','#4f68a8'],
  ['lzma','LZMA','#6f58b0'], ['lzma2','LZMA2','#9852b0'], ['liz','LIZ','#498042'],
  ['lzh','LZH','#9a713a'], ['lha','LHA','#a56845'], ['z','Z','#4b7e94'],
  ['arj','ARJ','#a85356'], ['cab','CAB','#a47728'], ['cpio','CPIO','#738333'],
  ['xar','XAR','#537562'], ['split','001','#737c8d'], ['deb','DEB','#b94363'],
  ['rpm','RPM','#b15639'], ['iso','ISO','#527b9b'], ['dmg','DMG','#6d76a5'],
  ['vhd','VHD','#326986'], ['wim','WIM','#267f9f'], ['apfs','APFS','#6074a0'],
  ['hfs','HFS','#8a6f94'], ['ntfs','NTFS','#526d80'], ['fat','FAT','#72835e'],
  ['sqfs','SQFS','#537b74']
];
const xml = s => s.replaceAll('&','&amp;').replaceAll('<','&lt;');
const wrap = (body,size=64) => `<svg xmlns="http://www.w3.org/2000/svg" width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">${body}</svg>`;
function outlined(svg) {
  return new Resvg(svg, {font:{loadSystemFonts:false,fontFiles:[fontFile]}}).toString().trimEnd()+'\n';
}
function write(name, svg) {
  const target=path.join(dir,'src',name+'.svg');
  fs.mkdirSync(path.dirname(target),{recursive:true});
  fs.writeFileSync(target,outlined(svg));
}
function book(label,color,variant='archive') {
  const size = label.length>4 ? 11 : label.length>3 ? 13 : 17;
  let body=`<defs><linearGradient id="cover" x1="0" y1="0" x2="0.8" y2="1"><stop stop-color="${color}"/><stop offset="1" stop-color="${color}"/></linearGradient><linearGradient id="light" x2="0.8" y2="1"><stop stop-color="white" stop-opacity=".17"/><stop offset="1" stop-color="white" stop-opacity="0"/></linearGradient></defs>
    <rect x="10" y="6" width="44" height="54" rx="7" fill="${color}"/>
    <rect x="10" y="6" width="44" height="54" rx="7" fill="black" opacity=".22"/>
    <rect x="10" y="4" width="44" height="52" rx="7" fill="url(#cover)"/>
    <rect x="10" y="4" width="44" height="52" rx="7" fill="url(#light)"/>
    <path d="M17 5V55" stroke="white" stroke-opacity=".13"/>
    <path d="M23 5V25" stroke="black" stroke-opacity=".18" stroke-width="2"/>`;
  for(let y=7;y<23;y+=5) body+=`<path d="M20 ${y}h4m-1 2.5h4" fill="none" stroke="white" stroke-opacity=".96" stroke-width="2"/>`;
  body+=`<rect x="20" y="25" width="7" height="9" rx="2" fill="white"/><rect x="22" y="28" width="3" height="3" rx=".7" fill="${color}"/>`;
  if(variant==='archive') body+=`<text x="33" y="49" text-anchor="middle" font-family="Segoe UI" font-weight="600" font-size="${size}" fill="white">${xml(label)}</text>`;
  else {
    body+=`<path d="M31 18H46V22L37 44H31L40 23H31Z" fill="white"/>`;
    if(variant!=='app') {
      const badge=variant==='uninstall'?'#c74248':variant==='sfx'?'#148263':'#0869bc';
      body+=`<circle cx="49" cy="49" r="12" fill="white"/><circle cx="49" cy="49" r="10" fill="${badge}"/>`;
      body+= variant==='uninstall' ? `<path d="M45 45l8 8m0-8l-8 8" stroke="white" stroke-width="2.5" stroke-linecap="round"/>` : variant==='sfx' ? `<path d="M45 43l9 6-9 6Z" fill="white"/>` : `<path d="M49 42v12m-5-4 5 5 5-5" fill="none" stroke="white" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/>`;
    }
  }
  return wrap(body);
}
function micro(label,color,variant='archive') {
  const short={LZMA:'LM',LZMA2:'L2',ZSTD:'ZS',CPIO:'CP',APFS:'AP',NTFS:'NT',SQFS:'SQ'}[label] || label;
  let b=`<rect x="2" y="1" width="12" height="14" rx="2" fill="${color}"/><path d="M3 13.5h10" stroke="black" stroke-opacity=".2"/><path d="M5 2h2m-1 2h2m-3 2h2" stroke="white" stroke-width="1"/>`;
  b+=variant==='archive'?`<text x="8" y="12" text-anchor="middle" font-family="Segoe UI" font-weight="600" font-size="${short.length>2?5:6}" fill="white">${short}</text>`:`<path d="M8 5h4v1.5L9 12H7.5l3-5.5H8Z" fill="white"/>`;
  if(variant!=='archive'&&variant!=='app') b+=`<circle cx="12" cy="12" r="3.7" fill="${variant==='uninstall'?'#c74248':'#0869bc'}" stroke="white" stroke-width=".8"/>`+(variant==='uninstall'?`<path d="m10.6 10.6 2.8 2.8m0-2.8-2.8 2.8" stroke="white" stroke-width="1"/>`:variant==='sfx'?`<path d="m11 10 3 2-3 2Z" fill="white"/>`:`<path d="M12 9.8v4m-1.5-1.5 1.5 1.5 1.5-1.5" stroke="white" fill="none"/>`);
  return wrap(b,16);
}
const tools={
  Add:'<path d="M12 4v16M4 12h16"/>',
  Extract:'<path d="M3 8V6a2 2 0 0 1 2-2h4l2 3h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8h14"/><path d="M10 14h7m-3-3 3 3-3 3"/>',
  Test:'<path d="m4 12 5 5L20 6"/>',
  Copy:'<rect x="8" y="3" width="12" height="14" rx="2"/><path d="M5 7H4a1 1 0 0 0-1 1v11a2 2 0 0 0 2 2h9a2 2 0 0 0 2-2"/>',
  Move:'<path d="M10 5H5a2 2 0 0 0-2 2v10a2 2 0 0 0 2 2h5M10 12h11m-4-4 4 4-4 4"/>',
  Delete:'<path d="M3 6h18M9 6V4h6v2M5 6l1 14h12l1-14M10 10v6m4-6v6"/>',
  Info:'<circle cx="12" cy="12" r="9"/><path d="M12 11v6"/><circle cx="12" cy="7.5" r=".9" fill="#263445" stroke="none"/>'
};
const manifest={sizes:[16,20,24,32,40,48,64,96,128,256],formats:[],applications:[],toolbars:[],package:[]};
for(const [name,label,color] of formats) {
  const source=`archive/${name}`;
  write(source,book(label,color)); write(source+'-16',micro(label,color));
  manifest.formats.push({name,label,color,source,target:`CPP/7zip/Archive/Icons/${name}.ico`});
}
const apps=[
  ['app',['CPP/7zip/UI/FileManager/FM.ico','CPP/7zip/UI/GUI/FM.ico','CPP/7zip/UI/FileManager/7zipLogo.ico']],
  ['install',['C/Util/7zipInstall/7zip.ico','C/Util/SfxSetup/setup.ico','CPP/7zip/Bundles/SFXSetup/setup.ico']],
  ['uninstall',['C/Util/7zipUninstall/7zipUninstall.ico']],
  ['sfx',['CPP/7zip/Bundles/SFXWin/7z.ico','CPP/7zip/Bundles/SFXCon/7z.ico']]
];
for(const [name,targets] of apps) {
  const source=`app/${name}`;
  write(source,book('7','#087bd9',name));write(source+'-16',micro('7','#087bd9',name));
  manifest.applications.push({name,source,targets});
}
for(const [name,geometry] of Object.entries(tools)) {
  const source=`toolbar/${name.toLowerCase()}`;
  write(source,wrap(`<g fill="none" stroke="#263445" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round">${geometry}</g>`,24));
  manifest.toolbars.push({name,source,targets:[{path:`CPP/7zip/UI/FileManager/${name}.bmp`,width:48,height:36,glyphSize:28},{path:`CPP/7zip/UI/FileManager/${name}2.bmp`,width:24,height:24,glyphSize:22}]});
}
for(const [name,size] of [['StoreLogo',50],['Square44x44Logo',44],['Square150x150Logo',150]]) manifest.package.push({target:`Package/Assets/${name}.png`,size});
manifest.menu={target:'CPP/7zip/UI/Explorer/MenuLogo.bmp',size:16};
manifest.excluded=['DarkMode/lib/dmlib_demo/demo.ico'];
fs.writeFileSync(path.join(dir,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log('Created 83 font-independent SVG masters and resource manifest.');
