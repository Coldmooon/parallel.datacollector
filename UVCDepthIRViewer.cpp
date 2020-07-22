/********************************************************************************
Copyright (c) 2015 Huajie IMI Technology Co., Ltd.
All rights reserved.

@File Name		: main.cpp
@Author			: Wendell
@Date			: 2018-06-18
@Description	: read UVC+Depth_IR frame and view
@Version		: 1.0.0
@History		:
1.2018-06-18 Wendell Created file
********************************************************************************/

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/time.h>
#endif

#include <vector>
#include <iostream>
#include <GL/freeglut.h>
#include <memory>
#include <thread>

// Imi Head File
#include "ImiNect.h"
#include "ImiCamera.h"
#include "ImiCameraPrivate.h"

// UI
#include "Render.h"
#include <X11/Xlib.h>

// VCLab Utils
#include "VCLab/VCLab_utils.h"
#include "VCLab/keyboard.h"

// keyboard
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <uiohook.h>
#include <wchar.h>

// window handle
SampleRender* g_pRender = nullptr;
std::vector<SampleRender *> g_pRenders;

// device attributes
ImiDeviceAttribute* g_DeviceAttr = nullptr; // Depth + IR
ImiCamAttribute*    g_pCameraAttr = nullptr; // UVC

// device handle
ImiDeviceHandle * g_ImiDevice    = nullptr;
ImiCameraHandle * g_cameraDevice = nullptr;
uint32_t deviceCount = 0;
int32_t deviceCameraCount = 0;

// parse camera specified
std::vector<int8_t> cameras;

// stream handles
//ImiStreamHandle * g_streams = nullptr;
ImiStreamHandle g_streams[10];
uint32_t g_streamNum = 0;

// switch of frame sync
bool g_bNeedFrameSync = false;
const int32_t g_width  = 640;
const int32_t g_height = 480;

bool g_bisPortraitDevice = false;

// task
int8_t task_id = 0;
int n_frames_sampling = 10;
int n_frames = n_frames_sampling;
bool g_bSave = false;
std::vector<std::string> tasks;

void sleepMs (int32_t msecs) {
#ifdef _WIN32
    Sleep (msecs);
#else
    struct timespec short_wait;
    struct timespec remainder;
    short_wait.tv_sec = msecs / 1000;
    short_wait.tv_nsec = (msecs % 1000) * 1000 * 1000;
    nanosleep (&short_wait, &remainder);
#endif
}

int stop()
{
    // close Camera
    if (nullptr != g_cameraDevice) {
        for (int k = 0; k < deviceCount; ++k) {
            imiCamStopStream(g_cameraDevice[k]);
            imiCamClose(g_cameraDevice[k]);
            g_cameraDevice[k] = nullptr;
        }
        delete[] g_cameraDevice;
    }

    //7.imiCloseStream()
    for (uint32_t num = 0; num < deviceCount; ++num) {
        if (nullptr != g_streams[num]) {
            imiCloseStream(g_streams[num]);
            g_streams[num] = nullptr;
        }
    }

    //8.imiCloseDevice()
    if (nullptr != g_ImiDevice) {
        for (int k = 0; k < deviceCount; ++k) {
            imiCloseDevice(g_ImiDevice[k]);
            g_ImiDevice[k] = nullptr;
        }
        delete[] g_ImiDevice;
    }

    //9.imiReleaseDeviceList
    if (nullptr != g_DeviceAttr) {
        imiReleaseDeviceList(&g_DeviceAttr);
        //  g_DeviceAttr = nullptr;
        delete[] g_DeviceAttr;
    }

    // Depth + IR
    destroyCamAttrList();
    delete[] g_pCameraAttr; // UVC

    //10.imiDestroy()
    imiDestroy();

    return -1;
}

