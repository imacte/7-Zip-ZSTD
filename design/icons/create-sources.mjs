// Design geometry. Run explicitly to reset SVG masters; build.mjs preserves edits.
// Labels are outlined once so normal resource builds require no installed fonts.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { Resvg } from '@resvg/resvg-js';
import { smallIcon } from './small-icons.mjs';

const dir = path.dirname(fileURLToPath(import.meta.url));
const fontFile = process.env.ICON_FONT || 'C:/Windows/Fonts/segoeui.ttf';
const smallFontFile = process.env.ICON_SMALL_FONT || 'C:/Windows/Fonts/seguisb.ttf';
if (!fs.existsSync(fontFile)) throw new Error('Set ICON_FONT to a Segoe UI font file. Ordinary builds do not need it.');
// Keep the original palette as the input so regeneration never lightens twice.
const baseFormats = [
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
function mix(color,other,weight) {
  return '#'+[1,3,5].map(i=>Math.round(parseInt(color.slice(i,i+2),16)*(1-weight)+parseInt(other.slice(i,i+2),16)*weight).toString(16).padStart(2,'0')).join('');
}
const overrides={'7z':'#8ED7F5',zip:'#FFE386',rar:'#C348A2',iso:'#C9CDD0',wim:'#C9CDD0'};
const formats=baseFormats.map(([name,label,color])=>[name,label,overrides[name] || mix(color,'#ffffff',.25)]);
const xml = s => s.replaceAll('&','&amp;').replaceAll('<','&lt;');
const wrap = (body,size=64) => `<svg xmlns="http://www.w3.org/2000/svg" width="${size}" height="${size}" viewBox="0 0 ${size} ${size}">${body}</svg>`;
function outlined(svg) {
  return new Resvg(svg, {font:{loadSystemFonts:false,fontFiles:[fontFile,smallFontFile]}}).toString().trimEnd()+'\n';
}
function write(name, svg) {
  const target=path.join(dir,'src',name+'.svg');
  fs.mkdirSync(path.dirname(target),{recursive:true});
  fs.writeFileSync(target,outlined(svg));
}
function labelPlate(label,x,y,maxWidth,height,fontSize) {
  const lettering=size=>`<text y="${size}" font-family="Segoe UI" font-weight="400" font-size="${size}">${xml(label)}</text>`;
  const options={font:{loadSystemFonts:false,fontFiles:[fontFile]}};
  let bounds=new Resvg(wrap(lettering(fontSize)),options).innerBBox();
  const padding=height*.19;
  fontSize*=Math.min(1,(maxWidth-padding*2)/bounds.width);
  bounds=new Resvg(wrap(lettering(fontSize)),options).innerBBox();
  const width=bounds.width+padding*2;
  return `<rect x="${x+.3}" y="${y+.7}" width="${width}" height="${height}" fill="#000" opacity=".1"/>
    <rect x="${x}" y="${y}" width="${width}" height="${height}" fill="#fff" stroke="#c8ccce" stroke-width="${height<8?.4:.6}"/>
    <g fill="#52595c" transform="translate(${x+padding-bounds.x} ${y+(height-bounds.height)/2-bounds.y})">${lettering(fontSize)}</g>`;
}
function actionBadge(variant) {
  if(variant==='app'||variant==='archive') return '';
  const color=variant==='uninstall'?'#c74248':variant==='sfx'?'#148263':'#0869bc';
  return `<circle cx="53" cy="53" r="9.5" fill="white"/><circle cx="53" cy="53" r="8" fill="${color}"/>`+
    (variant==='uninstall'?`<path d="m50 50 6 6m0-6-6 6" stroke="white" stroke-width="2" stroke-linecap="round"/>`:
      variant==='sfx'?`<path d="m50.5 48.5 6.5 4.5-6.5 4.5Z" fill="white"/>`:
      `<path d="M53 48v9m-3.5-3.5L53 57l3.5-3.5" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>`);
}
function book(label,color,variant='archive',small=false) {
  let body=`<defs>
    <linearGradient id="cover" x1="0" y1="0" x2="1" y2=".4"><stop stop-color="${mix(color,'#ffffff',.08)}"/><stop offset=".7" stop-color="${color}"/><stop offset="1" stop-color="${mix(color,'#000000',.06)}"/></linearGradient>
    <linearGradient id="metal"><stop stop-color="#eef0ee"/><stop offset=".22" stop-color="#b4bcba"/><stop offset=".48" stop-color="#f2f3ef"/><stop offset=".68" stop-color="#b9c0bc"/><stop offset="1" stop-color="#788480"/></linearGradient>
    <linearGradient id="teeth"><stop stop-color="#e1e6e3"/><stop offset=".4" stop-color="#aeb8b4"/><stop offset="1" stop-color="#65736d"/></linearGradient>
    </defs>
    <path d="M15 5h43v55H15Z" fill="#000" opacity=".12"/>
    <path d="M15 4h43v55H15Z" fill="url(#cover)"/>
    <path d="M54 4h4v55h-4Z" fill="${mix(color,'#000000',.12)}"/>
    <path d="M15 4h39" stroke="#fff" stroke-opacity=".3" stroke-width=".7"/>
    <path d="M54 37l5 5v17h-5Z" fill="${mix(color,'#000000',.04)}"/>
    <path d="M54 37v22" stroke="#fff" stroke-opacity=".3" stroke-width=".6"/>
    <path d="M41.5 4h7v55h-7Z" fill="#63716c"/>
    <path d="M41.5 4h1v55h-1ZM47.5 4h1v55h-1Z" fill="#c6cfca"/>`;
  const step=small?4:2.7;
  for(let y=18;y<58;y+=step) body+=`<path d="M42 ${y}h3.7v${step*.48}H42ZM44.3 ${y+step*.5}H48v${step*.46}h-3.7Z" fill="url(#teeth)"/><path d="M42 ${y}h3.7m-1.4 ${step*.5}H48" stroke="#edf0e9" stroke-opacity=".8" stroke-width=".45"/>`;
  body+=`<path d="M41.5 4h7l-.5 8-1.2 2v5.5h-3.6V14l-1.2-2Z" fill="#000" opacity=".15" transform="translate(.6 .6)"/>
    <path d="M41.5 4h7l-.5 8-1.2 2v5.5h-3.6V14l-1.2-2Z" fill="url(#metal)"/>
    <path d="M43 5.5h3.8v2H43Z" fill="#6e7a74"/>
    <path d="M44.2 12.5h1.5v4.8h-1.5Z" fill="#82908a"/>
    <path d="M43 4v6" stroke="#fff" stroke-opacity=".65" stroke-width=".6"/>`;
  body+=labelPlate(label,3,small?32:35,small?47:45,small?19:14,small?18:14);
  body+=actionBadge(variant);
  return wrap(body);
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
  write(source,book(label,color));
  for(const size of [16,20,24,32]) write(source+(size===32?'-small':'-'+size),smallIcon(label,color,'archive',size,smallFontFile));
  manifest.formats.push({name,label,color,source,smallSource:source+'-small',sizeSources:{16:source+'-16',20:source+'-20',24:source+'-24',32:source+'-small'},target:`CPP/7zip/Archive/Icons/${name}.ico`});
}
const apps=[
  ['app',['CPP/7zip/UI/FileManager/FM.ico','CPP/7zip/UI/GUI/FM.ico','CPP/7zip/UI/FileManager/7zipLogo.ico']],
  ['install',['C/Util/7zipInstall/7zip.ico','C/Util/SfxSetup/setup.ico','CPP/7zip/Bundles/SFXSetup/setup.ico']],
  ['uninstall',['C/Util/7zipUninstall/7zipUninstall.ico']],
  ['sfx',['CPP/7zip/Bundles/SFXWin/7z.ico','CPP/7zip/Bundles/SFXCon/7z.ico']]
];
for(const [name,targets] of apps) {
  const source=`app/${name}`;
  write(source,book('7-ZIP',overrides['7z'],name));
  for(const size of [16,20,24,32]) write(source+(size===32?'-small':'-'+size),smallIcon('7-ZIP',overrides['7z'],name,size,smallFontFile));
  manifest.applications.push({name,source,smallSource:source+'-small',sizeSources:{16:source+'-16',20:source+'-20',24:source+'-24',32:source+'-small'},targets});
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
console.log('Created 197 font-independent SVG masters and resource manifest.');
