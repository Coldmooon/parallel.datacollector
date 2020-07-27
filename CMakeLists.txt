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
include_directories(.)
include_directories(./include)
include_directories(cameras/a200.imi/include/)

find_package(Threads REQUIRED)

# define variable of source files for the main target.
set(SOURCE_FILES UVCDepthIRViewer.cpp util/util.cpp UI/opengl/Render.cpp util/VCLab_utils.cpp util/VCLab_utils.h include/keyboard.hpp)

# enable this if using Opencv.
# find_package(OpenCV REQUIRED)

# define targets for the project.
add_executable(UVCDepthIRViewer ${SOURCE_FILES})

# complile .cpp source files as libraries (optional).
# add_library(librender Utill.cpp Render.cpp)

# find the path of libraries for the project.
find_library(IMI_NECT iminect cameras/a200.imi/libs)
find_library(IMI_CAMERA ImiCamera cameras/a200.imi/libs)
find_library(GL GL /usr/lib/x86_64-linux-gnu/)
find_library(GLUT glut /usr/local/lib/)
find_library(X11 X11 /usr/lib/)
find_library(UIOHOOK uiohook /usr/local/lib/)

# define library files variable for resuing.
set(LIB_FILES ${IMI_NECT} ${IMI_CAMERA} ${GL} ${GLUT})

# link the targets above to the libraries.
target_link_libraries(UVCDepthIRViewer ${LIB_FILES} Threads::Threads ${X11} ${UIOHOOK})