bool devices_init() {
    int ret = imiInitialize();
    if(0 != ret) {
        printf("ImiNect Init Failed! ret = %d\n", ret);
        stop();
        exit(-1);
    }
    printf("ImiNect Init Success.\n");

    //2.imiGetDeviceList()
    imiGetDeviceList(&g_DeviceAttr, &deviceCount);
    if((deviceCount <= 0) || (nullptr == g_DeviceAttr)) {
        printf("Get No Connected ImiDevice!\n");
        stop();
        exit(-1);
    }
    printf("Get %d Connected ImiDevice.\n", deviceCount);

    if (cameras.size() == 0) {
        for (int i = 0; i < deviceCount; ++i)
            cameras.push_back(i);
    }
    else
        deviceCount = deviceCount > cameras.size()? cameras.size(): deviceCount;

    //3.imiOpenDevice()
    g_ImiDevice = new ImiDeviceHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        int8_t k = cameras[i];
        ret = imiOpenDevice(g_DeviceAttr[k].uri, &g_ImiDevice[i], 0);
        if(0 != ret) {
            printf("Open ImiDevice %d Failed! ret = %d\n", k, ret);
            stop();
            exit(-1);
        }
        printf("ImiDevice %d Opened.\n", k);
    }

    ret = getCamAttrList(&g_pCameraAttr, &deviceCameraCount);
    deviceCameraCount = deviceCameraCount > cameras.size()? cameras.size(): deviceCameraCount;
    if (ret != 0 || nullptr == g_pCameraAttr || deviceCount != deviceCameraCount) {
        printf("getCamAttrList Failed! ret = %d\n", ret);
        stop();
        exit(-1);
    }

    // open UVC camera
    g_cameraDevice = new ImiCameraHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        int8_t k = cameras[i];
        ret = imiCamOpenURI(g_pCameraAttr[k].uri, &g_cameraDevice[i]);
        if(0 != ret) {
            printf("Open UVC Camera %d Failed! ret = %d\n", k, ret);
            stop();
            exit(-1);
        }
        printf("Open UVC Camera %d Success  %s\n", i, g_pCameraAttr[k].uri);
    }

    ImiCameraFrameMode pMode = {CAMERA_PIXEL_FORMAT_RGB888, 640,  480,  24};

    if(g_bisPortraitDevice) {
        pMode.resolutionX = 480;
        pMode.resolutionY = 640;
    }

    // open stream
//    g_streams = new ImiStreamHandle[deviceCount];
//    g_streams = new ImiStreamHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        ret = imiCamStartStream(g_cameraDevice[i], &pMode);
        if(0 != ret) {
            printf("Start Camera %d stream Failed! ret = %d\n", i, ret);
            stop();
            exit(-1);
        }
        printf("Start Camera %d stream Success\n", i);

        //4.imiSetImageRegistration
        ret = imiSetImageRegistration(g_ImiDevice[i], IMI_TRUE);
        printf("imiSetImageRegistration ret = %d\n", ret);

        //5.imiOpenStream()
        ret = imiOpenStream(g_ImiDevice[i], IMI_DEPTH_IR_FRAME, nullptr, nullptr, &g_streams[0]);
        std::cout << imiGetLastError() << std::endl;
        if(0 != ret) {
            printf("Open Depth %d Stream Failed! ret = %d\n", i, ret);
            stop();
            exit(-1);
        }
        printf("Open Depth %d Stream Success.\n", i);

        // imiCamSetFramesSync to set Color Depth_IR sync
        if(!g_bisPortraitDevice) {
            ret = imiCamSetFramesSync(g_cameraDevice[i], true);
            printf("Camera [%d] imiCamSetFramesSync = %d\n", i, ret);
        }
    }
    return true;
}

int Exit()
{
    stop();
    printf("------ exit ------\n");
    getchar();

    return 0;
}

bool drawtexts(SampleRender* g_pRender, std::vector<std::string> tasks, int8_t task_id) {
    std::string task_name = tasks[task_id];
    g_pRender->drawString("Task List: ", 1925, 40, 0.2, 0.4, 1);
    for (int x = 1940, y = 70, i = 0; i < tasks.size(); ++i, y += 30) {
        std::string text = "[" + std::to_string(i) + "] " + tasks[i];
        if (i == task_id)
            g_pRender->drawString(text.c_str(), x, y, 1, 1., 0);
        else
            g_pRender->drawString(text.c_str(), x, y, 0.2, 0.4, 1);
    }
    return true;
}

