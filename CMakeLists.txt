cmake_minimum_required(VERSION 3.17)
project(parallel.datacollector)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_VERBOSE_MAKEFILE on)
# set librariy search path
set(CMAKE_PREFIX_PATH "/home/coldmoon/Dependencies/")

# output complied libraries to the given directory.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildlibs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/buildlibs)
# set(ARCHIVE_OUTPUT_DIRECTORY buildlibs)
# set(LIBRARY_OUTPUT_DIRECTORY buildlibs)

# specify include directory.
include_directories(.)
include_directories(./include)
include_directories(cameras/)

find_package(Threads REQUIRED)
# enable this if using OpenCV. set CMAKE_PREFIX_PATH to OpenCV install path. 
find_package(OpenCV REQUIRED)

# define source files for the main target.
set(SOURCE_FILES UVCDepthIRViewer.cpp util/util.cpp UI/opengl/Render.cpp util/VCLab_utils.cpp util/VCLab_utils.h include/keyboard.hpp modules/a200.hpp)

# define targets for the project.
add_executable(UVCDepthIRViewer ${SOURCE_FILES})
add_executable(T3S T3S.cpp include/keyboard.hpp util/VCLab_utils.cpp)

# complile .cpp source files as libraries (optional).
# add_library(librender Utill.cpp Render.cpp)

# find library path for the project.
find_library(IMI_NECT iminect cameras/a200.imi/libs)
find_library(IMI_CAMERA ImiCamera cameras/a200.imi/libs)
find_library(GL GL /usr/lib/x86_64-linux-gnu/)
find_library(GLUT glut /usr/local/lib/)
find_library(X11 X11 /usr/lib/)
find_library(UIOHOOK uiohook /usr/local/lib/)

find_library(SIMPLE Simple cameras/T3S.infiray/libs)
find_library(THER thermometry cameras/T3S.infiray/libs)

# define variable for the libraries.
set(LIB_FILES ${IMI_NECT} ${IMI_CAMERA} ${GL} ${GLUT})

# link the targets to the libraries.
# add stdc++fs if using c++14 or less.
target_link_libraries(UVCDepthIRViewer ${LIB_FILES} ${X11} ${UIOHOOK} Threads::Threads /lib/x86_64-linux-gnu/libuuid.so.1)
target_link_libraries(T3S ${OpenCV_LIBS} ${SIMPLE} ${THER} ${UIOHOOK} Threads::Threads /lib/x86_64-linux-gnu/libuuid.so.1)
