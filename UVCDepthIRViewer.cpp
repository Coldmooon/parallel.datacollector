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

// window handle
SampleRender* g_pRender = NULL;
std::vector<SampleRender *> g_pRenders;

// device attributes
ImiDeviceAttribute* g_DeviceAttr = NULL; // Depth + IR
ImiCamAttribute*    g_pCameraAttr = NULL; // UVC

// device handle
ImiDeviceHandle * g_ImiDevice    = NULL;
ImiCameraHandle * g_cameraDevice = NULL;
uint32_t deviceCount = 0;
int32_t deviceCameraCount = 0;

// parse camera specified
std::vector<int8_t> cameras;

// stream handles
ImiStreamHandle * g_streams = NULL;

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
    if (NULL != g_cameraDevice) {
        for (int k = 0; k < deviceCount; ++k) {
            imiCamStopStream(g_cameraDevice[k]);
            imiCamClose(g_cameraDevice[k]);
            g_cameraDevice[k] = NULL;
        }
        delete[] g_cameraDevice;
    }

    //7.imiCloseStream()
    for (uint32_t num = 0; num < deviceCount; ++num) {
        if (NULL != g_streams[num]) {
            imiCloseStream(g_streams[num]);
            g_streams[num] = NULL;
        }
    }

    //8.imiCloseDevice()
    if (NULL != g_ImiDevice) {
        for (int k = 0; k < deviceCount; ++k) {
            imiCloseDevice(g_ImiDevice[k]);
            g_ImiDevice[k] = NULL;
        }
        delete[] g_ImiDevice;
    }

    //9.imiReleaseDeviceList
    if (NULL != g_DeviceAttr) {
        imiReleaseDeviceList(&g_DeviceAttr);
        //  g_DeviceAttr = NULL;
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
        return stop();
    }
    printf("ImiNect Init Success.\n");

    //2.imiGetDeviceList()
    imiGetDeviceList(&g_DeviceAttr, &deviceCount);
    if((deviceCount <= 0) || (NULL == g_DeviceAttr)) {
        printf("Get No Connected ImiDevice!\n");
        return stop();
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
            return stop();
        }
        printf("ImiDevice %d Opened.\n", k);
    }

    ret = getCamAttrList(&g_pCameraAttr, &deviceCameraCount);
    deviceCameraCount = deviceCameraCount > cameras.size()? cameras.size(): deviceCameraCount;
    if (ret != 0 || NULL == g_pCameraAttr || deviceCount != deviceCameraCount) {
        printf("getCamAttrList Failed! ret = %d\n", ret);
        return stop();
    }

    // open UVC camera
    g_cameraDevice = new ImiCameraHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        int8_t k = cameras[i];
        ret = imiCamOpenURI(g_pCameraAttr[k].uri, &g_cameraDevice[i]);
        if(0 != ret) {
            printf("Open UVC Camera %d Failed! ret = %d\n", k, ret);
            return stop();
        }
        printf("Open UVC Camera %d Success  %s\n", i, g_pCameraAttr[k].uri);
    }

    ImiCameraFrameMode pMode = {CAMERA_PIXEL_FORMAT_RGB888, 640,  480,  24};

    if(g_bisPortraitDevice) {
        pMode.resolutionX = 480;
        pMode.resolutionY = 640;
    }

    // open stream
    g_streams = new ImiStreamHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        ret = imiCamStartStream(g_cameraDevice[i], &pMode);
        if(0 != ret) {
            printf("Start Camera %d stream Failed! ret = %d\n", i, ret);
            return stop();
        }
        printf("Start Camera %d stream Success\n", i);

        //4.imiSetImageRegistration
        ret = imiSetImageRegistration(g_ImiDevice[i], IMI_TRUE);
        printf("imiSetImageRegistration ret = %d\n", ret);

        //5.imiOpenStream()
        ret = imiOpenStream(g_ImiDevice[i], IMI_DEPTH_IR_FRAME, NULL, NULL, &g_streams[i]);
        std::cout << imiGetLastError() << std::endl;
        if(0 != ret) {
            printf("Open Depth %d Stream Failed! ret = %d\n", i, ret);
            return stop();
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
    if (NULL != g_camera_Device) {
        if ( NULL == s_colorImage)
            std::cerr << "ERROR: no UVC pixel container provided..";

        ImiCameraFrame *pCamFrame = NULL;
        if (0 != imiCamReadNextFrame(g_camera_Device, &pCamFrame, 100))
            std::cerr << "ERROR: could not get frame.." << std::endl;

        if (NULL == pCamFrame)
            std::cerr << "Get frame failed.." << std::endl;

        memcpy((void *) s_colorImage, (const void *) pCamFrame->pData, pCamFrame->size);

        imiCamReleaseFrame(&pCamFrame);
    }
    else
        std::cerr << "Camber Handle is NULL.." << std::endl;
}

