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

// Imi Head File
#include "ImiNect.h"
#include "ImiCamera.h"
#include "ImiCameraPrivate.h"

// UI
#include "Render.h"

// VCLab
#include "VCLab/VCLab_utils.h"

// window handle
SampleRender* g_pRender = NULL;

ImiDeviceAttribute* g_DeviceAttr = NULL;

// device handle
ImiDeviceHandle g_ImiDevice    = NULL;
ImiCameraHandle g_cameraDevice = NULL;
int camera_index = 0;

// stream handles
ImiStreamHandle g_streams[10];
uint32_t g_streamNum = 0;
static int countDepth = 0 ;
static int countUVC = 0 ;

// switch of frame sync
bool g_bNeedFrameSync = false;
int32_t g_width  = 640;
int32_t g_height = 480;

bool g_bisPortraitDevice = false;

// task
int8_t task_id = 0;
int n_frames_sampling = 10;
int n_frames = n_frames_sampling;
bool g_bSave = false;
std::vector<std::string> tasks;

void sleepMs (int32_t msecs)
{
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
    if(NULL != g_cameraDevice)
    {
        imiCamStopStream(g_cameraDevice);
        imiCamClose(g_cameraDevice);
        g_cameraDevice = NULL;
    }

    //7.imiCloseStream()
    for(uint32_t num = 0; num < g_streamNum; ++num)
    {
        if(NULL != g_streams[num])
        {
            imiCloseStream(g_streams[num]);
            g_streams[num] = NULL;
        }
    }

    //8.imiCloseDevice()
    if(NULL != g_ImiDevice)
    {
        imiCloseDevice(g_ImiDevice);
        g_ImiDevice = NULL;
    }

    //9.imiReleaseDeviceList
    if(NULL != g_DeviceAttr)
    {
        imiReleaseDeviceList(&g_DeviceAttr);
        g_DeviceAttr = NULL;
    }

    //10.imiDestroy()
    imiDestroy();

    return -1;
}


int start()
{
    g_streamNum = 0;

    countUVC = 0;

    countDepth = 0;


    int ret = imiInitialize();
    if(0 != ret)
    {
        printf("ImiNect Init Failed! ret = %d\n", ret);
        return stop();
    }
    printf("ImiNect Init Success.\n");

    //2.imiGetDeviceList()
    uint32_t deviceCount = 0;
    imiGetDeviceList(&g_DeviceAttr, &deviceCount);
    if((deviceCount <= 0) || (NULL == g_DeviceAttr))
    {
        printf("Get No Connected ImiDevice!\n");
        return stop();
    }
    printf("Get %d Connected ImiDevice.\n", deviceCount);

    //3.imiOpenDevice()
    ret = imiOpenDevice(g_DeviceAttr[camera_index].uri, &g_ImiDevice, 0);
    if(0 != ret)
    {
        printf("Open ImiDevice Failed! ret = %d\n", ret);
        return stop();
    }
    printf("ImiDevice Opened.\n");


    if(g_DeviceAttr[camera_index].productId == 0x0304 || g_DeviceAttr[camera_index].productId == 0x0303 || g_DeviceAttr[camera_index].productId == 0x0307 || g_DeviceAttr[camera_index].productId == 0x0308)
    {
        g_bisPortraitDevice = true;
        g_width = 480;
        g_height = 640;
    }

    // open UVC camera
    ret = imiCamOpen(&g_cameraDevice);
    if(0 != ret)
    {
        printf("Open UVC Camera Failed! ret = %d\n", ret);
        return stop();
    }
    printf("Open UVC Camera Success\n");

    ImiCameraFrameMode pMode =   {CAMERA_PIXEL_FORMAT_RGB888, 640,  480,  24};

    if(g_bisPortraitDevice)
    {
        pMode.resolutionX = 480;
        pMode.resolutionY = 640;
    }

    // open camera stream
    ret = imiCamStartStream(g_cameraDevice, &pMode);
    if(0 != ret)
    {
        printf("Start Camera stream Failed! ret = %d\n", ret);
        return stop();
    }
    printf("Start Camera stream Success\n");


    //4.imiSetImageRegistration
    ret = imiSetImageRegistration(g_ImiDevice, IMI_TRUE);
    printf("imiSetImageRegistration ret = %d\n", ret);

    //5.imiOpenStream()
    ret = imiOpenStream(g_ImiDevice, IMI_DEPTH_IR_FRAME, NULL, NULL, &g_streams[g_streamNum++]);
    std::cout << imiGetLastError() << std::endl;
    if(0 != ret)
    {
        printf("Open Depth Stream Failed! ret = %d\n", ret);
        return stop();
    }
    printf("Open Depth Stream Success.\n");

    // imiCamSetFramesSync to set Color Depth_IR sync

    if(!g_bisPortraitDevice)
    {
        ret = imiCamSetFramesSync(g_cameraDevice, true);
    }


    printf("imiCamSetFramesSync = %d\n", ret);

    return 0;
}

