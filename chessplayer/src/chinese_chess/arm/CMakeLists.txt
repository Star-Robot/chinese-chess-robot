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
include_directories(${CMAKE_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/chinese_chess)
file(GLOB ARM_SRC ${CMAKE_SOURCE_DIR}/src/common/arm_tools/*.cpp)
add_executable(arm arm.cpp ${ARM_SRC})
target_link_libraries(arm ${TinyRos_LIBRARIES} ${YAML_CPP_LIBRARIES} ${OpenCV_LIBS})
