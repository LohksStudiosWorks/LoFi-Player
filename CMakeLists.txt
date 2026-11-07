cmake_minimum_required(VERSION 3.28)

project(lofi-player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include (cmake/installLibCurl.cmake)
include (cmake/installJson.cmake)
include (cmake/installLSWE.cmake)
include (cmake/installBatteryEmbed.cmake)

file(GLOB_RECURSE PROJECT_SOURCES
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES}) # put WIN32 to avoid terminal on Windows, if desired as such

get_target_property(INCLUDES lswe::lsw-engine INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Includes: ${INCLUDES}")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        lswe::lsw-engine
        nlohmann_json::nlohmann_json
        libcurl
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

b_embed(${PROJECT_NAME} "resources/calibri_fix.ttf")
b_embed(${PROJECT_NAME} "resources/template.png")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Use CMAKE_PREFIX_PATH to point find package to install folder on call!