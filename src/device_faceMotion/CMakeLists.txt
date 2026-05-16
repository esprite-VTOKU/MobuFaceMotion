project(device_faceMotion LANGUAGES CXX)

file(GLOB SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} SHARED ${SRCS})

# MoBu's fbsdk headers concatenate ORSDK ## PRODUCT_VERSION to form an inline
# namespace (e.g. ORSDK2027). Without this define, symbols are emitted in
# namespace ORSDKPRODUCT_VERSION and fail to link against the SDK's libs.
target_compile_definitions(${PROJECT_NAME} PRIVATE PRODUCT_VERSION=${MOBU_VERSION})

target_link_libraries(${PROJECT_NAME} PRIVATE fbsdk ws2_32)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "device_faceMotion")

# Copying into <MoBu>/bin/x64/plugins requires admin privileges. Rather than
# silently failing every non-elevated build, expose installation as an opt-in
# target: `cmake --build <dir> --target install_plugin` from an elevated shell,
# or use tools/install.ps1 which self-elevates.
add_custom_target(install_plugin
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MOBU_ROOT}/bin/x64/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${MOBU_ROOT}/bin/x64/plugins/${PROJECT_NAME}.dll"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Installing ${PROJECT_NAME}.dll into ${MOBU_ROOT}/bin/x64/plugins"
    VERBATIM)