void get_color(ImiCameraHandle g_camera_Device, RGB888Pixel* s_colorImage) {
    if (nullptr != g_camera_Device) {
        if ( nullptr == s_colorImage)
            std::cerr << "ERROR: no UVC pixel container provided..";

        ImiCameraFrame *pCamFrame = nullptr;
        if (0 != imiCamReadNextFrame(g_camera_Device, &pCamFrame, 100))
            std::cerr << "ERROR: could not get frame.." << std::endl;

        if (nullptr == pCamFrame)
            std::cerr << "Get frame failed.." << std::endl;

        memcpy((void *) s_colorImage, (const void *) pCamFrame->pData, pCamFrame->size);

        imiCamReleaseFrame(&pCamFrame);
    }
    else
        std::cerr << "Camber Handle is nullptr.." << std::endl;
}

bool get_IR(ImiStreamHandle g_DepthIR_streams, RGB888Pixel * s_IRImage) {

    if (nullptr != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if (nullptr == s_IRImage)
            std::cerr << "ERROR: no IR pixel container provided.." << std::endl;

        ImiImageFrame *imiFrame = nullptr;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            std::cerr << "ERROR: could not get IR frame.." << std::endl;

        if (nullptr != imiFrame) {
            uint16_t *pIR = (uint16_t *) imiFrame->pData + framesize;
            for (uint32_t i = 0; i < framesize; ++i) {
                s_IRImage[i].r = pIR[i] >> 2;
                s_IRImage[i].g = s_IRImage[i].r;
                s_IRImage[i].b = s_IRImage[i].r;
            }
        }
        else
            std::cerr << "Get IR frame failed.." << std::endl;

        imiReleaseFrame(&imiFrame);
        return true;
    }
    else {
        std::cerr << "No IR stream given..." << std::endl;
        return false;
    }
}

bool get_depth(ImiStreamHandle g_DepthIR_streams, RGB888Pixel* s_depthImage) {

    if (nullptr != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if (nullptr == s_depthImage)
            std::cerr << "ERROR: no Depth pixel container provided.." << std::endl;

        ImiImageFrame *imiFrame = nullptr;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            std::cerr << "ERROR: could not get Depth frame.." << std::endl;

        if (nullptr != imiFrame) {
            uint16_t *pde = (uint16_t *) imiFrame->pData;
            for (uint32_t i = 0; i < framesize; ++i) {
                s_depthImage[i].r = pde[i] >> 3;
                s_depthImage[i].g = s_depthImage[i].r;
                s_depthImage[i].b = s_depthImage[i].r;
            }
        }
        else
            std::cerr << "Get Depth frame failed.." << std::endl;

        imiReleaseFrame(&imiFrame);
        return true;
    }
    else {
        std::cerr << "No Depth stream given..." << std::endl;
        return false;
    }
}

