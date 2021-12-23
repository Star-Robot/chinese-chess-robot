cmake_minimum_required(VERSION 2.8)
project(TinyRos)
set(PROJECT_VERSION 1.2.3.4)

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Find required GRPC package
list(APPEND CMAKE_INCLUDE_PATH "/export/cheguangfu/software/Grpc.sdk/include")
list(APPEND CMAKE_LIBRARY_PATH "/export/cheguangfu/software/Grpc.sdk/lib")
list(APPEND CMAKE_PROGRAM_PATH "/export/cheguangfu/software/Grpc.sdk/bin")
find_package(GRPC REQUIRED)
set(GRPC_LIBS ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY} ${GRPC_GRPC++_REFLECTION_LIBRARY})
message(STATUS "GRPC_LIBS: " ${GRPC_LIBS})

include_directories(${GRPC_INCLUDE_DIR})
# Find required protobuf package
find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
else()
    message(FATAL_ERROR "protobuf library is needed but cant be found")
endif()

file(GLOB_RECURSE TINYROS_PROTO "*.proto")

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${TINYROS_PROTO})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${TINYROS_PROTO})

include_directories(./src)
file(GLOB_RECURSE SOURCES . src/*.cpp)  
list(APPEND SOURCES ${PROTO_SRCS} ${GRPC_SRCS})

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -msse4.2 -fPIC -o3 -w -fopenmp -mavx2")
set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/)
set(LIBS ${OpenCV_LIBS} ${PROTOBUF_LIBRARIES} ${GRPC_LIBS})
add_library(tinyros SHARED ${SOURCES})
target_link_libraries(tinyros ${LIBS})

file(GLOB TOOLS exe/*.cpp)
foreach(TOOL ${TOOLS})
  message(STATUS "TARGET:" ${TOOL})
  get_filename_component(PREFIX ${TOOL} NAME_WE)
  add_executable(${PREFIX} ${TOOL} ${SOURCES})
  target_link_libraries(${PREFIX} ${LIBS})
  add_dependencies(${PREFIX} tinyros) 
  list(APPEND BINS ${PREFIX})
endforeach(TOOL)

##################
#####install######
##################
# Use this snippet *after* PROJECT(xxx):
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory" FORCE)
# Headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/" # source directory
        DESTINATION "include" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "node.hpp" # select header files
        PATTERN "parameter_server.hpp" # select header files
        PATTERN "publisher.hpp" # select header files
        PATTERN "publisher_proxy.hpp" # select header files
        PATTERN "subscriber.hpp" # select header files
        PATTERN "subscriber_proxy.hpp" # select header files
        PATTERN "std_msgs.hpp" # select header files
        PATTERN "util.hpp" # select header files
        PATTERN "proto" EXCLUDE
)
# libraries 
install(TARGETS tinyros
        EXPORT TinyRosTargets
        LIBRARY DESTINATION lib
)
# bins 
install(TARGETS ${BINS} 
        RUNTIME DESTINATION bin
)

# 将目标文件可导出文件
install(EXPORT TinyRosTargets DESTINATION share/cmake)
# 生成 TinyRosConfig.cmake
configure_file(${CMAKE_SOURCE_DIR}/TinyRosConfig.cmake.in 
               ${CMAKE_BINARY_DIR}/cmake/TinyRosConfig.cmake COPYONLY
)
install(FILES ${CMAKE_BINARY_DIR}/cmake/TinyRosConfig.cmake 
DESTINATION share/cmake
)

