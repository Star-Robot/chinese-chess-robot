cmake_minimum_required(VERSION 2.8)
project(TinyRos)
set(PROJECT_VERSION 1.2.3.4)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -fPIC -o3 -w -fopenmp")
set(CMAKE_BUILD_TYPE "Release")

find_package(gRPC CONFIG REQUIRED)
message(STATUS "GRPC ver: " ${gRPC_VERSION})

# Find required protobuf package
find_package(protobuf CONFIG REQUIRED)
message(STATUS "Protbuf ver: " ${Protobuf_VERSION})

# Add Library target with protobuf sources
file(GLOB_RECURSE PROTO_FILES "*.proto")
add_library(tinyros_proto ${PROTO_FILES})
target_link_libraries(tinyros_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++
)

# Compile protobuf and grpc files in tinyros_proto target to cpp
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET tinyros_proto LANGUAGE cpp)
protobuf_generate(TARGET tinyros_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

include_directories(./src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/proto)
file(GLOB_RECURSE SOURCES . src/*.cpp)  

add_library(tinyros SHARED ${SOURCES})
target_link_libraries(tinyros PRIVATE tinyros_proto)

file(GLOB TOOLS exe/*.cpp)
foreach(TOOL ${TOOLS})
  message(STATUS "TARGET:" ${TOOL})
  get_filename_component(PREFIX ${TOOL} NAME_WE)
  add_executable(${PREFIX} ${TOOL})
  target_link_libraries(${PREFIX} PRIVATE tinyros_proto tinyros)
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

