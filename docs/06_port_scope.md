# 06 - Portierungsumfang: was ein 8-Bit-Treiber braucht

Quelle: `https://api.github.com/repos/mamedev/mame/git/trees/mame0220?recursive=1` (gemessene Blob-Groessen aus dem Git-Baum)
Erzeugt: 2026-11-09T07:15:24Z

## Verzeichnisse

| Verzeichnis | Dateien | C/C++ | MB | C/C++ MB | Einschaetzung |
|---|---:|---:|---:|---:|---|
| `src/emu` | 229 | 212 | 4.0 | 3.7 | vorgesehen |
| `src/devices/cpu/m6502` | 105 | 76 | 0.4 | 0.2 | vorgesehen |
| `src/devices/cpu` | 1194 | 1078 | 22.9 | 18.7 | vorgesehen |
| `src/devices/machine` | 801 | 799 | 7.4 | 7.3 | vorgesehen |
| `src/devices/sound` | 311 | 301 | 3.4 | 3.2 | vorgesehen |
| `src/devices/video` | 240 | 236 | 3.7 | 3.5 | vorgesehen |
| `src/devices/bus` | 2500 | 2497 | 12.4 | 12.3 | vorgesehen |
| `src/devices/imagedev` | 32 | 32 | 0.2 | 0.2 | vorgesehen |
| `src/lib/util` | 91 | 91 | 1.2 | 1.2 | vorgesehen |
| `src/lib` | 694 | 680 | 4.0 | 3.8 | vorgesehen |
| `src/osd` | 569 | 332 | 2.8 | 2.1 | muss ersetzt werden |
| `plugins` | 50 | 0 | 0.5 | 0.0 | vorgesehen |
| `3rdparty/lua` | 74 | 61 | 1.0 | 0.7 | vorgesehen |
| `3rdparty/sol2` | 189 | 106 | 2.8 | 1.1 | vorgesehen |
| `3rdparty/luasocket` | 0 | 0 | 0.0 | 0.0 | vorgesehen |
| `3rdparty/luafilesystem` | 37 | 2 | 0.1 | 0.0 | vorgesehen |
| `3rdparty/softfloat3` | 452 | 437 | 1.7 | 1.6 | vorgesehen |
| `3rdparty/softfloat` | 10 | 7 | 0.3 | 0.2 | vorgesehen |
| `3rdparty/zlib` | 253 | 74 | 2.8 | 1.1 | vorgesehen |
| `3rdparty/expat` | 141 | 55 | 2.1 | 0.6 | vorgesehen |
| `3rdparty/pugixml` | 158 | 74 | 2.2 | 0.9 | vorgesehen |
| `3rdparty/rapidjson` | 158 | 82 | 2.0 | 1.0 | vorgesehen |
| `3rdparty/utf8proc` | 33 | 18 | 1.7 | 1.6 | vorgesehen |
| `3rdparty/linenoise` | 7 | 5 | 0.1 | 0.1 | vorgesehen |
| `3rdparty/lsqlite3` | 21 | 2 | 0.2 | 0.1 | vorgesehen |
| `3rdparty/lua-zlib` | 13 | 1 | 0.0 | 0.0 | vorgesehen |
| `3rdparty/libflac` | 916 | 194 | 10.6 | 2.7 | **entfaellt voraussichtlich** |
| `3rdparty/portaudio` | 347 | 170 | 6.5 | 3.5 | **entfaellt voraussichtlich** |
| `3rdparty/portmidi` | 138 | 44 | 2.1 | 0.6 | **entfaellt voraussichtlich** |
| `3rdparty/bgfx` | 9478 | 1566 | 185.6 | 45.9 | **entfaellt voraussichtlich** |
| `3rdparty/bimg` | 316 | 209 | 6.7 | 4.2 | **entfaellt voraussichtlich** |
| `3rdparty/bx` | 193 | 134 | 4.8 | 1.5 | **entfaellt voraussichtlich** |
| `3rdparty/glm` | 857 | 286 | 12.7 | 1.4 | **entfaellt voraussichtlich** |
| `3rdparty/SDL2` | 1087 | 745 | 59.2 | 8.7 | **entfaellt voraussichtlich** |
| `3rdparty/asio` | 995 | 730 | 8.6 | 4.3 | **entfaellt voraussichtlich** |
| `3rdparty/lzma` | 611 | 487 | 2.8 | 2.3 | **entfaellt voraussichtlich** |
| `3rdparty/nanosvg` | 13 | 5 | 0.4 | 0.1 | **entfaellt voraussichtlich** |

## Entfallen kann (Artwork/Treiber ausserhalb des Ziels)

* Layout-/Artwork-Dateien (`.lay`): **1495 Dateien, 165.2 MB**
* Treiberdateien gesamt: 2513 Dateien, 96.1 MB — gebraucht wird davon **eine**

## `src/devices/cpu/m6502/` (CPU des mm4)

