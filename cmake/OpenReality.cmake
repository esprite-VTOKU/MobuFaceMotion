# Imports the MotionBuilder OpenRealitySDK as an INTERFACE target named `fbsdk`.
#
# Expects OPENREALITY_ROOT and MOBU_ROOT to already be set in the parent scope
# (the root CMakeLists.txt handles that).

if(NOT EXISTS "${OPENREALITY_ROOT}/include/fbsdk/fbsdk.h")
    message(WARNING
        "OpenRealitySDK headers not found under '${OPENREALITY_ROOT}'. "
        "Configuration will continue, but the build will fail. "
        "Pass -DOPENREALITY_ROOT=<path> if your SDK lives elsewhere.")
endif()

set(_fbsdk_lib "${OPENREALITY_ROOT}/lib/x64/fbsdk.lib")
if(NOT EXISTS "${_fbsdk_lib}")
    # Some older SDKs use a 2014 subfolder; allow override.
    set(_fbsdk_lib "${OPENREALITY_ROOT}/lib/x64/2014/fbsdk.lib"
        CACHE FILEPATH "fbsdk import library" FORCE)
endif()

add_library(fbsdk INTERFACE)
target_include_directories(fbsdk INTERFACE "${OPENREALITY_ROOT}/include")
target_link_libraries(fbsdk INTERFACE "${_fbsdk_lib}")

# MoBu plugins are .dll's loaded into mobu.exe; the SDK headers expect
# the host to be Windows.
target_compile_definitions(fbsdk INTERFACE KARCH_ENV_WIN)
