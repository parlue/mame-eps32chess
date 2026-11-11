# MAME 0.220 als ESP-IDF-Komponente
#
# Quellen bleiben unveraendert unter mame-s3/ref/mame0220-src (Clone des Tags
# mame0220). Hier wird nur ausgewaehlt, was gebaut wird, und mit den noetigen
# Compiler-Schaltern versehen.
#
# Meilenstein 0 (dieser Stand): abhaengigkeitsarme Utilities aus src/lib/util
# kompilieren -> beweist Werkzeugkette, Include-Layout und Schalter.
# Meilenstein 1: src/emu (Kern) + CPU-Device m65c02 + Treiber mephisto_mm2.
# Meilenstein 2: OSD-Ersatzschicht (Datei/Zeit/Ausgabe) + ROM aus Flash.

set(MAME_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../ref/mame0220-src")

set(MAME_EMU_ROOT "${MAME_ROOT}/src/emu")
option(MAME_S3_COMPACT_DISPATCH "Use compact 32-bit dispatch segmentation" ON)
if(MAME_S3_COMPACT_DISPATCH)
    find_program(MAME_COMPACT_PYTHON NAMES python python3)
    if(NOT MAME_COMPACT_PYTHON)
        message(FATAL_ERROR "Python required for compact core view")
    endif()
    set(MAME_EMU_ROOT "${CMAKE_CURRENT_LIST_DIR}/generated/compact-emu")
    execute_process(
        COMMAND "${MAME_COMPACT_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_compact_emu.py"
            "${MAME_ROOT}/src/emu" "${MAME_EMU_ROOT}"
        RESULT_VARIABLE MAME_COMPACT_RESULT)
    if(NOT MAME_COMPACT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to prepare compact core view")
    endif()
endif()


# Keep upstream sources untouched. MAME 0.220 omits ownership cleanup of the
# read/write dispatch roots; repeated machines otherwise retain megabytes.
set(MAME_MEMORY_OVERLAY "${CMAKE_CURRENT_LIST_DIR}/generated/emumem_s3.cpp")
file(READ "${MAME_EMU_ROOT}/emumem.cpp" MAME_MEMORY_SOURCE)
set(MAME_MEMORY_ANCHOR "\tvoid *create_cache() override {")
string(FIND "${MAME_MEMORY_SOURCE}" "~address_space_specific()" MAME_MEMORY_DESTRUCTOR)
string(FIND "${MAME_MEMORY_SOURCE}" "${MAME_MEMORY_ANCHOR}" MAME_MEMORY_POSITION)
if(NOT MAME_MEMORY_DESTRUCTOR EQUAL -1 OR MAME_MEMORY_POSITION EQUAL -1)
    message(FATAL_ERROR "Unexpected emumem.cpp: review S3 ownership overlay")
endif()
string(REPLACE "${MAME_MEMORY_ANCHOR}"
    "\t~address_space_specific() override {\n\t\tm_root_read->unref();\n\t\tm_root_write->unref();\n\t\tfor (handler_entry *entry : m_delayed_unrefs)\n\t\t\tentry->unref();\n\t}\n\n${MAME_MEMORY_ANCHOR}"
    MAME_MEMORY_SOURCE "${MAME_MEMORY_SOURCE}")
set(MAME_MEMORY_PREVIOUS "")
if(EXISTS "${MAME_MEMORY_OVERLAY}")
    file(READ "${MAME_MEMORY_OVERLAY}" MAME_MEMORY_PREVIOUS)
endif()
if(NOT MAME_MEMORY_PREVIOUS STREQUAL MAME_MEMORY_SOURCE)
    file(WRITE "${MAME_MEMORY_OVERLAY}" "${MAME_MEMORY_SOURCE}")
endif()

# Current drivers all use a native 8-bit little-endian bus. Keep the full
# upstream allocator selectable for future CPU families and comparisons.
option(MAME_S3_65XX_MEMORY "Use native 8-bit LE memory dispatch profile" ON)
if(MAME_S3_65XX_MEMORY)
    find_program(MAME_PROFILE_PYTHON NAMES python python3 REQUIRED)
    execute_process(
        COMMAND "${MAME_PROFILE_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_65xx_memory.py"
            "${MAME_MEMORY_OVERLAY}"
        RESULT_VARIABLE MAME_PROFILE_RESULT)
    if(NOT MAME_PROFILE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to prepare the 65xx memory profile")
    endif()
endif()

option(MAME_S3_65XX_ROOTS "Keep only 16/32-bit address roots in native-8 chess profile" ON)
if(MAME_S3_65XX_ROOTS)
    if(NOT MAME_S3_65XX_MEMORY)
        message(FATAL_ERROR "MAME_S3_65XX_ROOTS requires MAME_S3_65XX_MEMORY; disable both for generic buses")
    endif()
    set(MAME_MEMORY_ROOTS "${CMAKE_CURRENT_LIST_DIR}/generated/emumem_roots_s3.cpp")
    execute_process(COMMAND "${MAME_PROFILE_PYTHON}"
        "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_65xx_roots.py"
        "${MAME_MEMORY_OVERLAY}" "${MAME_MEMORY_ROOTS}"
        RESULT_VARIABLE MAME_ROOTS_RESULT)
    if(NOT MAME_ROOTS_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to prepare 16/32-bit chess address roots")
    endif()
    set(MAME_MEMORY_OVERLAY "${MAME_MEMORY_ROOTS}")
endif()

# The 6502 interface also owns two allocated access caches. Its upstream
# destructor is empty. Use a local owning subtype, preserving the public
# device headers and the untouched upstream checkout.
set(MAME_6502_OVERLAY "${CMAKE_CURRENT_LIST_DIR}/generated/m6502_s3.cpp")
file(READ "${MAME_ROOT}/src/devices/cpu/m6502/m6502.cpp" MAME_6502_SOURCE)
string(REPLACE "\"m6502.h\"" "\"cpu/m6502/m6502.h\"" MAME_6502_SOURCE "${MAME_6502_SOURCE}")
string(REPLACE "\"m6502d.h\"" "\"cpu/m6502/m6502d.h\"" MAME_6502_SOURCE "${MAME_6502_SOURCE}")
set(MAME_6502_ANCHOR "mintf = std::make_unique<mi_default>();")
string(FIND "${MAME_6502_SOURCE}" "${MAME_6502_ANCHOR}" MAME_6502_POSITION)
if(MAME_6502_POSITION EQUAL -1)
    message(FATAL_ERROR "Unexpected m6502.cpp: review S3 cache ownership overlay")
endif()
string(REPLACE "${MAME_6502_ANCHOR}"
    "class owned_memory_interface final : public mi_default {\n\tpublic:\n\t\towned_memory_interface() { cache = nullptr; scache = nullptr; }\n\t\t~owned_memory_interface() override { delete cache; delete scache; }\n\t};\n\tmintf = std::make_unique<owned_memory_interface>();"
    MAME_6502_SOURCE "${MAME_6502_SOURCE}")
set(MAME_6502_PREVIOUS "")
if(EXISTS "${MAME_6502_OVERLAY}")
    file(READ "${MAME_6502_OVERLAY}" MAME_6502_PREVIOUS)
endif()
if(NOT MAME_6502_PREVIOUS STREQUAL MAME_6502_SOURCE)
    file(WRITE "${MAME_6502_OVERLAY}" "${MAME_6502_SOURCE}")
endif()
find_program(MAME_SPEED_PYTHON NAMES python python3 REQUIRED)
set(MAME_6502_SPEED_OVERLAY "${CMAKE_CURRENT_LIST_DIR}/generated/m6502_speed_s3.cpp")
execute_process(COMMAND "${MAME_SPEED_PYTHON}"
    "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_65xx_speed.py"
    "${MAME_6502_OVERLAY}" "${MAME_6502_SPEED_OVERLAY}"
    RESULT_VARIABLE MAME_SPEED_RESULT)
if(NOT MAME_SPEED_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to prepare 6502 speed comparison")
endif()
set(MAME_6502_OVERLAY "${MAME_6502_SPEED_OVERLAY}")
set(MAME_MEPHISTO_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/mephisto_internal_s3.cpp")
execute_process(COMMAND "${MAME_SPEED_PYTHON}"
    "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_mephisto_internal_cpu.py"
    "${MAME_ROOT}/src/mame/drivers/mephisto_mm2.cpp" "${MAME_MEPHISTO_SOURCE}"
    RESULT_VARIABLE MAME_MEPHISTO_RESULT)
if(NOT MAME_MEPHISTO_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to prepare internal Mephisto CPU allocation")
endif()

if(NOT EXISTS "${MAME_EMU_ROOT}/emu.h")
    message(FATAL_ERROR
        "MAME-0.220-Quellen fehlen unter '${MAME_ROOT}'.\n"
        "Bitte zuerst holen:\n"
        "  git clone --depth 1 --branch mame0220 --single-branch "
        "https://github.com/mamedev/mame.git mame-s3/ref/mame0220-src")
endif()

# Meilenstein 0: abhaengigkeitsarme Originaldateien (Namen gegen den echten
# 0.220-Baum geprueft). attotime.cpp liegt in src/emu, nicht in src/lib/util.
set(MAME_UTIL_SRC
    "${MAME_EMU_ROOT}/attotime.cpp"
    "${MAME_ROOT}/src/lib/util/strformat.cpp"
    "${MAME_ROOT}/src/lib/util/timeconv.cpp"
    "${MAME_ROOT}/src/lib/util/unicode.cpp"
    "${MAME_ROOT}/src/lib/util/corestr.cpp"
    "${MAME_ROOT}/src/lib/util/corealloc.cpp"
    "${MAME_ROOT}/src/lib/util/coreutil.cpp"
    "${MAME_ROOT}/src/lib/util/vecstream.cpp"
    "${MAME_ROOT}/src/lib/util/delegate.cpp"
    "${MAME_ROOT}/src/lib/util/hash.cpp"
    "${MAME_ROOT}/src/lib/util/hashing.cpp"
    "${MAME_ROOT}/src/lib/util/md5.cpp"
    "${MAME_ROOT}/src/lib/util/sha1.cpp"
    "${MAME_ROOT}/src/lib/util/opresolv.cpp"
    "${MAME_ROOT}/src/lib/util/path_to_regex.cpp"
    "${MAME_ROOT}/src/lib/util/options.cpp"
    "${MAME_ROOT}/src/lib/util/disasmintf.cpp"
    "${MAME_ROOT}/src/lib/util/unzip.cpp"
    # 3rdparty: von MAME mitgelieferte Abhaengigkeiten, unveraendert
    "${MAME_ROOT}/3rdparty/utf8proc/utf8proc.c"
    # ---------------------------------------------------------------------
    # Nachtrag Lauf #24: der Linker forderte genau diese Objekte an
    # (docs/raw_err.txt: 671x "undefined reference", 114 verschiedene Symbole).
    # Die Auswahl ist MAMEs eigene Liste fuer die Bibliothek "utils" aus
    # scripts/src/lib.lua (nichts erfunden, nichts gekuerzt): util::xml::*
    # (xmlfile), bitmap_t (bitmap), chd_file/chd_codec_list (chd, chdcd,
    # chdcodec), flac_decoder (flac), png_info (png), avi_file (aviio),
    # m7z_file_cache_clear (un7z), core_file::open/jed/zippath-Pfade
    # (corefile, zippath), huffman/harddisk/palette/plaparse/pool/vbiparse/
    # wavwrite/cdrom sind deren eigene Abhaengigkeiten bzw. Bestandteil
    # derselben Bibliothek. Quelldateien bleiben unveraendert.
    "${MAME_ROOT}/src/lib/util/avhuff.cpp"
    "${MAME_ROOT}/src/lib/util/aviio.cpp"
    "${MAME_ROOT}/src/lib/util/bitmap.cpp"
    "${MAME_ROOT}/src/lib/util/cdrom.cpp"
    "${MAME_ROOT}/src/lib/util/chd.cpp"
    "${MAME_ROOT}/src/lib/util/chdcd.cpp"
    "${MAME_ROOT}/src/lib/util/chdcodec.cpp"
    "${MAME_ROOT}/src/lib/util/corefile.cpp"
    "${MAME_ROOT}/src/lib/util/flac.cpp"
    "${MAME_ROOT}/src/lib/util/harddisk.cpp"
    "${MAME_ROOT}/src/lib/util/huffman.cpp"
    "${MAME_ROOT}/src/lib/util/jedparse.cpp"
    "${MAME_ROOT}/src/lib/util/nanosvg.cpp"
    "${MAME_ROOT}/src/lib/util/palette.cpp"
    "${MAME_ROOT}/src/lib/util/plaparse.cpp"
    "${MAME_ROOT}/src/lib/util/png.cpp"
    "${MAME_ROOT}/src/lib/util/pool.cpp"
    "${MAME_ROOT}/src/lib/util/un7z.cpp"
    "${MAME_ROOT}/src/lib/util/vbiparse.cpp"
    "${MAME_ROOT}/src/lib/util/wavwrite.cpp"
    "${MAME_ROOT}/src/lib/util/xmlfile.cpp"
    "${MAME_ROOT}/src/lib/util/zippath.cpp"
    # Expat (XML-Parser) - MAMEs eigener Bau, scripts/src/3rdparty.lua
    # project "expat". xmlfile.cpp bindet <expat.h> ein.
    "${MAME_ROOT}/3rdparty/expat/lib/xmlparse.c"
    "${MAME_ROOT}/3rdparty/expat/lib/xmlrole.c"
    "${MAME_ROOT}/3rdparty/expat/lib/xmltok.c"
    # pugixml - MAME baut es als extlib (scripts/extlib.lua:
    # 3rdparty/pugixml/src). Verlangt wird es von der Softwareliste
    # (softlist.cpp: pugi::xml_document/xpath).
    "${MAME_ROOT}/3rdparty/pugixml/src/pugixml.cpp"
    # 7-Zip/LZMA - MAMEs project "7z" (scripts/src/3rdparty.lua).
    # unzip.cpp/un7z.cpp verlangen LzmaDec_Init/Free und den 7z-Leser.
    "${MAME_ROOT}/3rdparty/lzma/C/7zAlloc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zArcIn.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zBuf.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zBuf2.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zCrc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zCrcOpt.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zDec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zStream.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Aes.c"
    "${MAME_ROOT}/3rdparty/lzma/C/AesOpt.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Alloc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Bcj2.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Bra.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Bra86.c"
    "${MAME_ROOT}/3rdparty/lzma/C/BraIA64.c"
    "${MAME_ROOT}/3rdparty/lzma/C/CpuArch.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Delta.c"
    "${MAME_ROOT}/3rdparty/lzma/C/LzFind.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma2Dec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma2Enc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma86Dec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma86Enc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/LzmaDec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/LzmaEnc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Ppmd7.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Ppmd7Dec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Ppmd7Enc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Sha256.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Sort.c"
    # LibFLAC - MAMEs project "flac" (scripts/src/3rdparty.lua). chdcodec.cpp
    # braucht flac_decoder; die Liste ist MAMEs eigene.
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/bitmath.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/bitreader.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/bitwriter.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/cpu.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/crc.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/fixed.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/float.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/format.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/lpc.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/md5.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/memory.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/stream_decoder.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/stream_encoder.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/stream_encoder_framing.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/window.c"
)
# Meilenstein 1a: Kern aus src/emu. Die Auswahl stammt aus MAMEs Build-Skript
# scripts/src/emu.lua (Block "files {}", Zeilen 41-260); die Dateien bleiben
# unveraendert. Bewusst NICHT dabei: main.cpp (MAMEs eigenes main()), src/emu/debug/*
# (Debugger-UI), video/rgbsse.cpp und video/rgbvmx.cpp (x86-SSE/PPC-VMX),
# drivers/empty.cpp sowie das Grafik-/Audio-Geruest (kommt als M1b).
option(MAME_S3_NO_DESKTOP_TEXT "Omit desktop text posting and fuzzy software suggestions" ON)
set(MAME_NATKEYBOARD_SOURCE "${MAME_EMU_ROOT}/natkeyboard.cpp")
set(MAME_SOFTLIST_DEVICE_SOURCE "${MAME_EMU_ROOT}/softlist_dev.cpp")
if(MAME_S3_NO_DESKTOP_TEXT)
    find_program(MAME_TEXT_PYTHON NAMES python python3 REQUIRED)
    foreach(MAME_TEXT_KIND keyboard software)
        if(MAME_TEXT_KIND STREQUAL "keyboard")
            set(MAME_TEXT_ORIGINAL natkeyboard.cpp)
            set(MAME_TEXT_OUTPUT natkeyboard_text_s3.cpp)
        else()
            set(MAME_TEXT_ORIGINAL softlist_dev.cpp)
            set(MAME_TEXT_OUTPUT softlist_dev_text_s3.cpp)
        endif()
        execute_process(COMMAND "${MAME_TEXT_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_no_desktop_text.py"
            "${MAME_TEXT_KIND}" "${MAME_ROOT}/src/emu/${MAME_TEXT_ORIGINAL}"
            "${CMAKE_CURRENT_LIST_DIR}/generated/${MAME_TEXT_OUTPUT}"
            RESULT_VARIABLE MAME_TEXT_RESULT)
        if(NOT MAME_TEXT_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to prepare desktop-text overlay")
        endif()
    endforeach()
    set(MAME_NATKEYBOARD_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/natkeyboard_text_s3.cpp")
    set(MAME_SOFTLIST_DEVICE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/softlist_dev_text_s3.cpp")
endif()
option(MAME_S3_NO_EXTERNAL_MEDIA "Omit CHD ROM disks, software catalogs and HSI metadata" ON)
set(MAME_ROMLOAD_SOURCE "${MAME_EMU_ROOT}/romload.cpp")
set(MAME_HASHINFO_SOURCE "${MAME_EMU_ROOT}/hashfile.cpp")
if(MAME_S3_NO_EXTERNAL_MEDIA)
    find_program(MAME_MEDIA_PYTHON NAMES python python3 REQUIRED)
    foreach(MAME_MEDIA_KIND rom catalog metadata)
        if(MAME_MEDIA_KIND STREQUAL "rom")
            set(MAME_MEDIA_INPUT "${MAME_ROMLOAD_SOURCE}")
            set(MAME_MEDIA_OUTPUT romload_nomedia_s3.cpp)
            set(MAME_ROMLOAD_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/${MAME_MEDIA_OUTPUT}")
        elseif(MAME_MEDIA_KIND STREQUAL "catalog")
            set(MAME_MEDIA_INPUT "${MAME_SOFTLIST_DEVICE_SOURCE}")
            set(MAME_MEDIA_OUTPUT softlist_dev_nomedia_s3.cpp)
            set(MAME_SOFTLIST_DEVICE_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/${MAME_MEDIA_OUTPUT}")
        else()
            set(MAME_MEDIA_INPUT "${MAME_HASHINFO_SOURCE}")
            set(MAME_MEDIA_OUTPUT hashfile_nomedia_s3.cpp)
            set(MAME_HASHINFO_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/${MAME_MEDIA_OUTPUT}")
        endif()
        execute_process(COMMAND "${MAME_MEDIA_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_no_external_media.py"
            "${MAME_MEDIA_KIND}" "${MAME_MEDIA_INPUT}"
            "${CMAKE_CURRENT_LIST_DIR}/generated/${MAME_MEDIA_OUTPUT}"
            RESULT_VARIABLE MAME_MEDIA_RESULT)
        if(NOT MAME_MEDIA_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to prepare external-media overlay")
        endif()
    endforeach()
endif()
option(MAME_S3_NO_MOVIE_RECORDING "Omit AVI/MNG movie recording" ON)
set(MAME_VIDEO_SOURCE "${MAME_EMU_ROOT}/video.cpp")
if(MAME_S3_NO_MOVIE_RECORDING)
    find_program(MAME_MOVIE_PYTHON NAMES python python3 REQUIRED)
    set(MAME_VIDEO_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/video_nomovie_s3.cpp")
    execute_process(COMMAND "${MAME_MOVIE_PYTHON}"
        "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_no_movie_recording.py"
        "${MAME_ROOT}/src/emu/video.cpp" "${MAME_VIDEO_SOURCE}"
        RESULT_VARIABLE MAME_MOVIE_RESULT)
    if(NOT MAME_MOVIE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to prepare movie-recording overlay")
    endif()
endif()
set(MAME_EMU_SRC
    # -- Geraetebasis und Schnittstellen -------------------------------------
    "${MAME_EMU_ROOT}/device.cpp"
    "${MAME_EMU_ROOT}/devdelegate.cpp"
    "${MAME_EMU_ROOT}/devcb.cpp"
    "${MAME_EMU_ROOT}/devcpu.cpp"
    "${MAME_EMU_ROOT}/devfind.cpp"
    "${MAME_EMU_ROOT}/diexec.cpp"
    "${MAME_EMU_ROOT}/dimemory.cpp"
    "${MAME_EMU_ROOT}/distate.cpp"
    "${MAME_EMU_ROOT}/dinvram.cpp"
    "${MAME_EMU_ROOT}/dirom.cpp"
    "${MAME_EMU_ROOT}/didisasm.cpp"
    "${MAME_EMU_ROOT}/diserial.cpp"
    "${MAME_EMU_ROOT}/disound.cpp"
    "${MAME_EMU_ROOT}/dislot.cpp"
    "${MAME_EMU_ROOT}/diimage.cpp"
    "${MAME_EMU_ROOT}/dirtc.cpp"
    "${MAME_EMU_ROOT}/dinetwork.cpp"
    "${MAME_EMU_ROOT}/dipty.cpp"
    "${MAME_EMU_ROOT}/divideo.cpp"
    "${MAME_EMU_ROOT}/divtlb.cpp"
    "${MAME_EMU_ROOT}/dipalette.cpp"
    "${MAME_EMU_ROOT}/digfx.cpp"
    "${MAME_EMU_ROOT}/drawgfx.cpp"
    # -- Speicher / Adressraum (starke Templates -> viele kleine Einheiten) ---
    "${MAME_MEMORY_OVERLAY}"
    "${MAME_EMU_ROOT}/emumem_mud.cpp"
    "${MAME_EMU_ROOT}/emumem_hem.cpp"
    "${MAME_EMU_ROOT}/emumem_hedp.cpp"
    "${MAME_EMU_ROOT}/emumem_heun.cpp"
    "${MAME_EMU_ROOT}/emumem_heu.cpp"
    "${MAME_EMU_ROOT}/emumem_hedr0.cpp"
    "${MAME_EMU_ROOT}/emumem_hedr1.cpp"
    "${MAME_EMU_ROOT}/emumem_hedr2.cpp"
    "${MAME_EMU_ROOT}/emumem_hedr3.cpp"
    "${MAME_EMU_ROOT}/emumem_hedw0.cpp"
    "${MAME_EMU_ROOT}/emumem_hedw1.cpp"
    "${MAME_EMU_ROOT}/emumem_hedw2.cpp"
    "${MAME_EMU_ROOT}/emumem_hedw3.cpp"
    "${MAME_EMU_ROOT}/emumem_hep.cpp"
    "${MAME_EMU_ROOT}/emumem_het.cpp"
    "${MAME_EMU_ROOT}/addrmap.cpp"
    "${MAME_EMU_ROOT}/memarray.cpp"
    # -- Maschine -------------------------------------------------------------
    "${MAME_EMU_ROOT}/machine.cpp"
    # machine.cpp + main.cpp aus MAMEs Original: machine_manager haelt den
    # HTTP-Verwalter als einziges Mitglied (main.h:98), und
    # running_machine::run() ruft bedingungslos m_manager.http()->clear()
    # (machine.cpp:306/383). Ohne diese Datei zeigte http() auf nullptr -> der
    # S3 wuerde beim Maschinenstart abstuerzen. src/emu/main.cpp enthaelt
    # ausschliesslich machine_manager::machine_manager und
    # start_http_server() - kein main() (die fruehere Annahme im Kommentar
    # oben war falsch, im 0.220-Baum nachgeprueft). Der Server wird mit
    # options().http() angelegt -> Vorgabe aus, also inaktiv (http.cpp:276
    # kehrt sofort zurueck, clear() ebenfalls).
    "${MAME_EMU_ROOT}/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/s3_http.cpp"
    "${MAME_EMU_ROOT}/mconfig.cpp"
    "${MAME_EMU_ROOT}/driver.cpp"
    # drivenum.cpp liefert driver_list::find(char const*) - die Treibertabelle
    # selbst setzt s3_startup.cpp, die Suche fehlte (114 fehlende Symbole).
    "${MAME_EMU_ROOT}/drivenum.cpp"
    "${MAME_EMU_ROOT}/schedule.cpp"
    "${MAME_EMU_ROOT}/save.cpp"
    "${MAME_EMU_ROOT}/emualloc.cpp"
    "${MAME_EMU_ROOT}/emucore.cpp"
    "${MAME_EMU_ROOT}/xtal.cpp"
    "${MAME_EMU_ROOT}/emuopts.cpp"
    "${MAME_EMU_ROOT}/config.cpp"
    "${MAME_EMU_ROOT}/bookkeeping.cpp"
    "${MAME_EMU_ROOT}/parameters.cpp"
    "${MAME_EMU_ROOT}/validity.cpp"
    "${MAME_EMU_ROOT}/profiler.cpp"
    # -- Dateien, ROM-Saetze, Hashes -----------------------------------------
    "${MAME_EMU_ROOT}/fileio.cpp"
    "${MAME_HASHINFO_SOURCE}"
    "${MAME_ROMLOAD_SOURCE}"
    "${MAME_EMU_ROOT}/romentry.cpp"
    # -- Ein- und Ausgabe ------------------------------------------------------
    "${MAME_EMU_ROOT}/ioport.cpp"
    "${MAME_EMU_ROOT}/input.cpp"
    "${MAME_EMU_ROOT}/inputdev.cpp"
    "${MAME_EMU_ROOT}/uiinput.cpp"
    "${MAME_NATKEYBOARD_SOURCE}"
    "${MAME_EMU_ROOT}/output.cpp"
    "${MAME_EMU_ROOT}/network.cpp"
    # http.cpp fehlt hier bewusst: MAMEs eingebauter HTTP-Server zieht asio und
    # die Web++-Bibliothek nach und braucht POSIX-Sockets (u. a. ::socketpair),
    # die ESP-IDF so nicht anbietet (Lauf #19:
    # "asio/detail/impl/socket_ops.ipp:623: error: '::socketpair' has not been
    # declared" plus -Werror=uninitialized in server_http_impl.hpp).
    # Fuer die Emulation von mm4 wird er nicht gebraucht.
)



# Meilenstein 1b: Grafik-/Audio-Geruest und die restlichen Kernverwalter.
# Grund (belegt in Lauf #21): running_machine haelt diese Verwalter als
# Mitglieder. Sie muessen deshalb uebersetzt werden, auch wenn mm4 weder
# Grafik noch Ton ausgibt:
#   video.cpp/screen.cpp/render*.cpp/tilemap.cpp/emupal.cpp -> Video
#   sound.cpp/speaker.cpp                                   -> Audio
#   softlist*.cpp                                           -> Softwarelisten
#   image.cpp/crsshair.cpp                                  -> Verwalter (image/crosshair)
#   debugger.cpp + src/emu/debug/*                          -> Debugger-Verwalter
# Die Datei debugger.cpp ist dabei nur wegen ihres Destruktors noetig (MAME
# legt den Debugger nur bei gesetztem debug_flags an, siehe machine.cpp:246).
option(MAME_S3_NO_DEBUG_COMMANDS "Omit desktop debugger commands in chess profile" ON)
set(MAME_DEBUG_COMMAND_SOURCE "${MAME_EMU_ROOT}/debug/debugcmd.cpp")
if(MAME_S3_NO_DEBUG_COMMANDS)
    find_program(MAME_DEBUG_PYTHON NAMES python python3 REQUIRED)
    set(MAME_DEBUG_COMMAND_SOURCE "${CMAKE_CURRENT_LIST_DIR}/generated/debugcmd_s3.cpp")
    execute_process(COMMAND "${MAME_DEBUG_PYTHON}"
        "${CMAKE_CURRENT_LIST_DIR}/../../../tools/prepare_no_debug_commands.py"
        "${MAME_ROOT}/src/emu/debug/debugcmd.cpp" "${MAME_DEBUG_COMMAND_SOURCE}"
        RESULT_VARIABLE MAME_DEBUG_RESULT)
    if(NOT MAME_DEBUG_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to prepare debugger command overlay")
    endif()
endif()
set(MAME_EMU_M1B_SRC
    "${MAME_VIDEO_SOURCE}"
    "${MAME_EMU_ROOT}/screen.cpp"
    "${MAME_EMU_ROOT}/render.cpp"
    "${MAME_EMU_ROOT}/rendfont.cpp"
    "${MAME_EMU_ROOT}/rendlay.cpp"
    "${MAME_EMU_ROOT}/rendutil.cpp"
    "${MAME_EMU_ROOT}/tilemap.cpp"
    "${MAME_EMU_ROOT}/emupal.cpp"
    "${MAME_EMU_ROOT}/sound.cpp"
    "${MAME_EMU_ROOT}/speaker.cpp"
    "${MAME_EMU_ROOT}/softlist.cpp"
    "${MAME_SOFTLIST_DEVICE_SOURCE}"
    "${MAME_EMU_ROOT}/video/generic.cpp"
    "${MAME_EMU_ROOT}/image.cpp"
    "${MAME_EMU_ROOT}/crsshair.cpp"
    "${MAME_EMU_ROOT}/debugger.cpp"
    "${MAME_EMU_ROOT}/debug/debugbuf.cpp"
    "${MAME_DEBUG_COMMAND_SOURCE}"
    "${MAME_EMU_ROOT}/debug/debugcon.cpp"
    "${MAME_EMU_ROOT}/debug/debugcpu.cpp"
    "${MAME_EMU_ROOT}/debug/debughlp.cpp"
    "${MAME_EMU_ROOT}/debug/debugvw.cpp"
    "${MAME_EMU_ROOT}/debug/dvbpoints.cpp"
    "${MAME_EMU_ROOT}/debug/dvdisasm.cpp"
    "${MAME_EMU_ROOT}/debug/dvmemory.cpp"
    "${MAME_EMU_ROOT}/debug/dvstate.cpp"
    "${MAME_EMU_ROOT}/debug/dvtext.cpp"
    "${MAME_EMU_ROOT}/debug/dvwpoints.cpp"
    "${MAME_EMU_ROOT}/debug/express.cpp"
    "${MAME_EMU_ROOT}/debug/textbuf.cpp"
)

# Meilenstein 1c: CPU und Geraete des Ziels mm4 (Mephisto MM IV).
# Die Pfade sind gegen den echten 0.220-Baum geprueft: 74259, dac, volt_reg und
# die generischen Steckplaetze liegen in src/devices (nicht in src/mame), nur
# mmboard liegt in src/mame/machine.
set(MAME_M1C_SRC
    "${MAME_MEPHISTO_SOURCE}"
    "${MAME_6502_OVERLAY}"
    "${MAME_ROOT}/src/devices/cpu/m6502/m6502d.cpp"
    "${MAME_ROOT}/src/devices/cpu/m6502/m65c02.cpp"
    "${MAME_ROOT}/src/devices/cpu/m6502/m65c02d.cpp"
    "${MAME_ROOT}/src/devices/machine/74259.cpp"
    "${MAME_ROOT}/src/devices/machine/timer.cpp"
    "${MAME_ROOT}/src/mame/machine/mmboard.cpp"
    "${MAME_ROOT}/src/devices/machine/sensorboard.cpp"
    "${MAME_ROOT}/src/devices/video/pwm.cpp"
    "${MAME_ROOT}/src/devices/sound/dac.cpp"
    "${MAME_ROOT}/src/devices/sound/volt_reg.cpp"
    "${MAME_ROOT}/src/devices/bus/generic/slot.cpp"
    "${MAME_ROOT}/src/devices/bus/generic/carts.cpp"
    "${MAME_ROOT}/src/devices/bus/generic/ram.cpp"
    "${MAME_ROOT}/src/devices/bus/generic/rom.cpp"
)

# --- Layoutdateien des Treibers -------------------------------------------
# mephisto_mm2.cpp bindet "mephisto_mm2.lh" und "mephisto_bup.lh" ein. Diese
# Dateien liegen nicht im Quellbaum: MAME erzeugt sie beim Bau aus den
# .lay-Dateien (makefile: scripts/build/complay.py <quelle.lay> <ziel.lh>
# layout_<name>). Hier wird genau dieses Originalwerkzeug benutzt, damit die
# Treiberquelle unveraendert bleibt (kein Fork).
#
# WICHTIG (Lauf #22): ESP-IDF liest diesen Bauplan zweimal - einmal im
# "Skriptmodus" (component_get_requirements.cmake sammelt nur die
# Abhaengigkeiten) und einmal normal. Im Skriptmodus bricht
# add_custom_command mit "add_custom_command command is not scriptable" ab.
# Deshalb wird complay.py mit execute_process sofort beim Konfigurieren
# aufgerufen; das ist in beiden Durchgaengen erlaubt und liefert dieselben
# Dateien. Das Ziel liegt bewusst neben dem Bauplan (nicht im Bauverzeichnis),
# weil der Skriptmodus kein zuverlaessiges CMAKE_CURRENT_BINARY_DIR hat.
set(MAME_LAYOUT_DIR "${CMAKE_CURRENT_LIST_DIR}/generated/layout")
file(MAKE_DIRECTORY "${MAME_LAYOUT_DIR}")

find_program(MAME_PYTHON NAMES python python3)
if(MAME_PYTHON)
    set(MAME_LAYOUT_PATHS
        "${MAME_ROOT}/src/mame/layout/mephisto_mm2"
        "${MAME_ROOT}/src/mame/layout/mephisto_bup"
        "${MAME_EMU_ROOT}/layout/noscreens"
        "${MAME_EMU_ROOT}/layout/dualhsxs"
        "${MAME_EMU_ROOT}/layout/dualhovu"
        "${MAME_EMU_ROOT}/layout/dualhuov"
        "${MAME_EMU_ROOT}/layout/triphsxs"
        "${MAME_EMU_ROOT}/layout/quadhsxs")
    foreach(_lay_base ${MAME_LAYOUT_PATHS})
        get_filename_component(_lay "${_lay_base}" NAME)
        set(_mame_lay_file "${_lay_base}.lay")
        set(_mame_lh_file  "${MAME_LAYOUT_DIR}/${_lay}.lh")
        if(EXISTS "${_mame_lay_file}")
            execute_process(
                COMMAND "${MAME_PYTHON}" "${MAME_ROOT}/scripts/build/complay.py"
                        "${_mame_lay_file}" "${_mame_lh_file}" "layout_${_lay}"
                RESULT_VARIABLE _mame_lay_rc
                OUTPUT_QUIET ERROR_QUIET)
            if(_mame_lay_rc EQUAL 0)
                message(STATUS "MAME-Layout erzeugt: ${_mame_lh_file}")
            else()
                message(WARNING "complay.py scheiterte fuer ${_lay} (Rueckgabe ${_mame_lay_rc})")
            endif()
        else()
            message(WARNING "Layoutdatei fehlt im Quellbaum: ${_mame_lay_file}")
        endif()
    endforeach()
else()
    message(WARNING "Kein Python gefunden (python/python3): die Layoutdateien des Treibers fehlen")
endif()

# --- Von MAME erzeugte CPU-Tabellen (Mephisto nutzt den 65C02) -------------
# m6502.cpp und m65c02.cpp binden "cpu/m6502/m6502.hxx" bzw. "m65c02.hxx" ein
# (dazu die Disassembler-Tabellen *d.hxx). Beide entstehen aus den .lst-Listen
# mit MAMEs eigenem Werkzeug m6502make.py.
# Aufruf laut m6502make.py (main): <modus s|d> <geraet> <opc.lst|-> <disp.lst> <ziel>
# Ablage unter generated/, damit der Include-Pfad "cpu/m6502/..." passt.
set(MAME_GEN_DIR "${CMAKE_CURRENT_LIST_DIR}/generated")
set(MAME_6502_DIR "${MAME_ROOT}/src/devices/cpu/m6502")
set(MAME_6502_GEN "${MAME_GEN_DIR}/cpu/m6502")
file(MAKE_DIRECTORY "${MAME_6502_GEN}")
if(MAME_PYTHON)
    execute_process(COMMAND "${MAME_PYTHON}" "${MAME_6502_DIR}/m6502make.py" s m6502
        "${MAME_6502_DIR}/om6502.lst" "${MAME_6502_DIR}/dm6502.lst" "${MAME_6502_GEN}/m6502.hxx"
        RESULT_VARIABLE _rc1 OUTPUT_QUIET ERROR_QUIET)
    execute_process(COMMAND "${MAME_PYTHON}" "${MAME_6502_DIR}/m6502make.py" d m6502 -
        "${MAME_6502_DIR}/dm6502.lst" "${MAME_6502_GEN}/m6502d.hxx"
        RESULT_VARIABLE _rc2 OUTPUT_QUIET ERROR_QUIET)
    execute_process(COMMAND "${MAME_PYTHON}" "${MAME_6502_DIR}/m6502make.py" s m65c02
        "${MAME_6502_DIR}/om65c02.lst" "${MAME_6502_DIR}/dm65c02.lst" "${MAME_6502_GEN}/m65c02.hxx"
        RESULT_VARIABLE _rc3 OUTPUT_QUIET ERROR_QUIET)
    execute_process(COMMAND "${MAME_PYTHON}" "${MAME_6502_DIR}/m6502make.py" d m65c02 -
        "${MAME_6502_DIR}/dm65c02.lst" "${MAME_6502_GEN}/m65c02d.hxx"
        RESULT_VARIABLE _rc4 OUTPUT_QUIET ERROR_QUIET)
    message(STATUS "CPU-Tabellen erzeugt (m6502/m65c02, je s und d): ${_rc1}/${_rc2}/${_rc3}/${_rc4}")
else()
    message(WARNING "Kein Python: die CPU-Tabellen (*.hxx) fehlen")
endif()

# ESP-IDF 5.3 hat keine zlib-Komponente -> MAMEs eigenes 3rdparty/zlib nutzen.
file(GLOB MAME_ZLIB_SRC "${MAME_ROOT}/3rdparty/zlib/*.c")
# Die gz*-Dateien (gzip-Datei-I/O) brauchen POSIX read/write/lseek und sind fuer
# crc32/adler32/compress/decompress nicht erforderlich -> ausgeschlossen.
# Filter ueber foreach+MATCHES: list(FILTER ... REGEX) hat die Pfade nicht
# getroffen (Build #9 hat gzlib/gzread/gzwrite weiter uebersetzt).
set(MAME_ZLIB_KEEP "")
foreach(_zsrc ${MAME_ZLIB_SRC})
    get_filename_component(_zname "${_zsrc}" NAME)
    if(_zname MATCHES "^gz")
        # gzlib.c/gzread.c/gzwrite.c/gzclose.c: nur gzip-Datei-I/O -> weg
    else()
        list(APPEND MAME_ZLIB_KEEP "${_zsrc}")
    endif()
endforeach()
list(LENGTH MAME_ZLIB_SRC _zall)
list(LENGTH MAME_ZLIB_KEEP _zkeep)
message(STATUS "MAME 3rdparty/zlib: ${_zkeep} von ${_zall} Dateien aktiv")

# JPEG core sources from the upstream scripts/src/3rdparty.lua target. Exclude
# command-line programs and platform-specific memory managers; use jmemansi.
file(GLOB MAME_JPEG_SRC "${MAME_ROOT}/3rdparty/libjpeg/j*.c")
list(FILTER MAME_JPEG_SRC EXCLUDE REGEX "/(jpegtran|jmemdos|jmemmac|jmemname|jmemnobs)\\.c$")

get_filename_component(RETRO_UCI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../integration/chessengine" ABSOLUTE)
set(RETRO_LUA_ROOT "${MAME_ROOT}/3rdparty/lua/src")
file(GLOB RETRO_LUA_SRC "${RETRO_LUA_ROOT}/*.c")
list(FILTER RETRO_LUA_SRC EXCLUDE REGEX "/(lua|luac|linit|liolib|loslib|loadlib|ldblib)\\.c$")
set(RETRO_CPP_SRC "${RETRO_UCI_ROOT}/src/retro_runtime.cpp" "${RETRO_UCI_ROOT}/src/s3_uci_console.cpp")
# Keep Lua/payload includes local to these files: changing MAME's target-wide
# include search path would unnecessarily recompile every core translation unit.

idf_component_register(
    SRCS
        ${MAME_UTIL_SRC}
        ${MAME_EMU_SRC}
        ${MAME_EMU_M1B_SRC}
        ${MAME_M1C_SRC}
        ${MAME_ZLIB_KEEP}
        ${MAME_JPEG_SRC}
        ${RETRO_LUA_SRC}
        ${RETRO_CPP_SRC}
        # Projektseitige Dateien: Bruecke, OSD-Ersatzschicht, Startpfad
        "src/mame_bridge.cpp"
        "src/s3_osd.cpp"
        "src/s3_osd_core.cpp"
        "src/s3_netdev.cpp"
        "src/s3_startup.cpp"
        # MAMEs plattformunabhaengiger Teil der OSD-Schicht: er liefert
        # osd_output::push/pop und osd_vprintf_* (die Verteilung der Meldungen
        # an die Ausgabekette). Die plattformabhaengigen Reste stehen in
        # s3_osd_core.cpp. Die Datei bleibt unveraendert.
        "src/s3_osd_output.cpp"
    REQUIRES
        esp_timer
    INCLUDE_DIRS
        "include"
        "${MAME_EMU_ROOT}"
        "${MAME_ROOT}/src/lib"
        "${MAME_ROOT}/src/lib/util"
        "${MAME_ROOT}/src/osd"
        # Wurzel fuer die Include-Pfade der Geraete, wie sie der Treiber nennt
        # ("cpu/m6502/m65c02.h", "machine/74259.h", "sound/dac.h", ...)
        "${MAME_ROOT}/src/devices"
        "${MAME_ROOT}/src/mame"
        # Erzeugte Layoutdateien des Treibers (mephisto_mm2.lh, mephisto_bup.lh)
        "${MAME_LAYOUT_DIR}"
        # Erzeugte CPU-Tabellen: darunter liegt cpu/m6502/m6502.hxx usw., genau
        # so, wie die MAME-Quellen sie einbinden.
        "${MAME_GEN_DIR}"
        "${MAME_ROOT}/3rdparty"
        "${MAME_ROOT}/3rdparty/utf8proc"
        "${MAME_ROOT}/3rdparty/zlib"
        # Lauf #24: Include-Pfade der nachgetragenen Bibliotheken
        # (7z/LZMA: 7zTypes.h usw.; libFLAC: config.h liegt unter
        # src/libFLAC/include, FLAC/*.h unter include, share/compat.h unter src)
        "${MAME_ROOT}/3rdparty/lzma/C"
        "${MAME_ROOT}/3rdparty/libflac/include"
        "${MAME_ROOT}/3rdparty/libflac/src"
        "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/include"
        "${MAME_ROOT}/3rdparty/expat/lib"
        "${MAME_ROOT}/3rdparty/pugixml/src"
        "${MAME_ROOT}/3rdparty/rapidjson/include"
        "${MAME_ROOT}/3rdparty/asio/include"
        "${MAME_ROOT}/3rdparty/libflac/include"
        "${MAME_ROOT}/3rdparty/libjpeg"
)

set_source_files_properties(${RETRO_CPP_SRC} PROPERTIES COMPILE_OPTIONS
    "-I${RETRO_UCI_ROOT}/include;-I${RETRO_UCI_ROOT}/generated;-I${RETRO_LUA_ROOT}")

# Die Layoutdateien entstehen beim Konfigurieren (execute_process, siehe oben);
# hier ist deshalb keine Zielabhaengigkeit mehr noetig.

# MAME-0.220-Schalter (aus dem Original-Makefile abgeleitet)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -fexceptions
    -frtti
    -fsigned-char
    -Wno-invalid-offsetof
    -Wno-format
    -Wno-implicit-fallthrough
    -Wno-format-security
    # MAME 0.220 stammt aus 2020 und ist unter GCC 13 nicht warnungsfrei;
    # ESP-IDF setzt aber -Werror=all. Ohne die naechsten beiden Zeilen bricht
    # der Bau an Warnungen ab, die keine Fehler sind - belegt in Lauf #22:
    #   mconfig.h:126  loop variable 'lay' binds to a temporary
    #                  [-Werror=range-loop-construct], ausgeloest von
    #                  render.cpp:1587, ebenso rendlay.cpp und rendfont.cpp
    # Beide Schalter stehen hinter den IDF-Schaltern (nachgeprueft in
    # build.ninja) und gewinnen deshalb. Die Warnungen bleiben im Protokoll
    # sichtbar, brechen den Bau aber nicht ab.
    -Wno-error
    -Wno-error=range-loop-construct
    # xtensa-Typzustand herstellen, den MAME 0.220 voraussetzt (Nachweis:
    # tools/type_probe.ps1 und tools/xtensa_int32_probe.cpp).
    # Auf x86/ARM ist int32_t == int und uint32_t == unsigned int; auf xtensa
    # (newlib) ist int32_t == long, waehrend size_t == unsigned int ist. MAME
    # verlaesst sich an vielen Stellen auf die x86/ARM-Identitaet, z. B.
    #   save.h:166  save_item(int)            -> static_assert schlaegt fehl
    #   dipalette.cpp:211  std::min(size_t, u32)
    #   inputdev.cpp:946   std::max(s32, int)
    #   natkeyboard.cpp:337  Delegate (void*, int) vs (void*, s32)
    # Die beiden Defines beheben alle diese Faelle an der Wurzel; sie wirken
    # ausschliesslich auf diese Komponente (ESP-IDF bleibt unberuehrt) und sind
    # ABI-identisch (beide Typen bleiben 32 Bit, gleiche Vorzeichenhaftigkeit).
    -D__INT32_TYPE__=int
    -D__UINT32_TYPE__=unsigned int
    # xtensa: der Linker hat in Lauf #24 achtzehnmal gemeldet
    #   "dangerous relocation: l32r: literal target out of range
    #    (try using text-section-literals)"
    # Die Literal-Pools lagen weiter als 256 KB vom Code entfernt (das Abbild
    # enthaelt jetzt die grosse MAME-Utils-Schicht). -mtext-section-literals
    # legt die Literale in die Textnaehe, -mlongcalls macht entfernte Aufrufe
    # ueber CALL8 statt ueber den begrenzten Direktaufruf. Beides ist der vom
    # Linker selbst genannte Weg und aendert keine Quelle.
    -mtext-section-literals
    -mlongcalls
    # Xtensa ld 13.2.0 literal relaxation can coalesce a format's one-past-end
    # pointer with the following string's start BEFORE merging string sections.
    # Those addresses differ after merging across objects (config.cpp queried
    # "default.cfg" instead of "cfg_directory"). Reproduced independently in
    # tools/xtensa_literal_{probe,anchor}.cpp. Keep SDK vector relaxation active;
    # disable string-section merging only for this MAME component instead.
    -fno-merge-constants
    # Ersatz-Deklaration fuer die eine libc-Funktion, die xtensa/newlib nicht
    # kennt und die asio (nur) deklariert benutzt: socketpair. Die Datei ist
    # reine Deklaration, kein Ersatzverhalten; siehe src/xtensa_posix_shim.h.
    -include "${CMAKE_CURRENT_LIST_DIR}/src/xtensa_posix_shim.h"
)

# Schalter der nachgetragenen 3rdparty-Bibliotheken - genau die Werte aus
# MAMEs eigenem Bauplan (scripts/src/3rdparty.lua), nur fuer diese Dateien:
#   expat:   project "expat"   (HAVE_* / PACKAGE_* / BYTEORDER, XML_DTD, XML_NS)
#   flac:    project "flac"    (HAVE_CONFIG_H -> 3rdparty/libflac/src/libFLAC/
#                               include/config.h, FLAC__NO_ASM, FLAC__HAS_OGG=0)
#   7z/lzma: project "7z"      (_7ZIP_ST, _7ZIP_PPMD_SUPPPORT - Schreibweise
#                               aus MAME uebernommen)
set_source_files_properties(
    "${MAME_ROOT}/3rdparty/expat/lib/xmlparse.c"
    "${MAME_ROOT}/3rdparty/expat/lib/xmlrole.c"
    "${MAME_ROOT}/3rdparty/expat/lib/xmltok.c"
    PROPERTIES COMPILE_DEFINITIONS
    "HAVE_MEMMOVE;HAVE_STDINT_H;HAVE_STDLIB_H;HAVE_STRING_H;STDC_HEADERS;XML_CONTEXT_BYTES=1024;XML_DTD;XML_NS;BYTEORDER=1234;PACKAGE_NAME=\"expat\";PACKAGE_VERSION=\"2.1.1\";PACKAGE_STRING=\"expat 2.1.1\";PACKAGE_TARNAME=\"expat\";PACKAGE_URL=\"\";PACKAGE_BUGREPORT=\"expat-bugs@libexpat.org\""
    COMPILE_OPTIONS
    "-Wno-error=maybe-uninitialized;-Wno-maybe-uninitialized")
set_source_files_properties(
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/bitmath.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/bitreader.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/bitwriter.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/cpu.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/crc.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/fixed.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/float.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/format.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/lpc.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/md5.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/memory.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/stream_decoder.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/stream_encoder.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/stream_encoder_framing.c"
    "${MAME_ROOT}/3rdparty/libflac/src/libFLAC/window.c"
    PROPERTIES COMPILE_DEFINITIONS
    # Lauf #24, Fehler 3: libFLACs "#ifdef HAVE_CONFIG_H / #include <config.h>"
    # fand MAMEs src/emu/config.h (beide Include-Pfade liegen in derselben
    # Komponente; -I-Reihenfolge kann pro Datei nicht getrennt werden). In
    # C-Dateien endet das fatal: MAMEs config.h zieht xmlfile.h -> osdcore.h ->
    # osdcomm.h -> <cstdio>, das es in C nicht gibt.
    # libFLACs eigenes config.h enthaelt nur eine Zeile:
    #     #define VERSION "1.2.1"
    # und libFLAC hat fuer VERSION einen eigenen Rueckfallwert (metadata_object.c:
    # #ifdef VERSION ... #else "1.2.1"). Deshalb wird HAVE_CONFIG_H hier bewusst
    # NICHT gesetzt: libFLAC uebersetzt dann ohne config.h - der uebliche Weg auf
    # Plattformen ohne autoconf. Alles andere (FLAC__NO_ASM, FLAC__HAS_OGG,
    # WORDS_BIGENDIAN) bleibt gesetzt.
    # libFLACs eigenes config.h enthaelt genau eine Zeile:
    #     #define VERSION "1.2.1"
    # Diese Zeile wird hier wortgleich als Definition mitgegeben - damit ist der
    # Inhalt von config.h erfuellt, ohne die kollidierende Datei einzubinden
    # (format.c:60/66 braucht VERSION unabhaengig vom autoconf-Zweig).
    # Alles andere (FLAC__NO_ASM, FLAC__HAS_OGG, WORDS_BIGENDIAN) bleibt gesetzt.
    "FLAC__NO_ASM;FLAC__HAS_OGG=0;WORDS_BIGENDIAN=0;FLAC__OVERFLOW_DETECT=1;FLAC__NO_DLL;VERSION=\"1.2.1\""
    COMPILE_OPTIONS
    "-Wno-error=maybe-uninitialized;-Wno-maybe-uninitialized")
# pugixml (Fremdquelle): braucht LLONG_MIN/LLONG_MAX/ULLONG_MAX (Zeilen
# 4564/4569). Der -include-Shim liefert sie inzwischen auch, aber diese drei
# Definitionen gelten unabhaengig von Include-Reihenfolge und Spracheinstellung
# und machen die Stelle sicher. Dazu derselbe Warnungsgrund wie oben (alte
# C++-Quelle unter GCC 13 mit -Wall -Werror=all). Betrifft nur diese Datei.
set_source_files_properties(
    "${MAME_ROOT}/3rdparty/pugixml/src/pugixml.cpp"
    PROPERTIES COMPILE_OPTIONS
    "-Wno-error=maybe-uninitialized;-Wno-maybe-uninitialized"
    COMPILE_DEFINITIONS
    "LLONG_MAX=9223372036854775807LL;LLONG_MIN=(-9223372036854775807LL-1);ULLONG_MAX=18446744073709551615ULL")
set_source_files_properties(
    "${MAME_ROOT}/3rdparty/lzma/C/7zAlloc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zArcIn.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zBuf.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zBuf2.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zCrc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zCrcOpt.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zDec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/7zStream.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Aes.c"
    "${MAME_ROOT}/3rdparty/lzma/C/AesOpt.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Alloc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Bcj2.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Bra.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Bra86.c"
    "${MAME_ROOT}/3rdparty/lzma/C/BraIA64.c"
    "${MAME_ROOT}/3rdparty/lzma/C/CpuArch.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Delta.c"
    "${MAME_ROOT}/3rdparty/lzma/C/LzFind.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma2Dec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma2Enc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma86Dec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Lzma86Enc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/LzmaDec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/LzmaEnc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Ppmd7.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Ppmd7Dec.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Ppmd7Enc.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Sha256.c"
    "${MAME_ROOT}/3rdparty/lzma/C/Sort.c"
    PROPERTIES COMPILE_DEFINITIONS
    "_7ZIP_ST;_7ZIP_PPMD_SUPPPORT"
    COMPILE_OPTIONS
    "-Wno-error=maybe-uninitialized;-Wno-maybe-uninitialized")

# PTR64 wird bewusst NICHT definiert: MAME setzt es nur auf 64-Bit-Zielen, und
# validity.cpp prueft per "#ifdef PTR64" (Lauf #19: sonst
# "static assertion failed: PTR64 flag enabled, but was compiled for 32-bit target").
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LSB_FIRST=1
    MAME_NOASM=1
    __STDC_LIMIT_MACROS
    __STDC_FORMAT_MACROS
    NDEBUG=1
    # Lauf #24, Fehler 1: corefile.cpp:35 bricht ohne CRLF ab
    #   "#error CRLF undefined: must be 1 (CR), 2 (LF) or 3 (CR/LF)".
    # MAME setzt das in scripts/genie.lua:629-638 global: Windows -> CRLF=3,
    # jeder andere Zielsystem-Typ -> CRLF=2. Der S3 ist ein Unix-artiges Ziel
    # (Zeilenende LF), der Wert ist also 2 - nicht der des Windows-Bauhosts.
    CRLF=2
)
set_property(SOURCE "${MAME_6502_OVERLAY}"
    "${MAME_ROOT}/src/devices/cpu/m6502/m65c02.cpp"
    APPEND PROPERTY COMPILE_OPTIONS -O2 -fjump-tables -ftree-switch-conversion -fno-merge-constants)
