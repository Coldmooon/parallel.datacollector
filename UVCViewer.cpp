/********************************************************************************
Copyright (c) 2015 Huajie IMI Technology Co., Ltd.
All rights reserved.

@File Name		: main.cpp
@Author			: Wendell
@Date			: 2018-06-18
@Description	: read UVC Color frame and view
@Version		: 1.0.0
@History		:
1.2018-06-18 Wendell Created file
********************************************************************************/
#include <stdio.h>
#include <stdlib.h>

// Imi Head File
#include "ImiCamera.h"

// UI
#include "Render.h"

#pragma pack(push,1)

typedef struct
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BMPFILEHEADER_T;

typedef struct
{
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPINFOHEADER_T;

typedef struct tagRGBPixel
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RGBPixel;

#pragma pack()

//switch of save one frame
bool g_bSave = false;

//bmp buffer
uint8_t g_bmpColor[1920*1080*3] = {0};

// window handle
SampleRender* g_pRender = NULL;

// device handle
ImiCameraHandle g_cameraDevice = NULL;

int32_t g_width  = 640;
int32_t g_height = 480;

int32_t saveToBMP(const char* bmpImagePath, const uint8_t* pframe, int width, int height)
{
    BMPFILEHEADER_T bmfh; // bitmap file header
    BMPINFOHEADER_T bmih; // bitmap info header (windows)

    const int OffBits = 54;

    int32_t imagePixSize = width * height;

    memset(&bmfh, 0, sizeof(BMPFILEHEADER_T));
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfType      = 0x4d42;
    bmfh.bfOffBits   = OffBits; // ͷ����Ϣ54�ֽ�
    bmfh.bfSize      = imagePixSize * 3 + OffBits;

    memset(&bmih, 0, sizeof(BMPINFOHEADER_T));
    bmih.biSize      = 40; // �ṹ���СΪ40
    bmih.biPlanes    = 1;
    bmih.biSizeImage = imagePixSize * 3;

    bmih.biBitCount    = 24;
    bmih.biCompression = 0;
    bmih.biWidth       = width;
    bmih.biHeight      = height;

    // rgb -> bgr
    RGBPixel* pRgb = (RGBPixel*)g_bmpColor;
    RGBPixel* pSrc = (RGBPixel*)pframe;
    int tmpindex1(0), tmpindex2(0);

    for(int i = 0; i < height; ++i)
    {
        tmpindex1 = i * width;
        tmpindex2 = (height - i - 1) * width;
        for(int j = 0; j < width; ++j)
        {
            pRgb[tmpindex1 + j].r = pSrc[tmpindex2 + j].b;
            pRgb[tmpindex1 + j].g = pSrc[tmpindex2 + j].g;
            pRgb[tmpindex1 + j].b = pSrc[tmpindex2 + j].r;
        }
    }

    char buf[128]= {0};
    std::string fullPath = bmpImagePath;

    FILE* pSaveBmp = fopen(fullPath.c_str(), "wb");
    if(NULL == pSaveBmp)
    {
        return -1;
    }

    fwrite(&bmfh, 8, 1, pSaveBmp );
    fwrite(&bmfh.bfReserved2, sizeof(bmfh.bfReserved2), 1, pSaveBmp);
    fwrite(&bmfh.bfOffBits, sizeof(bmfh.bfOffBits), 1, pSaveBmp);
    fwrite(&bmih, sizeof(BMPINFOHEADER_T), 1, pSaveBmp );
    fwrite(g_bmpColor, imagePixSize*3, 1, pSaveBmp);

    fclose(pSaveBmp);

    return 0;
}

// window callback, called by SampleRender::display()
static bool needImage(void* pData)
{
    ImiCameraFrame* pCamFrame = NULL;
    if(0 != imiCamReadNextFrame(g_cameraDevice, &pCamFrame, 100))
    {
        return false;
    }

    if(NULL == pCamFrame)
    {
        return false;
    }

    uint8_t* pRGB = (uint8_t*)(pCamFrame->pData);

    if(g_bSave)
    {
        FILE* pFile = fopen("Color_reg.raw", "wb");
        if(pFile)
        {
            fwrite(pRGB, pCamFrame->size, 1, pFile);
            fclose(pFile);
            printf("save one Color frame raw data to Color_reg.raw Success !\n");
        }
        else
        {
            printf("save one Color frame raw data to Color_reg.raw Failed !\n");
        }

        if(0 == saveToBMP("Color_reg.bmp", pRGB, pCamFrame->width, pCamFrame->height))
        {
            printf("save one Color frame to Color_reg.bmp Success !\n");
        }
        else
        {
            printf("save one Color frame to Color_reg.bmp Failed !\n");
        }

        g_bSave = false;
    }

    WindowHint hint(0, 0, g_width, g_height);

    g_pRender->initViewPort();

    g_pRender->draw(pRGB, pCamFrame->size, hint);

    g_pRender->update();

    // free frame
    imiCamReleaseFrame(&pCamFrame);

    return true;
}

// keyboard event callback to save one frame
void keyboardFun(unsigned char key, int32_t x, int32_t y)
{
    switch (key)
    {
        case 'S':
        case 's':
            g_bSave = true;
            break;
        default:
            printf("Please input 's' or 'S'  to save image: \n");
            break;
    }
}

int Exit()
{
    // close Camera
    if(NULL != g_cameraDevice)
    {
        imiCamStopStream(g_cameraDevice);
        imiCamClose(g_cameraDevice);
        g_cameraDevice = NULL;
    }

    printf("------ exit ------\n");

    getchar();

    return 0;
}

int main(int argc, char** argv)
{
    int ret = 0;

    // open UVC camera
    ret = imiCamOpen(&g_cameraDevice);
    if(0 != ret)
    {
        printf("Open UVC Camera Failed! ret = %d\n", ret);
        return Exit();
    }
    printf("Open UVC Camera Success\n");

    const ImiCameraFrameMode* pMode = imiCamGetCurrentFrameMode(g_cameraDevice);
    if (NULL == pMode)
    {
        printf("Get current frame mode failed!\n");
        return Exit();
    }

    g_width  = pMode->resolutionX;
    g_height = pMode->resolutionY;

    // open camera stream
    ret = imiCamStartStream(g_cameraDevice, pMode);
    if(0 != ret)
    {
        printf("Start Camera stream Failed! ret = %d\n", ret);
        return Exit();
    }
    printf("Start Camera stream Success\n");

    //ret = imiCamGetProperty(g_cameraDevice, IMI_CAM_PROPERTY_COLOR_BACKLIGHT_COMPENSATION, &iValue, &vLen);
    g_pRender = new SampleRender("UVC Color View", g_width, g_height);  // window title & size
    g_pRender->init(argc, argv);
    g_pRender->setKeyCallback(keyboardFun);
    g_pRender->setDataCallback(needImage, NULL);
    g_pRender->run();

    return Exit();
}