// window callback, called by SampleRender::display()
static bool needImage(void* pData)
{
    static RGB888Pixel s_depthImage[1920 * 1080];
    static RGB888Pixel s_irImage[1920 * 1080];
    static RGB888Pixel s_colorImage[1920 * 1080];
    static bool s_bColorFrameOK    = false;
    static bool s_bDepth_IRFrameOK = false;
    static uint64_t s_depth_t      = 0;
    static uint64_t s_color_t      = 0;
    ImiImageFrame* imiFrame = NULL;
    ImiCameraFrame* pCamFrame = NULL;
    uint32_t nFrameSize = g_width * g_height;

//    std::copy(tasks.begin(), tasks.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

    if(!s_bDepth_IRFrameOK)
    {
        int32_t avStreamIndex = 0;

        // frame read.
        if(0 != imiReadNextFrame(g_streams[avStreamIndex], &imiFrame, 200))
        {
            countDepth++;

            if(countDepth > 100)
            {
                printf("in 100 times not get Depth Frame, restart stream =  %d\n");

                stop();

                sleepMs(2000);

                start();
            }

            return false;
        }

        if(NULL == imiFrame)
        {
            return false;
        }


        /*if(g_bisPortraitDevice)
        {
        nFrameSize = g_width * 480;
        }*/

        // point to Depth data buffer
        uint16_t* pde = (uint16_t*)imiFrame->pData;
        for(uint32_t i = 0; i < nFrameSize; ++i)
        {
            s_depthImage[i].r = pde[i] >> 3;
            s_depthImage[i].g = s_depthImage[i].r;
            s_depthImage[i].b = s_depthImage[i].r;
        }

        // point to IR data buffer
        uint16_t* pIR = (uint16_t*)imiFrame->pData + nFrameSize;
        for(uint32_t i = 0; i < nFrameSize; ++i)
        {
            s_irImage[i].r = pIR[i] >> 2;
            s_irImage[i].g = s_irImage[i].r;
            s_irImage[i].b = s_irImage[i].r;
        }

        s_depth_t = imiFrame->timeStamp;
        s_bDepth_IRFrameOK = true;
    }

    if(!s_bColorFrameOK)
    {
        if(0 != imiCamReadNextFrame(g_cameraDevice, &pCamFrame, 200))
        {
            countUVC++;

            if(countUVC > 100)
            {
                printf("in 100 times not get UVC Frame, restart stream =  %d\n");

                stop();

                sleepMs(2000);

                start();
            }

            return false;
        }

        if(NULL == pCamFrame)
        {
            return false;
        }

        memcpy((void*)&s_colorImage, (const void*)pCamFrame->pData, pCamFrame->size);

        s_color_t = pCamFrame->timeStamp;
        s_bColorFrameOK = true;
    }

    if(s_bColorFrameOK && s_bDepth_IRFrameOK)
    {
        if(g_bNeedFrameSync)
        {
            int64_t delta = (s_depth_t - s_color_t);
            delta /= 1000; //ms

            if(delta < -20)
            {
                s_bDepth_IRFrameOK = false; //drop Depth_IR frame
                return true;
            }
            else if(delta > 10)
            {
                s_bColorFrameOK = false; //drop Color frame
                return true;
            }
        }

        g_pRender->initViewPort();

        WindowHint hint(0, 0, g_width, g_height);

        // draw Color
        g_pRender->draw((uint8_t*)s_colorImage, g_width * g_height * 3, hint);

        hint.x += g_width;
        hint.w = g_width;
        hint.h = g_height;

        // draw Depth
        if(g_bisPortraitDevice)
        {
            g_pRender->draw((uint8_t*)s_depthImage, g_width * 480 * 3, hint);
        }
        else
        {
            g_pRender->draw((uint8_t*)s_depthImage, g_width * g_height * 3, hint);
        }

        hint.x += g_width;
        hint.w = g_width;
        hint.h = g_height;

        // draw IR
        if(g_bisPortraitDevice)
        {
            g_pRender->draw((uint8_t*)s_irImage, g_width * 480 * 3, hint);
        }
        else
        {
            g_pRender->draw((uint8_t*)s_irImage, g_width * g_height * 3, hint);
        }

        std::string task_name = tasks[task_id];
        g_pRender->drawString("Task List: ", 1925, 40, 0.2,0.4,1);
        for (int x = 1940, y = 70, i = 0; i < tasks.size(); ++i, y+=30) {
            std::string text = "[" + std::to_string(i) + "] " + tasks[i];
            if (i == task_id)
                g_pRender->drawString(text.c_str(), x, y, 1,1.,0);
            else
                g_pRender->drawString(text.c_str(), x, y, 0.2,0.4,1);
        }
        g_pRender->drawCursorXYZValue(&imiFrame);
        g_pRender->update();

        s_bColorFrameOK = false;
        s_bDepth_IRFrameOK = false;

        if(g_bSave) {
            save((uint16_t *) pCamFrame->pData, s_colorImage, pCamFrame->size, pCamFrame->width, pCamFrame->height,
                 task_name + "_UVC_frame" + std::to_string(n_frames));
            save((uint16_t *) imiFrame->pData, s_depthImage, imiFrame->size, imiFrame->width, imiFrame->height,
                 task_name + "_Depth_frame" + std::to_string(n_frames));
            save((uint16_t *) imiFrame->pData + nFrameSize, s_irImage, imiFrame->size, imiFrame->width,
                 imiFrame->height, task_name + "_IR_frame" + std::to_string(n_frames));

            n_frames -= 1;
            if (n_frames == 0) {
                g_bSave = false;
                n_frames = n_frames_sampling;
            }
        }
    }

    // call this to free frame
    imiReleaseFrame(&imiFrame);
    // call this to free camera frame
    imiCamReleaseFrame(&pCamFrame);

    return true;
}

