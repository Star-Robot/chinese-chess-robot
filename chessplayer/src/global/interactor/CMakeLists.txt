# Yaml
find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

# TinyRos
set(TinyRos_DIR ${CMAKE_SOURCE_DIR}/3rd/tiny_ros/share/cmake/)
find_package(TinyRos REQUIRED)
include_directories(${TinyRos_INCLUDE_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/global)
add_executable(interactor interactor.cpp)
target_link_libraries(interactor ${TinyRos_LIBRARIES} ${YAML_CPP_LIBRARIES})
