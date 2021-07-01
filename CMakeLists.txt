cmake_minimum_required(VERSION 2.8)
project(TinyRos)

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Find required GRPC package
LIST(APPEND CMAKE_INCLUDE_PATH "/export/cheguangfu/software/Grpc.sdk/include")
LIST(APPEND CMAKE_LIBRARY_PATH "/export/cheguangfu/software/Grpc.sdk/lib")
LIST(APPEND CMAKE_PROGRAM_PATH "/export/cheguangfu/software/Grpc.sdk/bin")
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

#OpenCV
# set(CMAKE_PREFIX_PATH "/export/cheguangfu/software/opencv-3.4/build/install/share/OpenCV/")
# find_package(OpenCV REQUIRED)

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
endforeach(TOOL)