// keyboard event callback to save frames
template<typename T>
void keyboardFun(T key, int32_t x, int32_t y)
{
    tasks = load_tasks("./config.txt");
    int8_t tmp = task_id;

    switch (key)
    {
        case GLUT_KEY_UP:
            task_id++;
            break;
        case GLUT_KEY_DOWN:
            task_id--;
            break;
        case GLUT_KEY_LEFT:
            break;
        case GLUT_KEY_RIGHT:
            break;
        case 's':
            g_bSave = true;
            break;
        default:
            printf("\n");
            break;
    }

    if (key >= 48 && key <= 57) {
        task_id = key - '0';
        if (task_id < 0 || task_id >= tasks.size()) {
            printf("ERROR: Please choose the task ID from %d to %d \n", 0, tasks.size() - 1);
            task_id = tmp;
        }
        printf("Current task ID %d: %s \n", task_id, tasks[task_id].c_str());
    }

    if (task_id >= tasks.size()) {
        printf("ERROR: Please choose the task ID from %d to %d \n", 0, tasks.size() - 1);
        task_id = tmp;
    }
    printf("Current task ID %d: %s \n", task_id, tasks[task_id].c_str());
}

// keyboard event callback to save frames
//void keyboardFun_func(int key, int32_t x, int32_t y) {
//    tasks = load_tasks("./config.txt");
//    int8_t tmp = task_id;
//    switch (key)
//    {
//        case GLUT_KEY_UP:
//            task_id++;
//            break;
//        case GLUT_KEY_DOWN:
//            task_id--;
//            break;
//        case GLUT_KEY_LEFT:
//            break;
//        case GLUT_KEY_RIGHT:
//            break;
//    }
//
//    if (task_id >= tasks.size()) {
//        printf("ERROR: Please choose the task ID from %d to %d \n", 0, tasks.size() - 1);
//        task_id = tmp;
//    }
//    printf("Current task ID %d: %s \n", task_id, tasks[task_id].c_str());
//}

int Exit()
{
    stop();

    printf("------ exit ------\n");

    getchar();

    return 0;
}

int main(int argc, char** argv)
{
    camera_index = atoi(argv[1]);

    int ret = start();
    if(ret != 0)
    {
        printf("------ exit ------\n");

        getchar();
    }

    tasks = load_tasks("./config.txt");
    //6.create window and set read Stream frame data callback
    g_pRender = new SampleRender("UVCDepthIRViewer", g_width * 3 + 300, g_height); // window title & size
    g_pRender->init(argc, argv);
    g_pRender->setDataCallback(needImage, NULL);
//    g_pRender->setKeyCallback(keyboardFun_func);
    g_pRender->setKeyCallback(keyboardFun<int>);
    g_pRender->setKeyCallback(keyboardFun<unsigned char>);
    g_pRender->run();

    return Exit();
}
