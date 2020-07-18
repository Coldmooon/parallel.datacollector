cmake_minimum_required(VERSION 3.15)
project(vclab.datacollector.a200)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_PREFIX_PATH "/home/coldmoon/Dependencies/")
set(THREADS_PREFER_PTHREAD_FLAG ON)

# output complied libraries to the given directory.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildlibs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildlibs)
# set(ARCHIVE_OUTPUT_DIRECTORY buildlibs)
# set(LIBRARY_OUTPUT_DIRECTORY buildlibs)

# specify include directory.
include_directories(include/)
include_directories(VCLab/)

find_package(Threads REQUIRED)

# define variable of source files for the main target.
set(SOURCE_FILES UVCDepthIRViewer.cpp Common/Utill.cpp Common/Render.cpp VCLab/VCLab_utils.cpp VCLab/VCLab_utils.h)

# enable this if using Opencv.
# find_package(OpenCV REQUIRED)

# define targets for the project.
add_executable(UVCDepthIRViewer ${SOURCE_FILES})
add_executable(UVCViewer UVCViewer.cpp Common/Utill.cpp Common/Render.cpp)
add_executable(DepthViewer DepthViewer.cpp Common/Utill.cpp Common/Render.cpp)
add_executable(IRView IRViewer.cpp Common/Utill.cpp Common/Render.cpp)
add_executable(MultiUVC MultiUVC.cpp Common/Utill.cpp Common/Render.cpp)

# complile .cpp source files as libraries (optional).
# add_library(librender Utill.cpp Render.cpp)

# find the path of libraries for the project.
find_library(IMI_NECT iminect libs)
find_library(IMI_CAMERA ImiCamera libs)
find_library(GL GL /usr/lib/x86_64-linux-gnu/)
find_library(GLUT glut /usr/lib/x86_64-linux-gnu/)
find_library(X11 X11 /usr/lib/)

# define library files variable for resuing.
set(LIB_FILES ${IMI_NECT} ${IMI_CAMERA} ${GL} ${GLUT})

# link the targets above to the libraries.
target_link_libraries(UVCDepthIRViewer ${LIB_FILES} Threads::Threads X11)
target_link_libraries(UVCViewer ${LIB_FILES})
target_link_libraries(DepthViewer ${LIB_FILES})
target_link_libraries(IRView ${LIB_FILES})
target_link_libraries(MultiUVC ${LIB_FILES})