//
// Created by coldmoon on 2020/7/27.
//

// Camera module head files
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "util/util.h"
#include "util/VCLab_utils.h"
#include "cameras/a200.imi/include/ImiNect.h"
#include "cameras/a200.imi/include/ImiCamera.h"
#include "cameras/a200.imi/include/ImiCameraPrivate.h"

// device attributes
ImiDeviceAttribute* g_DeviceAttr = nullptr; // Depth + IR
ImiCamAttribute*    g_pCameraAttr = nullptr; // UVC

// device handle
ImiDeviceHandle * g_ImiDevice    = nullptr;
ImiCameraHandle * g_cameraDevice = nullptr;

// stream handles
ImiStreamHandle * g_streams = nullptr;

// Frame data address
ImiImageFrame * imiFrame = nullptr;
ImiCameraFrame * pCamFrame = nullptr;

bool g_bisPortraitDevice = false;

const int32_t g_width  = 640;
const int32_t g_height = 480;

// frame saving count.
int color_count = 1;
int depth_count = 1;
int ir_count = 1;

// frame saving control
bool g_bSave = false;
int8_t n_frames_init = 30;
int8_t n_frames = n_frames_init;

int stop(int deviceCount)
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

int Exit(int deviceCount)
{
    stop(deviceCount);
    printf("------ exit ------\n");
    getchar();

    return 0;
}

void save_frames(const uint16_t * pData, const RGB888Pixel * s_colorImage, const RGB888Pixel * s_depthImage, const RGB888Pixel * s_IRImage, const std::string filename, const std::string postfix, int8_t k, bool saveRaw) {
    // pCamFrame size: 640*480*3
    // imiFrame size: 640*480*4
    // framesize: g_width * g_height
    if (s_colorImage != nullptr) {
        save_frame(pData, s_colorImage, pCamFrame->size, pCamFrame->width, pCamFrame->height,
             filename + postfix + "_camera[" + std::to_string(k) + "]_" + std::to_string(color_count), false);
        color_count++;
    }
    if (s_depthImage != nullptr) {
        save_frame(pData, s_depthImage, imiFrame->size, imiFrame->width, imiFrame->height,
             filename + postfix + "_camera[" + std::to_string(k) + "]_" + std::to_string(depth_count), saveRaw);
        depth_count++;
    }
    if (s_IRImage != nullptr) {
        save_frame(pData, s_IRImage, imiFrame->size, imiFrame->width, imiFrame->height,
             filename + postfix + "_camera[" + std::to_string(k) + "]_" + std::to_string(ir_count), saveRaw);
        ir_count++;
    }
}

void save(const uint16_t * pData, const RGB888Pixel * s_colorImage, const RGB888Pixel * s_depthImage, const RGB888Pixel * s_IRImage,
          int8_t camera_idx, const std::string filename, const std::string modality) {
    if(g_bSave) {
        save_frames(pData, s_colorImage, s_depthImage, s_IRImage, filename, modality, camera_idx, true);
    }
}

void save_a200_frame() {
    g_bSave = true;
}

