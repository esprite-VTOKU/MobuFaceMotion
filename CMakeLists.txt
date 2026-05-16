cmake_minimum_required(VERSION 3.19)

# MobuFaceMotion - MotionBuilder plugin for iFacialMocap / FaceMotion3D.
#
# Picks the MotionBuilder version from PRODUCT_VERSION.txt (or -DMOBU_VERSION).
# Builds one shared library (.dll) and, if COPY_TO_PLUGINS is ON, copies it
# into the matching MotionBuilder plugins directory after each build.

project(MobuFaceMotion LANGUAGES CXX)

# --- MoBu version selection -------------------------------------------------

if(NOT DEFINED MOBU_VERSION)
    file(READ "${CMAKE_SOURCE_DIR}/PRODUCT_VERSION.txt" _mobu_ver_raw)
    string(STRIP "${_mobu_ver_raw}" MOBU_VERSION)
endif()

message(STATUS "MobuFaceMotion: building for MotionBuilder ${MOBU_VERSION}")

set(MOBU_ROOT
    "C:/Program Files/Autodesk/MotionBuilder ${MOBU_VERSION}"
    CACHE PATH "MotionBuilder install root")
set(OPENREALITY_ROOT
    "${MOBU_ROOT}/OpenRealitySDK"
    CACHE PATH "OpenRealitySDK root inside MotionBuilder")

# --- Compiler -----------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# --- OpenReality SDK target ---------------------------------------------------

include(cmake/OpenReality.cmake)

# --- Plugin subprojects -------------------------------------------------------

add_subdirectory(src/device_faceMotion)