bool get_depthIR(ImiStreamHandle g_DepthIR_streams, RGB888Pixel* s_depthImage, RGB888Pixel * s_IRImage) {

    if (nullptr != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if (nullptr == s_depthImage)
            std::cerr << "ERROR: no Depth / IR pixel container provided.." << std::endl;

        ImiImageFrame *imiFrame = nullptr;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            std::cerr << "ERROR: could not get Depth / IR frame.." << std::endl;

        if (nullptr != imiFrame) {
            uint16_t *pde = (uint16_t *) imiFrame->pData;
            for (uint32_t i = 0; i < framesize; ++i) {
                s_depthImage[i].r = pde[i] >> 3;
                s_depthImage[i].g = s_depthImage[i].r;
                s_depthImage[i].b = s_depthImage[i].r;
            }
            if (nullptr != s_IRImage) {
                uint16_t *pIR = (uint16_t *) imiFrame->pData + framesize;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_IRImage[i].r = pIR[i] >> 2;
                    s_IRImage[i].g = s_IRImage[i].r;
                    s_IRImage[i].b = s_IRImage[i].r;
                }
            }
        }
        else
            std::cerr << "Get Depth / IR frame failed.." << std::endl;

        imiReleaseFrame(&imiFrame);
        return true;
    }
    else {
        std::cerr << "No Depth/IR stream given..." << std::endl;
        return false;
    }
}
int count = 0;
bool get_frames(ImiStreamHandle g_DepthIR_streams, ImiCameraHandle g_camera_Device, RGB888Pixel* s_colorImage, RGB888Pixel* s_depthImage, RGB888Pixel* s_IRImage=nullptr) {

    if ( nullptr != g_DepthIR_streams) {
        printf("count : %d\n", count);
        int framesize = g_width * g_height;
        if(nullptr == s_depthImage && nullptr == s_IRImage) {
            std::cerr << "ERROR: no Depth / IR pixel container provided..";
            return false;
        }

        ImiImageFrame *imiFrame = nullptr;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100)) {
            std::cerr << "imiReadNextFrame() function reported an error." << std::endl;
            return false;
        }

        if (nullptr != imiFrame) {
            if (nullptr != s_depthImage) {
                uint16_t *pde = (uint16_t *) imiFrame->pData;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_depthImage[i].r = pde[i] >> 3;
                    s_depthImage[i].g = s_depthImage[i].r;
                    s_depthImage[i].b = s_depthImage[i].r;
                }
            }
            if (nullptr != s_IRImage) {
                uint16_t *pIR = (uint16_t *) imiFrame->pData + framesize;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_IRImage[i].r = pIR[i] >> 2;
                    s_IRImage[i].g = s_IRImage[i].r;
                    s_IRImage[i].b = s_IRImage[i].r;
                }
            }
        }
        else {
            std::cerr << "Failed to receive the next frame in imiReadNextFrame() function." << std::endl;
            return false;
        }

        imiReleaseFrame(&imiFrame);
        count ++;

        return true;
    }

    if (nullptr != g_camera_Device) {
        if ( nullptr == s_colorImage) {
            std::cerr << "ERROR: no UVC pixel container provided..";
            return false;
        }

        ImiCameraFrame *pCamFrame = nullptr;
        if (0 != imiCamReadNextFrame(g_camera_Device, &pCamFrame, 100)) {
            std::cerr << "imiCamReadNextFrame() function reported an error." << std::endl;
            return false;
        }

        if (nullptr != pCamFrame)
            memcpy((void *) s_colorImage, (const void *) pCamFrame->pData, pCamFrame->size);
        else {
            std::cerr << "Failed to receive the next frame in imiCamReadNextFrame() function." << std::endl;
            return false;
        }

        imiCamReleaseFrame(&pCamFrame);
        return true;
    }

    return false;
}