uint32_t a200init(std::vector<int8_t> & cameras) {
    int ret = imiInitialize();
    if(0 != ret) {
        printf("ImiNect Init Failed! ret = %d\n", ret);
        stop(0);
        exit(-1);
    }
    printf("ImiNect Init Success.\n");

    //2.imiGetDeviceList()
    uint32_t deviceCount = 0;
    imiGetDeviceList(&g_DeviceAttr, &deviceCount);
    if((deviceCount <= 0) || (nullptr == g_DeviceAttr)) {
        printf("Get No Connected ImiDevice!\n");
        stop(deviceCount);
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
            stop(deviceCount);
            exit(-1);
        }
        printf("ImiDevice %d Opened.\n", k);
    }

    int deviceCameraCount = 0;
    ret = getCamAttrList(&g_pCameraAttr, &deviceCameraCount);
    deviceCameraCount = deviceCameraCount > cameras.size()? cameras.size(): deviceCameraCount;
    if (ret != 0 || nullptr == g_pCameraAttr || deviceCount != deviceCameraCount) {
        printf("getCamAttrList Failed! ret = %d\n", ret);
        stop(deviceCount);
        exit(-1);
    }

    // open UVC camera
    g_cameraDevice = new ImiCameraHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        int8_t k = cameras[i];
        ret = imiCamOpenURI(g_pCameraAttr[k].uri, &g_cameraDevice[i]);
        if(0 != ret) {
            printf("Open UVC Camera %d Failed! ret = %d\n", k, ret);
            stop(deviceCount);
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
    g_streams = new ImiStreamHandle[deviceCount];
    for (int i = 0; i < deviceCount; ++i) {
        ret = imiCamStartStream(g_cameraDevice[i], &pMode);
        if(0 != ret) {
            printf("Start Camera %d stream Failed! ret = %d\n", i, ret);
            stop(deviceCount);
            exit(-1);
        }
        printf("Start Camera %d stream Success\n", i);

        //4.imiSetImageRegistration
        ret = imiSetImageRegistration(g_ImiDevice[i], IMI_TRUE);
        printf("imiSetImageRegistration ret = %d\n", ret);

        //5.imiOpenStream()
        ret = imiOpenStream(g_ImiDevice[i], IMI_DEPTH_IR_FRAME, nullptr, nullptr, &g_streams[i]);
        std::cout << imiGetLastError() << std::endl;
        if(0 != ret) {
            printf("Open Depth %d Stream Failed! ret = %d\n", i, ret);
            stop(deviceCount);
            exit(-1);
        }
        printf("Open Depth %d Stream Success.\n", i);

        // imiCamSetFramesSync to set Color Depth_IR sync
        if(!g_bisPortraitDevice) {
            ret = imiCamSetFramesSync(g_cameraDevice[i], true);
            printf("Camera [%d] imiCamSetFramesSync = %d\n", i, ret);
        }
    }

    return deviceCount;
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

bool get_colorDepthIR(ImiStreamHandle g_DepthIR_streams, ImiCameraHandle g_camera_Device, RGB888Pixel* s_colorImage, RGB888Pixel* s_depthImage, RGB888Pixel* s_IRImage,
                      int8_t camera_idx, const std::string frame_description) {

    bool s_bColorFrameOK = false;
    bool s_bDepth_IRFrameOK = false;
    if ( nullptr != g_DepthIR_streams) {
        int framesize = g_width * g_height;
        if(nullptr == s_depthImage && nullptr == s_IRImage) {
            std::cerr << "ERROR: no Depth / IR pixel container provided..";
            return false;
        }

        if (0 != imiReadNextFrame(g_DepthIR_streams, &imiFrame, 100)) {
            std::cerr << "imiReadNextFrame() function reported an error." << std::endl;
            return false;
        }

        if (nullptr != imiFrame) {
            if (nullptr != s_depthImage) {
                uint16_t  * pde = (uint16_t *) imiFrame->pData;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_depthImage[i].r = pde[i] >> 3;
                    s_depthImage[i].g = s_depthImage[i].r;
                    s_depthImage[i].b = s_depthImage[i].r;
                }
                save(pde, nullptr, s_depthImage, nullptr, camera_idx, frame_description, "_depth_frame" + std::to_string(n_frames));
            }
            if (nullptr != s_IRImage) {
                uint16_t * pIR = (uint16_t *) imiFrame->pData + framesize;
                for (uint32_t i = 0; i < framesize; ++i) {
                    s_IRImage[i].r = pIR[i] >> 2;
                    s_IRImage[i].g = s_IRImage[i].r;
                    s_IRImage[i].b = s_IRImage[i].r;
                }
                save(pIR, nullptr, nullptr, s_IRImage, camera_idx, frame_description, "_IR_frame" + std::to_string(n_frames));
            }
        }
        else {
            std::cerr << "Failed to receive the next frame in imiReadNextFrame() function." << std::endl;
            return false;
        }

        s_bDepth_IRFrameOK = true;
        imiReleaseFrame(&imiFrame);
    }

    if (nullptr != g_camera_Device) {
        if ( nullptr == s_colorImage) {
            std::cerr << "ERROR: no UVC pixel container provided..";
            return false;
        }

        if (0 != imiCamReadNextFrame(g_camera_Device, &pCamFrame, 100)) {
            std::cerr << "imiCamReadNextFrame() function reported an error." << std::endl;
            return false;
        }

        if (nullptr != pCamFrame) {
            memcpy((void *) s_colorImage, (const void *) pCamFrame->pData, pCamFrame->size);
            save(nullptr, s_colorImage, nullptr, nullptr, camera_idx, frame_description, "_rgb_frame" + std::to_string(n_frames));
        }
        else {
            std::cerr << "Failed to receive the next frame in imiCamReadNextFrame() function." << std::endl;
            return false;
        }

        s_bColorFrameOK = true;
        imiCamReleaseFrame(&pCamFrame);
    }

    if (g_bSave) {
        n_frames--;
        if (n_frames == 0) {
            g_bSave = false;
            n_frames = n_frames_init;
        }
    }
    return s_bColorFrameOK & s_bDepth_IRFrameOK;
}

bool get_a200_frame(int8_t winId, int camera_idx, RGB888Pixel * s_colorImage, RGB888Pixel * s_depthImage, RGB888Pixel * s_IRImage, const std::string frame_description) {

    // To check: if the following codes use two steps to capture color and Depth/IR frames individually, "xxxReadNextFrame" will be called
    // twice. In this case, the color frame and the Depth/IR frame may be captured at different time stamp, leading to async problem.
    if (s_depthImage != nullptr && s_IRImage != nullptr && s_colorImage != nullptr) {
        return get_colorDepthIR(g_streams[winId], g_cameraDevice[winId], s_colorImage, s_depthImage, s_IRImage, camera_idx, frame_description);
    }
    else
        return false;
}

