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

# RKNN
set(RKNN_DIR ${CMAKE_SOURCE_DIR}/3rd/rknn/)
include_directories(${RKNN_DIR}/include)
set(RKNN_RT_LIB ${RKNN_DIR}/aarch64/librknnrt.so)

# targets
include_directories(${CMAKE_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/chinese_chess)
add_executable(detector detector.cpp)
target_link_libraries(detector ${TinyRos_LIBRARIES} ${OpenCV_LIBS} ${YAML_CPP_LIBRARIES} ${RKNN_RT_LIB})