static bool receiving_rendering_single_a200(int camera_idx) {

    const int8_t factor = 3; // can avoid signal aliasing ???? I guess.
    const int resolution = g_width * g_height;
    const int frame_length = resolution * factor;

    // method 1： define a 2-D array at runtime by 'new' method. Note: do not delete them in this function if declare them static varables.
     static RGB888Pixel (* s_depthImages)[frame_length] = new RGB888Pixel [deviceCount][frame_length];
     static RGB888Pixel (* s_colorImages)[frame_length] = new RGB888Pixel [deviceCount][frame_length];
     static RGB888Pixel (* s_irImages)[frame_length] = new RGB888Pixel [deviceCount][frame_length];

    // method 2： define a 2-D array at runtime using smart pointer, which is a little unstable.
//    std::unique_ptr<RGB888Pixel[]> s_colorImages_data;
//    std::unique_ptr<RGB888Pixel[]> s_depthImages_data;
//    std::unique_ptr<RGB888Pixel[]> s_irImages_data;
//
//    std::unique_ptr<RGB888Pixel *[]> s_colorImages;
//    std::unique_ptr<RGB888Pixel *[]> s_depthImages;
//    std::unique_ptr<RGB888Pixel *[]> s_irImages;

//    s_colorImages_data = std::make_unique<RGB888Pixel[]>(deviceCount * frame_length);
//    s_depthImages_data = std::make_unique<RGB888Pixel[]>(deviceCount * frame_length);
//    s_irImages_data = std::make_unique<RGB888Pixel[]>(deviceCount * frame_length);
//
//    s_colorImages = std::make_unique<RGB888Pixel *[]>(deviceCount);
//    s_depthImages = std::make_unique<RGB888Pixel *[]>(deviceCount);
//    s_irImages = std::make_unique<RGB888Pixel *[]>(deviceCount);
//
//    for (int i = 0; i < deviceCount; ++i) {
//        s_colorImages[i] = &s_colorImages_data[i * frame_length];
//        s_depthImages[i] = &s_depthImages_data[i * frame_length];
//        s_irImages[i] = &s_irImages_data[i * frame_length];
//    }
    // --------------------------------------- done ------------------------------------------------------

    bool s_bColorFrameOK = true; // don't declare it a static variable, or the whole streams will be dropped once some frame is lost.
    bool s_bDepth_IRFrameOK = true; // don't declare it a static variable, or the whole streams will be dropped once some frame is lost.
    static uint64_t s_depth_t = 0;
    static uint64_t s_color_t = 0;

    std::cout << "camera_Idx: " << camera_idx << std::endl;
    s_bDepth_IRFrameOK &= get_frames(g_streams[0], nullptr, nullptr, s_depthImages[0], s_irImages[0]);
    s_bColorFrameOK &= get_frames(nullptr, g_cameraDevice[camera_idx], s_colorImages[0], nullptr, nullptr);

    SampleRender *g_pRender = g_pRenders[camera_idx];

    if (s_bColorFrameOK && s_bDepth_IRFrameOK) {

        WindowHint hint(0, 0, g_width, g_height);

        // draw Color
        hint.x = 0;
        g_pRender->draw((uint8_t *) s_colorImages[0], g_width * g_height * 3, hint);
        hint.x += g_width;
        hint.w = g_width;
        hint.h = g_height;
        // draw Depth
        g_pRender->draw((uint8_t *) s_depthImages[0], g_width * g_height * 3, hint);
        hint.x += g_width;
        hint.w = g_width;
        hint.h = g_height;
        // draw IR
        g_pRender->draw((uint8_t *) s_irImages[0], g_width * g_height * 3, hint);

//            if (g_bSave) {
//                save((uint16_t *) pCamFrame->pData, s_colorImages[k], pCamFrame->size, pCamFrame->width, pCamFrame->height,
//                     task_name + "_UVC_frame" + "_camera[" + std::to_string(k) + "]" + std::to_string(n_frames));
//                save((uint16_t *) imiFrame->pData, s_depthImages[k], imiFrame->size, imiFrame->width, imiFrame->height,
//                     task_name + "_Depth_frame" + "_camera[" + std::to_string(k) + "]" + std::to_string(n_frames));
//                save((uint16_t *) imiFrame->pData + nFrameSize, s_irImages[k], imiFrame->size, imiFrame->width,
//                     imiFrame->height, task_name + "_IR_frame" + "_camera[" + std::to_string(k) + "]" + std::to_string(n_frames));
//
//                n_frames -= 1;
//                if (n_frames == 0) {
//                    g_bSave = false;
//                    n_frames = n_frames_sampling;
//                }
//            }
        drawtexts(g_pRender, tasks, task_id);
//        g_pRender->drawCursorXYZValue(&imiFrame[0]);
    }
    else {
        std::cerr << "\nFrame loss!" << std::endl;
        return false;
    }

    return true;
}