```
src/devices/cpu/m6502/ddeco16.lst
src/devices/cpu/m6502/deco16.cpp
src/devices/cpu/m6502/deco16.h
src/devices/cpu/m6502/deco16d.cpp
src/devices/cpu/m6502/deco16d.h
src/devices/cpu/m6502/dm4510.lst
src/devices/cpu/m6502/dm6502.lst
src/devices/cpu/m6502/dm6509.lst
src/devices/cpu/m6502/dm6510.lst
src/devices/cpu/m6502/dm65c02.lst
src/devices/cpu/m6502/dm65ce02.lst
src/devices/cpu/m6502/dm740.lst
src/devices/cpu/m6502/dn2a03.lst
src/devices/cpu/m6502/dr65c02.lst
src/devices/cpu/m6502/dr65c19.lst
src/devices/cpu/m6502/dst2xxx.lst
src/devices/cpu/m6502/dxavix.lst
src/devices/cpu/m6502/dxavix2000.lst
src/devices/cpu/m6502/m3745x.cpp
src/devices/cpu/m6502/m3745x.h
src/devices/cpu/m6502/m4510.cpp
src/devices/cpu/m6502/m4510.h
src/devices/cpu/m6502/m4510d.cpp
src/devices/cpu/m6502/m4510d.h
src/devices/cpu/m6502/m5074x.cpp
src/devices/cpu/m6502/m5074x.h
src/devices/cpu/m6502/m6500_1.cpp
src/devices/cpu/m6502/m6500_1.h
src/devices/cpu/m6502/m6502.cpp
src/devices/cpu/m6502/m6502.h
src/devices/cpu/m6502/m6502.txt
src/devices/cpu/m6502/m6502d.cpp
src/devices/cpu/m6502/m6502d.h
src/devices/cpu/m6502/m6502make.py
src/devices/cpu/m6502/m6504.cpp
src/devices/cpu/m6502/m6504.h
src/devices/cpu/m6502/m6507.cpp
src/devices/cpu/m6502/m6507.h
src/devices/cpu/m6502/m6509.cpp
src/devices/cpu/m6502/m6509.h
src/devices/cpu/m6502/m6509d.cpp
src/devices/cpu/m6502/m6509d.h
src/devices/cpu/m6502/m6510.cpp
src/devices/cpu/m6502/m6510.h
src/devices/cpu/m6502/m6510d.cpp
src/devices/cpu/m6502/m6510d.h
src/devices/cpu/m6502/m6510t.cpp
src/devices/cpu/m6502/m6510t.h
src/devices/cpu/m6502/m65c02.cpp
src/devices/cpu/m6502/m65c02.h
src/devices/cpu/m6502/m65c02d.cpp
src/devices/cpu/m6502/m65c02d.h
src/devices/cpu/m6502/m65ce02.cpp
src/devices/cpu/m6502/m65ce02.h
src/devices/cpu/m6502/m65ce02d.cpp
src/devices/cpu/m6502/m65ce02d.h
src/devices/cpu/m6502/m65sc02.cpp
src/devices/cpu/m6502/m65sc02.h
src/devices/cpu/m6502/m740.cpp
src/devices/cpu/m6502/m740.h
src/devices/cpu/m6502/m740d.cpp
src/devices/cpu/m6502/m740d.h
src/devices/cpu/m6502/m7501.cpp
src/devices/cpu/m6502/m7501.h
src/devices/cpu/m6502/m8502.cpp
src/devices/cpu/m6502/m8502.h
src/devices/cpu/m6502/n2a03.cpp
src/devices/cpu/m6502/n2a03.h
src/devices/cpu/m6502/n2a03d.cpp
src/devices/cpu/m6502/n2a03d.h
src/devices/cpu/m6502/odeco16.lst
src/devices/cpu/m6502/om4510.lst
src/devices/cpu/m6502/om6502.lst
src/devices/cpu/m6502/om6509.lst
src/devices/cpu/m6502/om6510.lst
src/devices/cpu/m6502/om65c02.lst
src/devices/cpu/m6502/om65ce02.lst
src/devices/cpu/m6502/om740.lst
src/devices/cpu/m6502/on2a03.lst
src/devices/cpu/m6502/or65c19.lst
src/devices/cpu/m6502/ost2xxx.lst
src/devices/cpu/m6502/oxavix.lst
src/devices/cpu/m6502/oxavix2000.lst
src/devices/cpu/m6502/r65c02.cpp
src/devices/cpu/m6502/r65c02.h
src/devices/cpu/m6502/r65c02d.cpp
src/devices/cpu/m6502/r65c02d.h
src/devices/cpu/m6502/r65c19.cpp
src/devices/cpu/m6502/r65c19.h
src/devices/cpu/m6502/r65c19d.cpp
src/devices/cpu/m6502/r65c19d.h
src/devices/cpu/m6502/st2204.cpp
src/devices/cpu/m6502/st2204.h
src/devices/cpu/m6502/st2205u.cpp
src/devices/cpu/m6502/st2205u.h
src/devices/cpu/m6502/st2xxx.cpp
src/devices/cpu/m6502/st2xxx.h
src/devices/cpu/m6502/xavix.cpp
src/devices/cpu/m6502/xavix.h
src/devices/cpu/m6502/xavix2000.cpp
src/devices/cpu/m6502/xavix2000.h
src/devices/cpu/m6502/xavix2000d.cpp
src/devices/cpu/m6502/xavix2000d.h
src/devices/cpu/m6502/xavixd.cpp
src/devices/cpu/m6502/xavixd.h
```

## Einordnung

Die Zahlen belegen: der *Treiber* ist winzig, der *Rahmen* ist es nicht.
Ein Herauslösen (CPU + Gerätesatz + Lua-Schnittstelle) ist damit eine
berechenbare Option, aber ausdruecklich eine **Abweichung** vom vollen
Portierungsansatz und nicht als gleichwertig darzustellen.
