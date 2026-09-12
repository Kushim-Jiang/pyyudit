cmake_minimum_required(VERSION 3.15)
project(yudit_shaper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Options ────────────────────────────────────────────────────────────────
option(YUDIT_BUILD_SHARED "Build shared library (default for wheel)" ON)
option(YUDIT_BUILD_STATIC "Build static library (for testing)" OFF)

# ── Yudit source root (vendored in-tree) ──────────────────────────────────
set(YUDIT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/native/vendor/yudit"
    CACHE PATH "Path to the vendored Yudit 3.1.0 source directory")

# ── Collect Yudit C++ sources (shaping core only) ─────────────────────────

# stoolkit — core data structures, BiDi, Unicode, glyph representation
set(STOOLKIT_DIR "${YUDIT_SRC_DIR}/stoolkit")
set(STOOLKIT_SRCS
    ${STOOLKIT_DIR}/SBiDi.cpp
    ${STOOLKIT_DIR}/SCharClass.cpp
    ${STOOLKIT_DIR}/SCluster.cpp
    ${STOOLKIT_DIR}/SGlyph.cpp
    ${STOOLKIT_DIR}/SParagraph.cpp
    ${STOOLKIT_DIR}/SRendClass.cpp
    ${STOOLKIT_DIR}/SUniMap.cpp
    ${STOOLKIT_DIR}/SEmbedState.cpp
    ${STOOLKIT_DIR}/SEncoder.cpp
    ${STOOLKIT_DIR}/SProperties.cpp
    ${STOOLKIT_DIR}/SIO.cpp
    ${STOOLKIT_DIR}/SIOStream.cpp
    ${STOOLKIT_DIR}/SEvent.cpp
    ${STOOLKIT_DIR}/SEventBSD.cpp
    ${STOOLKIT_DIR}/SExcept.cpp
    ${STOOLKIT_DIR}/SString.cpp
    ${STOOLKIT_DIR}/SStringVector.cpp
    ${STOOLKIT_DIR}/SObject.cpp
    ${STOOLKIT_DIR}/SBVector.cpp
    ${STOOLKIT_DIR}/SBHashtable.cpp
    ${STOOLKIT_DIR}/SBMap.cpp
    ${STOOLKIT_DIR}/SHShared.cpp
    ${STOOLKIT_DIR}/SHWConverter.cpp
    ${STOOLKIT_DIR}/SConfig.cpp
    ${STOOLKIT_DIR}/SShared.cpp
    ${STOOLKIT_DIR}/SUniqueID.cpp
    ${STOOLKIT_DIR}/SUtil.cpp
    ${STOOLKIT_DIR}/SDimension.cpp
    ${STOOLKIT_DIR}/SLocation.cpp
    ${STOOLKIT_DIR}/SMatrix.cpp
    ${STOOLKIT_DIR}/STextData.cpp
    ${STOOLKIT_DIR}/STextIndex.cpp
    ${STOOLKIT_DIR}/SAutogen.cpp
    ${STOOLKIT_DIR}/SCursorIndex.cpp
    ${STOOLKIT_DIR}/SLineCurve.cpp
    ${STOOLKIT_DIR}/SRectangle.cpp
    # Encoding backends (required by SEncoder)
    ${STOOLKIT_DIR}/sencoder/SB_BiDi.cpp
    ${STOOLKIT_DIR}/sencoder/SB_BIG5.cpp
    ${STOOLKIT_DIR}/sencoder/SB_DeShape.cpp
    ${STOOLKIT_DIR}/sencoder/SB_EUC_JP.cpp
    ${STOOLKIT_DIR}/sencoder/SB_EUC_JP0213.cpp
    ${STOOLKIT_DIR}/sencoder/SB_EUC_KR.cpp
    ${STOOLKIT_DIR}/sencoder/SB_GB18030.cpp
    ${STOOLKIT_DIR}/sencoder/SB_GB2312_8.cpp
    ${STOOLKIT_DIR}/sencoder/SB_Generic.cpp
    ${STOOLKIT_DIR}/sencoder/SB_HZ.cpp
    ${STOOLKIT_DIR}/sencoder/SB_ISO2022_JP.cpp
    ${STOOLKIT_DIR}/sencoder/SB_ISO2022_JP3.cpp
    ${STOOLKIT_DIR}/sencoder/SB_Java.cpp
    ${STOOLKIT_DIR}/sencoder/SB_Johab.cpp
    ${STOOLKIT_DIR}/sencoder/SB_NCR.cpp
    ${STOOLKIT_DIR}/sencoder/SB_S_JIS.cpp
    ${STOOLKIT_DIR}/sencoder/SB_S_JIS0213.cpp
    ${STOOLKIT_DIR}/sencoder/SB_UCS2.cpp
    ${STOOLKIT_DIR}/sencoder/SB_UHC.cpp
    ${STOOLKIT_DIR}/sencoder/SB_UInput.cpp
    ${STOOLKIT_DIR}/sencoder/SB_UTF7.cpp
    ${STOOLKIT_DIR}/sencoder/SB_UTF8.cpp
    ${STOOLKIT_DIR}/sencoder/SB_X11_HZ.cpp
    ${STOOLKIT_DIR}/sencoder/SB_X11_JP.cpp
    ${STOOLKIT_DIR}/sencoder/SB_X11_KSC.cpp
    ${STOOLKIT_DIR}/sencoder/SBEncoder.cpp
)

# swindow — font handling + script processor (the shaping pipeline)
set(SWINDOW_DIR "${YUDIT_SRC_DIR}/swindow")
set(SWINDOW_SRCS
    ${SWINDOW_DIR}/SColor.cpp
    ${SWINDOW_DIR}/SPen.cpp
    ${SWINDOW_DIR}/SImage.cpp
    ${SWINDOW_DIR}/SFontTTF.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/src/SFontOTF_patched.cpp
    ${SWINDOW_DIR}/SFontCFF.cpp
    ${SWINDOW_DIR}/SScriptProcessor.cpp
)

# ── Our C API wrapper + GUI stubs ────────────────────────────────────────
set(WRAPPER_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/native/src/yudit_shaper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/src/YuditScriptProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/src/YuditParagraphShaper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/src/yudit_trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/native/src/yudit_gui_stubs.cpp
)

# ── Include paths ──────────────────────────────────────────────────────────
# Yudit expects `#include "stoolkit/XXX.h"` and `#include "swindow/XXX.h"`
# so we add the source root, plus the individual dirs for local headers.
set(YUDIT_INCLUDES
    ${YUDIT_SRC_DIR}
    ${STOOLKIT_DIR}
    ${SWINDOW_DIR}
    ${YUDIT_SRC_DIR}/svgicon
    ${CMAKE_CURRENT_SOURCE_DIR}/native/include
)

# ── Platform defines ───────────────────────────────────────────────────────
add_definitions(-DHAVE_LONG_LONG -DYUDIT_BUILDING_DLL)
if(WIN32)
    add_definitions(-DUSE_WINAPI)
    # winsock provides ntohl/ntohs used by SFontTTF
    link_libraries(ws2_32)
endif()

# ── Suppress noisy warnings in Yudit legacy code ──────────────────────────
if(MSVC)
    add_compile_options(/W0)
else()
    add_compile_options(-w)
endif()

# ── Build targets ──────────────────────────────────────────────────────────

set(ALL_SRCS ${STOOLKIT_SRCS} ${SWINDOW_SRCS} ${WRAPPER_SRCS})

if(YUDIT_BUILD_SHARED)
    add_library(yudit_shared SHARED ${ALL_SRCS})
    target_include_directories(yudit_shared PRIVATE ${YUDIT_INCLUDES})
    set_target_properties(yudit_shared PROPERTIES
        OUTPUT_NAME "yudit_shaper"
        PREFIX ""                # no "lib" prefix on Unix
        VERSION 0.3.0
        SOVERSION 0
    )
    if(WIN32)
        set_target_properties(yudit_shared PROPERTIES
            SUFFIX ".dll"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    elseif(APPLE)
        set_target_properties(yudit_shared PROPERTIES
            SUFFIX ".dylib"
            INSTALL_RPATH "@loader_path"
        )
    else()
        set_target_properties(yudit_shared PROPERTIES
            SUFFIX ".so"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
    # Link against socket/network libs on Unix for ntohl etc.
    # Only link nsl on older Linux systems that still have it
    if(NOT APPLE)
        find_library(NSL_LIB nsl)
        if(NSL_LIB)
            target_link_libraries(yudit_shared PRIVATE ${NSL_LIB})
        endif()
    endif()

    # Copy data files (.my) next to the shared library
    set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/native/vendor/yudit/data")
    add_custom_command(TARGET yudit_shared POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:yudit_shared>/../data"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${DATA_DIR}" "$<TARGET_FILE_DIR:yudit_shared>/../data"
        COMMENT "Copying data files..."
    )
endif()

if(YUDIT_BUILD_STATIC)
    add_library(yudit_static STATIC ${ALL_SRCS})
    target_include_directories(yudit_static PRIVATE ${YUDIT_INCLUDES})
    set_target_properties(yudit_static PROPERTIES
        OUTPUT_NAME "yudit_shaper"
    )
endif()