template<typename T>
void keyboardFun(T key, int32_t x, int32_t y)
{
//    tasks = load_tasks("./config.txt");
//    int8_t tmp = task_id;
//    bool checknumber = false;
//
//    switch (key) {
//        case GLUT_KEY_UP:
//            task_id++;
//            if (task_id >= tasks.size()) task_id = 0;
//            break;
//        case GLUT_KEY_DOWN:
//            task_id--;
//            if (task_id < 0) task_id = tasks.size() - 1;
//            break;
//        case GLUT_KEY_LEFT:
//            break;
//        case GLUT_KEY_RIGHT:
//            break;
//        case 's':
//            g_bSave = true;
//            break;
//        case 'q':
//            Exit();
//            break;
//        default:
//            checknumber = true;
//            break;
//    }
//
//    if (checknumber && key >= 48 && key <= 57) {
//        task_id = key - '0';
//        if (task_id < 0 || task_id >= tasks.size()) {
//            printf("ERROR: Please choose the task ID from %d to %d \n", 0, tasks.size() - 1);
//            task_id = tmp;
//        }
//    }
//    printf("Current task ID %d: %s \n", task_id, tasks[task_id].c_str());
}

bool parse_cameras(const std::string & args, std::vector<int8_t> & cameras) {
    for (char const &c: args) {
        if(isdigit(c)) {
            int id = c - '0';
            cameras.push_back(id);
        }
        else if (c == ',')
            continue;
        else {
            std::cerr << "Format: 0,1,2,3,4 ..." << std::endl;
            exit(-1);
        }
    }
}

static bool assign_tasks(SampleRender* g_pRender) {
    // single thread
    int win = glutGetWindow();
    if (win - 1 < 0) {std::cerr << "Got negative window ID." << std::endl; Exit(); return false;}

    bool isdone = receiving_rendering_single_a200(win - 1);

    if (!isdone) {
        std::cerr << "\nRendering wrong. Please check receiving_rendering_single_a200()." << std::endl;
        Exit();
        return false;
    }
    glutSwapBuffers();
    glutPostRedisplay();
    g_pRender->initViewPort();

    return true;
}

void register_tasks(int argc, char** argv) {
    for (int i = 0; i < deviceCount; ++i){
        SampleRender * pRender = new SampleRender(std::to_string(i).c_str(), g_width * 3 + 300, g_height + 10);
        pRender->m_camera_id = cameras[i];

        pRender->init(argc, argv); // init and create a window

        pRender->setDataCallback_multithread(assign_tasks);

        pRender->setKeyCallback(keyboardFun<unsigned char>); // normal and functional keys
        g_pRenders.push_back(pRender);
    }
}

void idle () {
    for (int i = 0; i < g_pRenders.size(); ++i ) {
        glutSetWindow(g_pRenders[i]->m_glWin);
        glutPostRedisplay();
        g_pRenders[i]->initViewPort();
    }
};

