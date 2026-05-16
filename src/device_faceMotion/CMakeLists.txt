project(device_faceMotion LANGUAGES CXX)

file(GLOB SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(${PROJECT_NAME} SHARED ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE fbsdk ws2_32)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "device_faceMotion")

if(COPY_TO_PLUGINS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${MOBU_ROOT}/bin/x64/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${PROJECT_NAME}>"
                "${MOBU_ROOT}/bin/x64/plugins/${PROJECT_NAME}.dll"
        COMMENT "Installing ${PROJECT_NAME}.dll into ${MOBU_ROOT}/bin/x64/plugins")
endif()
