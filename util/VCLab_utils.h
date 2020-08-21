//
// Created by coldmoon on 2020/6/26.
//
#ifndef UNTITLED_VCLAB_UTILS_H
#define UNTITLED_VCLAB_UTILS_H

#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <fstream>

// Imi Head File
#include "a200.imi/include/ImiNect.h"
#include "a200.imi/include/ImiCamera.h"
#include "UI/opengl/Render.h"

// system
void sleepMs (int32_t msecs);

// ------------------- file processing ---------------------
template <class Container>
void split_string(const std::string& str, Container& cont);
std::string load_file(std::string path);

// --------------- export data -------------------

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

int32_t save_to_BMP(const char* bmpImagePath, const uint8_t* pframe, int width, int height);
void save_frame(const uint16_t * pdata, const RGB888Pixel * s_xxxImage, uint32_t size, int32_t width, int32_t height, std::string name_prefix, bool saveRaw);
// --------------- task assign -------------------
std::vector<std::string> load_tasks(std::string path);
bool parse_cameras(const std::string & args, std::vector<int8_t> & cameras);
bool drawtexts(SampleRender* g_pRender, std::vector<std::string> tasks, int8_t task_id);

#endif //UNTITLED_VCLAB_UTILS_H