//
// Created by coldmoon on 2020/6/26.
//

#include <regex>
#include "VCLab_utils.h"

//bmp buffer
uint8_t g_bmpColor[1920*1080*3] = {0};

template <class Container>
void split_string(const std::string& str, Container& cont) {
    std::istringstream iss(str);
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter(cont));
};

std::string load_file(std::string path) {
    std::ifstream t(path);
    if (!t) std::cerr << "Error opening file." << std::endl;
    std::stringstream buffer;
    buffer << t.rdbuf();

    return buffer.str();
}


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
    bmfh.bfOffBits   = OffBits; // 头部信息54字节
    bmfh.bfSize      = imagePixSize * 3 + OffBits;

    memset(&bmih, 0, sizeof(BMPINFOHEADER_T));
    bmih.biSize      = 40; // 结构体大小为40
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

    fwrite(&bmfh, 8, 1, pSaveBmp);
    fwrite(&bmfh.bfReserved2, sizeof(bmfh.bfReserved2), 1, pSaveBmp);
    fwrite(&bmfh.bfOffBits, sizeof(bmfh.bfOffBits), 1, pSaveBmp);
    fwrite(&bmih, sizeof(BMPINFOHEADER_T), 1, pSaveBmp );
    fwrite(g_bmpColor, imagePixSize*3, 1, pSaveBmp);

    fclose(pSaveBmp);

    return 0;
}

void save(uint16_t* pdata, RGB888Pixel * s_xxxImage, uint32_t size, int32_t width, int32_t height, std::string name_prefix) {
    std::string raw_name = name_prefix + ".raw";
    std::string visual_name = name_prefix + ".bmp";
    bool isUVC = std::regex_match(name_prefix, std::regex("(.*)(UVC)(.*)"));

    if (!isUVC) {
        FILE *pFile = fopen(raw_name.c_str(), "wb");
        if (pFile) {
            fwrite(pdata, size, 1, pFile);
            fclose(pFile);
            printf("save one %s frame raw data to %s_reg.raw Success !\n", name_prefix.c_str(),
                   name_prefix.c_str());
        } else
            printf("save one %s frame raw data to %s_reg.raw Failed !\n", name_prefix.c_str(), name_prefix.c_str());
    }

    if (0 == saveToBMP(visual_name.c_str(), (const uint8_t *) s_xxxImage, width, height)) {
        printf("save one Depth frame to Depth_reg.bmp Success !\n");
    } else
        printf("save one Depth frame to Depth_reg.bmp Failed !\n");
}

std::vector<std::string> load_tasks(std::string path) {
    std::string task_raw = load_file(path);

    std::vector<std::string> task_list;
    split_string(task_raw, task_list);
//    std::copy(task_list.begin(), task_list.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

    return task_list;
}
