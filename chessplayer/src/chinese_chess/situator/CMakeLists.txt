# Yaml
find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

# TinyRos
set(TinyRos_DIR ${CMAKE_SOURCE_DIR}/3rd/tiny_ros/share/cmake/)
find_package(TinyRos REQUIRED)
include_directories(${TinyRos_INCLUDE_DIRS})

# targets
include_directories(${CMAKE_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/chinese_chess)
file(GLOB SRC *.cpp)
add_executable(situator ${SRC})
target_link_libraries(situator ${TinyRos_LIBRARIES} ${YAML_CPP_LIBRARIES})
