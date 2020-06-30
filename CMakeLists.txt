cmake_minimum_required(VERSION 3.15)
project(untitled)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_PREFIX_PATH "/home/coldmoon/Dependencies/")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildlibs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildlibs)
#set(ARCHIVE_OUTPUT_DIRECTORY buildlibs)
#set(LIBRARY_OUTPUT_DIRECTORY buildlibs)

include_directories(include/)
include_directories(VCLab/)

set(SOURCE_FILES UVCDepthIRViewer.cpp Common/Utill.cpp Common/Render.cpp VCLab/VCLab_utils.cpp VCLab/VCLab_utils.h)

#find_package(OpenCV REQUIRED)

add_executable(UVCDepthIRViewer ${SOURCE_FILES})
add_executable(UVCViewer UVCViewer.cpp Common/Utill.cpp Common/Render.cpp)
add_executable(DepthViewer DepthViewer.cpp Common/Utill.cpp Common/Render.cpp)
add_executable(IRView IRViewer.cpp Common/Utill.cpp Common/Render.cpp)
add_executable(UVCDepthIRViewer_DP UVCDepthIRViewer_DP.cpp VCLab/VCLab_utils.cpp VCLab/VCLab_utils.h VCLab/VCLab_Render.cpp VCLab/VCLab_Render.h)
#add_library(librender_util Utill.cpp Render.cpp)

find_library(IMI_NECT iminect libs)
find_library(IMI_CAMERA ImiCamera libs)
find_library(GL GL /usr/lib/x86_64-linux-gnu/)
find_library(GLUT glut /usr/lib/x86_64-linux-gnu/)

set(LIB_FILES ${IMI_NECT} ${IMI_CAMERA} ${GL} ${GLUT})

target_link_libraries(UVCDepthIRViewer ${LIB_FILES})
target_link_libraries(UVCViewer ${LIB_FILES})
target_link_libraries(DepthViewer ${LIB_FILES})
target_link_libraries(IRView ${LIB_FILES})
target_link_libraries(UVCDepthIRViewer_DP ${LIB_FILES})
