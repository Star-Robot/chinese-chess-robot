# Yaml
find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

# TinyRos
set(TinyRos_DIR ${CMAKE_SOURCE_DIR}/3rd/tiny_ros/share/cmake/)
find_package(TinyRos REQUIRED)
include_directories(${TinyRos_INCLUDE_DIRS})

# FFMpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswresample
    libavutil
)

# ALSA
find_package(ALSA REQUIRED)                                    
include_directories(${ALSA_INCLUDE_DIRS})                  

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/global)

file(GLOB_RECURSE SRC *.cpp)
add_executable(voice ${SRC})
target_link_libraries(voice ${TinyRos_LIBRARIES} ${YAML_CPP_LIBRARIES} PkgConfig::LIBAV ${ALSA_LIBRARIES})
