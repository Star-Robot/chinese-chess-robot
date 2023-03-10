# OpenCV
find_package(OpenCV REQUIRED)
include_directories(OpenCV_INCLUDE_DIRS)

# Yaml
find_package(YAML-CPP REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

# TinyRos
set(TinyRos_DIR ${CMAKE_SOURCE_DIR}/3rd/tiny_ros/share/cmake/)
find_package(TinyRos REQUIRED)
include_directories(${TinyRos_INCLUDE_DIRS})

# targets
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/chinese_chess)
add_executable(camera camera.cpp)
target_link_libraries(camera ${TinyRos_LIBRARIES} ${OpenCV_LIBS} ${YAML_CPP_LIBRARIES})