void hotkeys(uiohook_event * const event) {
    char buffer[256] = { 0 };
    size_t length = snprintf(buffer, sizeof(buffer),
    "id=%i,when=%" PRIu64 ",mask=0x%X",
            event->type, event->time, event->mask);

    switch (event->type) {

        case EVENT_KEY_PRESSED:
            // If the escape key is pressed, naturally terminate the program.
            if (event->data.keyboard.keycode == VC_ESCAPE) {
                int status = hook_stop();
                switch (status) {
                    // System level errors.
                    case UIOHOOK_ERROR_OUT_OF_MEMORY:
                        logger_proc(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
                        break;

                    case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
                        // NOTE This is the only platform specific error that occurs on hook_stop().
                        logger_proc(LOG_LEVEL_ERROR, "Failed to get XRecord context. (%#X)", status);
                        break;

                        // Default error.
                    case UIOHOOK_FAILURE:
                    default:
                        logger_proc(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
                        break;
                }
            }
            else if (event->data.keyboard.keycode == VC_S)
                g_bSave = true;
            else if (event->data.keyboard.keycode == VC_UP) {
                task_id++;
                if (task_id >= tasks.size()) task_id = 0;
            }
            else if (event->data.keyboard.keycode == VC_DOWN) {
                task_id--;
                if (task_id < 0) task_id = tasks.size() - 1;
            }
//            else
//                checknumber = true;
//
            if (event->data.keyboard.keycode >= VC_1 && event->data.keyboard.keycode <= VC_9) {
                int tmp = task_id;
                task_id = event->data.keyboard.keycode - VC_1 + 1;
                if (task_id < 0 || task_id >= tasks.size()) {
                    printf("ERROR: Please choose the task ID from %d to %d \n", 0, tasks.size() - 1);
                    task_id = tmp;
                }
            }
            printf("Current task ID %d: %s \n", task_id, tasks[task_id].c_str());

        case EVENT_KEY_RELEASED:
            snprintf(buffer + length, sizeof(buffer) - length,
                     ",keycode=%u,rawcode=0x%X",
                     event->data.keyboard.keycode, event->data.keyboard.rawcode);
            break;

        case EVENT_KEY_TYPED:
            snprintf(buffer + length, sizeof(buffer) - length,
                     ",keychar=%lc,rawcode=%u",
                     (wint_t) event->data.keyboard.keychar,
                     event->data.keyboard.rawcode);
            break;

        case EVENT_MOUSE_PRESSED:
        case EVENT_MOUSE_RELEASED:
        case EVENT_MOUSE_CLICKED:
        case EVENT_MOUSE_MOVED:
        case EVENT_MOUSE_DRAGGED:
            snprintf(buffer + length, sizeof(buffer) - length,
                     ",x=%i,y=%i,button=%i,clicks=%i",
                     event->data.mouse.x, event->data.mouse.y,
                     event->data.mouse.button, event->data.mouse.clicks);
            break;

        case EVENT_MOUSE_WHEEL:
            snprintf(buffer + length, sizeof(buffer) - length,
                     ",type=%i,amount=%i,rotation=%i",
                     event->data.wheel.type, event->data.wheel.amount,
                     event->data.wheel.rotation);
            break;

        default:
            break;
    }

    fprintf(stdout, "%s\n",     buffer);
}

void keyboardIO_thread() {
    printf("\nthis is keyboard threading\n");

    keyboard_monitor(hotkeys);
}

void cameraIO_thread(int argc, char** argv) {
    XInitThreads();
    glutInit(&argc, argv);

    register_tasks(argc, argv);

//    glutIdleFunc(idle);
    glutMainLoop();
    std::cout << "Exited glutMainLoop." << std::endl;
    Exit();
}

int main(int argc, char** argv)
{
    if (nullptr != argv[1])
        parse_cameras(argv[1], cameras);

    if(!devices_init()) {
        printf("------ exit ------\n");
        getchar();
    }

    if (deviceCount == 0) {std::cerr << "Error Counting devices." << std::endl; exit(-1);}
    tasks = load_tasks("./config.txt");

    // Glut pipline:
    // 1. initialize the window properties --> 2. create the window --> 3. register the callback function when an event, e.g., a mouse move, occurs.
    // That is to tell GLUT which function to call back --> 4. enter the event processing loop
    // For each event type, GLUT provides a specific function to register the call back function.
    // Registering a callback function means to tell GLUT that it should use the function, e.g., needImage(), we just wrote/created for the rendering.
    // GLUT will call the function you choose whenever rendering is required.
    // Note: GLUT functions, as C functions, can not invoke a callback with instance specific information. So avoid registering memeber function as callback.

    printf("\n Go into multi threading. \n");
    std::vector<std::thread> workers;
    for (int k = 0; k < 2; ++k) {
        if (k == 0)
            workers.push_back(std::thread(cameraIO_thread, argc, argv));
//        else
//            workers.push_back(std::thread(keyboardIO_thread));
    }
    for (auto &worker: workers) {
        worker.join();
    }
    return 0;
}
