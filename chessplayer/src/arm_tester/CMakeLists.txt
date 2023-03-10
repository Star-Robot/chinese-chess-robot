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

include_directories(${CMAKE_SOURCE_DIR}/src)
# targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/arm_tester)
file(GLOB ARM_SRC ${CMAKE_SOURCE_DIR}/src/common/arm_tools/*.cpp)
add_executable(arm_tester arm_tester.cpp ${ARM_SRC})
target_link_libraries(arm_tester ${TinyRos_LIBRARIES} ${OpenCV_LIBS} ${YAML_CPP_LIBRARIES})
