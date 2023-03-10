cmake_minimum_required(VERSION 2.8)
project(astrabot)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -fPIC -w -fopenmp")
set(CMAKE_BUILD_TYPE "Release")

####### global #############
add_subdirectory(src/global/interactor)
add_subdirectory(src/global/mode_ctrl)
add_subdirectory(src/global/voice)

####### chinese_chess ##############
add_subdirectory(src/chinese_chess/game)
add_subdirectory(src/chinese_chess/camera)
add_subdirectory(src/chinese_chess/calibrator)
add_subdirectory(src/chinese_chess/detector)
add_subdirectory(src/chinese_chess/situator)
add_subdirectory(src/chinese_chess/arm)

####### arm_calibrator #####
add_subdirectory(src/arm_calibrator/)

####### arm_tester #########
add_subdirectory(src/arm_tester/)

##################
#####install######
##################
# Use this snippet *after* PROJECT(xxx):
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory" FORCE)

# scripts 
install(DIRECTORY "${CMAKE_SOURCE_DIR}/scripts/" # source directory
        DESTINATION "scripts" # target directory
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING # install only matched files
        PATTERN "*.sh" # select sh files
)

# hlbcore 
install(PROGRAMS "${CMAKE_SOURCE_DIR}/3rd/tiny_ros/bin/hlbcore"
        DESTINATION "./bin/global" 
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
        GROUP_EXECUTE GROUP_READ
)

# binaries
install(DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DESTINATION "./" 
        USE_SOURCE_PERMISSIONS
)

# DO NOT NEED:
# libraries 
install(DIRECTORY "${CMAKE_SOURCE_DIR}/libs"
        DESTINATION "./" 
        USE_SOURCE_PERMISSIONS
)

# resource 
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resource"
        DESTINATION "./" 
        USE_SOURCE_PERMISSIONS
)

