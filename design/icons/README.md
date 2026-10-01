# 7-Zip ZS icon family

Original SVG artwork redrawn from the user's reference screenshot: straight-sided
colored sleeves, a full-height metallic zipper on the right, and a white label
projecting from the lower left. Backgrounds are transparent; the screenshot's
thumbnail frame and wallpaper are not part of the artwork. The monochrome
toolbar inspired by [NanaZip](https://github.com/M2Team/NanaZip) is unchanged.
The pipeline covers all 61 application-owned image resources:
34 archive ICOs, 9 application/installer/SFX ICOs, 14 toolbar BMPs, one shell menu
BMP and three package PNGs. The third-party DarkMode demo icon and Windows-owned
folder/drive/file icons are outside this set.

See [light preview](preview-light.png), [dark preview](preview-dark.png), and
[small-size inspection](preview-sizes.png).
`manifest.json` maps every source to its existing resource path; resource IDs and
archive index order are preserved. Brotli and Fast-LZMA2 codec resources now use
their own format icons. Bundled FM also embeds all 34 archive icons with explicit
extension mappings, preserving the original application/About indices 0/1. This
avoids the previous fallback to a single app icon when no external 7z.dll exists.

## Editing and regeneration

Edit the SVG masters in `src/`, then from this directory run:

```powershell
npm ci
npm run build
npm run check
pwsh -File verify-windows.ps1
```

After a native build, also check the compiled group-icon order and extension maps:

```powershell
pwsh -File verify-windows.ps1 -FileManager ../../CPP/7zip/Bundles/Fm/x64/7zFM.exe -ArchiveLibrary ../../CPP/7zip/Bundles/Format7zF/x64/7z.dll
```

The ordinary build is font-independent: SVG lettering is already outlined. It
uses pinned resvg to rasterize the masters and writes resources directly into
the existing C, CPP and Package paths. It skips writing unchanged files. The
check command compares every generated resource byte-for-byte without writing.
The Windows check exercises all 430 icon-size loads, DIB alpha, image-list
insertion, and coverage of tracked image resources.

`create-sources.mjs` is the optional geometry/palette bootstrap, not part of the
normal build. Running it **resets SVG edits and the manifest**. It requires
Segoe UI Regular (`C:/Windows/Fonts/segoeui.ttf`, or `ICON_FONT`). Preview-only
captions use system fonts; the icon artwork itself does not.

The fixed palette is 7Z `#8ED7F5`, ZIP `#FFE386`, RAR `#C348A2`, and ISO/WIM
`#C9CDD0`. Other formats mix the original palette with white at a 75:25 ratio;
the bootstrap retains the original inputs so repeated runs do not lighten again.
Applications share the 7Z sleeve with a `7-Zip` label and their existing action
badges. Label plate widths are measured from the outlined lettering.

## Small sizes and Windows integration

ICOs contain 16, 20, 24, 32, 40, 48, 64, 96, 128 and 256 pixel frames. Sizes
through 48 use straight-alpha DIBs and AND masks; larger frames use PNG to keep
resource size down. The 16px masters simplify geometry and abbreviate long
labels: LM=LZMA, L2=LZMA2, ZS=ZSTD, CP=CPIO, AP=APFS, NT=NTFS, SQ=SQFS.
The 16px application label is `7Z`. Separate `-small.svg` masters, selected via
`smallSource` in the manifest, enlarge the label and simplify zipper teeth at
20/24/32px without abbreviating the text. The remaining sizes use the regular
masters and full labels. Split archives use `001`.

Toolbar BMPs retain the original 48x36 and 24x24 dimensions. They now carry
premultiplied BGRA, are loaded as DIB sections, and are tinted to the current
system/dark-mode text color before insertion into the image list. The menu BMP
also uses premultiplied alpha. Package assets are checked in and the packaging
script reports missing assets instead of silently drawing the previous logo.

The NMAKE resource rule tracks image changes for incremental rebuilds. Normal
native builds consume the committed ICO/BMP/PNG files and do not need Node.
Rebuilding/replacing application binaries is required before installed copies
can show the new resources. This work does not change registry associations or
install/register a package.
