/********************************************************************************
Copyright (c) 2015 Huajie IMI Technology Co., Ltd.
All rights reserved.

@File Name		: main.cpp
@Author			: Wendell
@Date			: 2018-06-18
@Description	: read IR frame and view
@Version		: 1.0.0
@History		:
1.2018-06-18 Wendell Created file
********************************************************************************/
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

// Imi Head File
#include "ImiNect.h"

// UI
#include "Render.h"

#define MAX_DEPTH 10000

// window handle
SampleRender* g_pRender = NULL;

ImiDeviceAttribute* g_DeviceAttr = NULL;

// device handle
ImiDeviceHandle g_ImiDevice = NULL;

// stream handles
ImiStreamHandle g_streams[10];
uint32_t g_streamNum = 0;

// Histogram view mode
void calculateHistogram(float* pHistogram, int histogramSize, const ImiImageFrame* frame)
{
    const uint16_t* pDepth = (const uint16_t*)frame->pData;

    memset(pHistogram, 0, histogramSize*sizeof(float));

    int height = frame->height;
    int width = frame->width;

    unsigned int nNumberOfPoints = 0;
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x, ++pDepth)
        {
            if(*pDepth != 0)
            {
                pHistogram[*pDepth]++;
                nNumberOfPoints++;
            }
        }
    }

    for(int nIndex = 1; nIndex < histogramSize; ++nIndex)
    {
        pHistogram[nIndex] += pHistogram[nIndex-1];
    }

    if(nNumberOfPoints)
    {
        for(int nIndex = 1; nIndex < histogramSize; ++nIndex)
        {
            pHistogram[nIndex] = (256 * (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
        }
    }
}

// window callback, called by SampleRender::display()
static bool needImage(void* pData)
{
    static float       s_depthHist[MAX_DEPTH];
    static RGB888Pixel s_rgbImage[1280 * 1024];

    int32_t        avStreamIndex = 0;
    ImiImageFrame* pFrame = NULL;

    // frame read.
    if(0 != imiReadNextFrame(g_streams[avStreamIndex], &pFrame, 100))
    {
        return false;
    }

    if(NULL == pFrame)
    {
        return false;
    }

    // Calculate histogram
    calculateHistogram(s_depthHist, MAX_DEPTH, pFrame);

    uint32_t rgbSize;
    uint16_t * pde = (uint16_t*)pFrame->pData;
    for(rgbSize = 0; rgbSize < pFrame->size/2; ++rgbSize)
    {
        s_rgbImage[rgbSize].r = s_depthHist[pde[rgbSize]];
        s_rgbImage[rgbSize].g = s_rgbImage[rgbSize].r;
        s_rgbImage[rgbSize].b = 0;
    }

    // Draw
    g_pRender->draw(-1, (uint8_t*)s_rgbImage, rgbSize, pFrame->width, pFrame->height, &pFrame);

    // call this to free frame
    imiReleaseFrame(&pFrame);

    return true;
}

int Exit()
{
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

    if(NULL != g_pRender)
    {
        delete g_pRender;
        g_pRender = NULL;
    }

    printf("------ exit ------\n");

    getchar();

    return 0;
}

int main(int argc, char** argv)
{
    //1.imiInitialize()
    int ret = imiInitialize();
    if(0 != ret)
    {
        printf("ImiNect Init Failed! ret = %d\n", ret);
        return Exit();
    }
    printf("ImiNect Init Success.\n");

    //2.imiGetDeviceList()
    uint32_t deviceCount = 0;
    imiGetDeviceList(&g_DeviceAttr, &deviceCount);
    if((deviceCount <= 0) || (NULL == g_DeviceAttr))
    {
        printf("Get No Connected ImiDevice!\n");
        return Exit();
    }
    printf("Get %d Connected ImiDevice.\n", deviceCount);

    //3.imiOpenDevice()
    ret = imiOpenDevice(g_DeviceAttr[0].uri, &g_ImiDevice, 0);
    if(0 != ret)
    {
        printf("Open ImiDevice Failed! ret = %d\n", ret);
        return Exit();
    }
    printf("ImiDevice Opened.\n");

    //4.imiGetCurrentFrameMode
    const ImiFrameMode* pMode = imiGetCurrentFrameMode(g_ImiDevice, IMI_DEPTH_FRAME);
    if (NULL == pMode)
    {
        printf("Get current frame mode failed!\n");
        return Exit();
    }
    printf("Get current frame mode:%d-%d-%d-%d-%d\n", pMode->bitsPerPixel,
           pMode->framerate, pMode->pixelFormat, pMode->resolutionX, pMode->resolutionY);
    imiSetFrameMode(g_ImiDevice, IMI_DEPTH_FRAME, (ImiFrameMode*)pMode);

    //5.imiOpenStream()
    ret = imiOpenStream(g_ImiDevice, IMI_DEPTH_FRAME, NULL, NULL, &g_streams[g_streamNum++]);
    if(0 != ret)
    {
        printf("Open Depth Stream Failed! ret = %d\n", ret);
        return Exit();
    }
    printf("Open Depth Stream Success.\n");

    //6.create window and set read Stream frame data callback
    g_pRender = new SampleRender("Depth View", pMode->resolutionX, pMode->resolutionY);  // window title & size
    g_pRender->init(argc, argv);
    g_pRender->setDataCallback(needImage, NULL);
    g_pRender->run();

    return Exit();
}
