# Optional EXTRA_COMPONENT_DIRS entry; deliberately outside esp32s3/components
# so the concurrently running MAME build is not reconfigured by this work.
get_filename_component(MAME_LUA "${CMAKE_CURRENT_LIST_DIR}/../../ref/mame0220-src/3rdparty/lua/src" ABSOLUTE)
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/generated/payload.h")
    message(FATAL_ERROR "Run integration/chessengine/tools/import_cbemu.py first")
endif()
file(GLOB LUA_CORE "${MAME_LUA}/*.c")
list(FILTER LUA_CORE EXCLUDE REGEX "/(lua|luac|linit|liolib|loslib|loadlib|ldblib)\\.c$")
idf_component_register(SRCS "src/retro_runtime.cpp" ${LUA_CORE}
    INCLUDE_DIRS "include" PRIV_INCLUDE_DIRS "generated" "${MAME_LUA}" REQUIRES heap)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