bool get_IR(ImiStreamHandle g_DepthIR_streams, RGB888Pixel * s_IRImage) {

    if (NULL != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if (NULL == s_IRImage)
            std::cerr << "ERROR: no IR pixel container provided.." << std::endl;

        ImiImageFrame *imiFrame = NULL;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            std::cerr << "ERROR: could not get IR frame.." << std::endl;

        if (NULL != imiFrame) {
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

    if (NULL != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if (NULL == s_depthImage)
            std::cerr << "ERROR: no Depth pixel container provided.." << std::endl;

        ImiImageFrame *imiFrame = NULL;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            std::cerr << "ERROR: could not get Depth frame.." << std::endl;

        if (NULL != imiFrame) {
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

    if (NULL != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if (NULL == s_depthImage)
            std::cerr << "ERROR: no Depth / IR pixel container provided.." << std::endl;

        ImiImageFrame *imiFrame = NULL;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            std::cerr << "ERROR: could not get Depth / IR frame.." << std::endl;

        if (NULL != imiFrame) {
            uint16_t *pde = (uint16_t *) imiFrame->pData;
            for (uint32_t i = 0; i < framesize; ++i) {
                s_depthImage[i].r = pde[i] >> 3;
                s_depthImage[i].g = s_depthImage[i].r;
                s_depthImage[i].b = s_depthImage[i].r;
            }
            if (NULL != s_IRImage) {
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

bool get_frames(ImiStreamHandle g_DepthIR_streams, ImiCameraHandle g_camera_Device, RGB888Pixel* s_colorImage, RGB888Pixel* s_depthImage, RGB888Pixel* s_IRImage=NULL) {

    if ( NULL != g_DepthIR_streams) {

        int framesize = g_width * g_height;
        if(NULL == s_depthImage && NULL == s_IRImage)
            std::cerr << "ERROR: no Depth / IR pixel container provided..";

        ImiImageFrame *imiFrame = NULL;
        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100))
            return false;

        if (NULL != imiFrame) {
            if (NULL != s_depthImage) {
                uint16_t *pde = (uint16_t *) imiFrame->pData;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_depthImage[i].r = pde[i] >> 3;
                    s_depthImage[i].g = s_depthImage[i].r;
                    s_depthImage[i].b = s_depthImage[i].r;
                }
            }

            if (NULL != s_IRImage) {
                uint16_t *pIR = (uint16_t *) imiFrame->pData + framesize;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_IRImage[i].r = pIR[i] >> 2;
                    s_IRImage[i].g = s_IRImage[i].r;
                    s_IRImage[i].b = s_IRImage[i].r;
                }
            }
        }
        else
            printf("Get Frame Failed..");

        imiReleaseFrame(&imiFrame);
        return true;
    }

    if (NULL != g_camera_Device) {
        if ( NULL == s_colorImage)
            std::cerr << "ERROR: no UVC pixel container provided..";

        ImiCameraFrame *pCamFrame = NULL;
        if (0 != imiCamReadNextFrame(g_camera_Device, &pCamFrame, 100))
            return false;

        if (NULL == pCamFrame)
            return false;

        memcpy((void *) s_colorImage, (const void *) pCamFrame->pData, pCamFrame->size);

        imiCamReleaseFrame(&pCamFrame);
        return true;
    }

    return false;
}

static bool receiving_rendering_single_a200(int camera_idx) {
    // method 1： define a 2-D array at runtime by 'new' method. Note: do not delete them in this function if declare them static varables.
    // static RGB888Pixel (* s_depthImages)[res_width * res_height] = new RGB888Pixel [deviceCount][g_width * g_height];
    // static RGB888Pixel (* s_colorImages)[res_width * res_height] = new RGB888Pixel [deviceCount][g_width * g_height];
    // static RGB888Pixel (* s_irImages)[res_width * res_height] = new RGB888Pixel [deviceCount][g_width * g_height];

    // method 2： define a 2-D array at runtime using smart pointer, which is a little unstable.
    std::unique_ptr<RGB888Pixel[]> s_colorImages_data;
    std::unique_ptr<RGB888Pixel[]> s_depthImages_data;
    std::unique_ptr<RGB888Pixel[]> s_irImages_data;

    std::unique_ptr<RGB888Pixel *[]> s_colorImages;
    std::unique_ptr<RGB888Pixel *[]> s_depthImages;
    std::unique_ptr<RGB888Pixel *[]> s_irImages;

    int8_t num_device = 1; // for temp test
    int8_t factor = 3; // can avoid signal aliasing ???? I guess.
    int resolution = g_width * g_height;
    int frame_length = resolution * factor;

    s_colorImages_data = std::make_unique<RGB888Pixel[]>(num_device * frame_length);
    s_depthImages_data = std::make_unique<RGB888Pixel[]>(num_device * frame_length);
    s_irImages_data = std::make_unique<RGB888Pixel[]>(num_device * frame_length);

    s_colorImages = std::make_unique<RGB888Pixel *[]>(num_device);
    s_depthImages = std::make_unique<RGB888Pixel *[]>(num_device);
    s_irImages = std::make_unique<RGB888Pixel *[]>(num_device);

    for (int i = 0; i < num_device; ++i) {
        s_colorImages[i] = &s_colorImages_data[i * frame_length];
        s_depthImages[i] = &s_depthImages_data[i * frame_length];
        s_irImages[i] = &s_irImages_data[i * frame_length];
    }
    // --------------------------------------- done ------------------------------------------------------

    bool s_bColorFrameOK = true; // don't declare it a static variable, or the whole streams will be dropped once some frame is lost.
    bool s_bDepth_IRFrameOK = true; // don't declare it a static variable, or the whole streams will be dropped once some frame is lost.
    static uint64_t s_depth_t = 0;
    static uint64_t s_color_t = 0;

    s_bDepth_IRFrameOK &= get_frames(g_streams[camera_idx], NULL, NULL, s_depthImages[0], s_irImages[0]);
    s_bColorFrameOK &= get_frames(NULL, g_cameraDevice[camera_idx], s_colorImages[0], NULL, NULL);

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
        printf("frame loss!");
        return false;
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

template<typename T>
void keyboardFun(T key, int32_t x, int32_t y)
{
    tasks = load_tasks("./config.txt");
    int8_t tmp = task_id;
    bool checknumber = false;

    switch (key) {
        case GLUT_KEY_UP:
            task_id++;
            if (task_id >= tasks.size()) task_id = 0;
            break;
        case GLUT_KEY_DOWN:
            task_id--;
            if (task_id < 0) task_id = tasks.size() - 1;
            break;
        case GLUT_KEY_LEFT:
            break;
        case GLUT_KEY_RIGHT:
            break;
        case 's':
            g_bSave = true;
            break;
        case 'q':
            Exit();
            break;
        default:
            checknumber = true;
            break;
    }

    if (checknumber && key >= 48 && key <= 57) {
        task_id = key - '0';
        if (task_id < 0 || task_id >= tasks.size()) {
            printf("ERROR: Please choose the task ID from %d to %d \n", 0, tasks.size() - 1);
            task_id = tmp;
        }
    }
    printf("Current task ID %d: %s \n", task_id, tasks[task_id].c_str());
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

    // multiple thread
//    std::vector<std::thread> workers;
//    for (int k = 0; k < deviceCount; ++k) {
//        workers.push_back(std::thread(receiving_rendering_single_a200, g_pRender, k));
//    }
//    for (auto &worker: workers) {
//        worker.join();
//    }

    // single thread
    int win = glutGetWindow();
    if (win - 1 < 0) {std::cerr << "Got negative window ID." << std::endl; Exit(); return false;}

    bool isdone = receiving_rendering_single_a200(win - 1);

    if (!isdone) {
        std::cerr << "Rendering wrong. Please check receiving_rendering_single_a200()." << std::endl;
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

//    std::vector<std::thread> workers;
//    for (int k = 0; k < deviceCount; ++k) {
//        workers.push_back(std::thread(assign_tasks, g_pRenders[k]));
//    }
//    for (auto &worker: workers) {
//        worker.join();
//    }

};

int main(int argc, char** argv)
{
    if (NULL != argv[1])
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
    XInitThreads();
    glutInit(&argc, argv);

    register_tasks(argc, argv);

//    glutIdleFunc(idle);
    glutMainLoop();
    return Exit();
}
