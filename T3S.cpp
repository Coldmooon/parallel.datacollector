//
// Created by coldmoon on 2020/8/25.
//

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>


#include <stdbool.h>
#include <math.h>

//#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>

#include "cameras/T3S.infiray/include/thermometry.h"
#include "cameras/T3S.infiray/include/SimplePictureProcessing.h"

// Utils
#include "util/VCLab_utils.h"

// IO
#include "keyboard.hpp"
#include <inttypes.h>
#include <uiohook.h>
#include <wchar.h>
#include <thread>

using namespace xtherm;
unsigned char *buffer;                          //buffers 指针记录缓冲帧

#define IMAGEWIDTH 384
#define IMAGEHEIGHT 292

#define TRUE 1
#define FALSE 0

#define FILE_VIDEO1 "/dev/video0"

static int fd;                          //设备描述符
struct v4l2_streamparm setfps;          //结构体v4l2_streamparm来描述视频流的属性
struct v4l2_capability cap;             //取得设备的capability，看看设备具有什么功能，比如是否具有视频输入,或者音频输入输出等
struct v4l2_fmtdesc fmtdesc;            //枚举设备所支持的image format:  VIDIOC_ENUM_FMT
struct v4l2_format fmt, fmtack;          //子结构体struct v4l2_pix_format设置摄像头采集视频的宽高和类型：V4L2_PIX_FMT_YYUV V4L2_PIX_FMT_YUYV
struct v4l2_requestbuffers req;         //向驱动申请帧缓冲的请求，里面包含申请的个数
struct v4l2_buffer buf;                 //代表驱动中的一帧
enum v4l2_buf_type type;              //帧类型
struct v4l2_control ctrl;
float temperatureTable[16384];
float temperatureData[384 * 288];

int init_v4l2(void);                    //初始化
int v4l2_grab(void);                    //采集
int v4l2_control(int);                  //控制
bool needRefreshTable;
int delayy;

int8_t task_id = 0;
std::vector<std::string> tasks;
std::string person_name = "unknown";

bool b_gSaved = false;
int save_count = 0;
void save_t3s_frame() {
    b_gSaved = true;
};

int t3s_process() {
    SimplePictureProcessing *mSimplePictureProcessing = new SimplePictureProcessing(384, 288);
    mSimplePictureProcessing->SetParameter(100, 0.5f, 0.1f, 0.1f, 1.0f, 3.5f);
    printf("first~~\n");
    if (init_v4l2() == FALSE) {      //打开摄像头
        printf("Init fail~~\n");
        exit(EXIT_FAILURE);
    }
    printf("second~~\n");
    if (v4l2_grab() == FALSE) {
        printf("grab fail~~\n");
        exit(EXIT_FAILURE);
    }
    printf("fourth~~\n");
    if (v4l2_control(0x8004) == FALSE) {
        printf("control fail~~\n");
        exit(EXIT_FAILURE);
    }
    needRefreshTable = 1;
    delayy = 0;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;           //Stream 或者Buffer的类型。此处肯定为V4L2_BUF_TYPE_VIDEO_CAPTURE
    buf.memory = V4L2_MEMORY_MMAP;                    //既然是Memory Mapping模式，则此处设置为：V4L2_MEMORY_MMAP
    printf("third~~\n");
    int i = 100;
    double t;
    while (1) {
        t = (double) cvGetTickCount();                      //调用时钟测时间
        //ioctl(fd,VIDIOC_DQBUF,&buf);
//        printf("VIDIOC_DQBUF ready\n");
        if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) {
            printf("VIDIOC_DQBUF err\n");
            switch (errno) {
                case EAGAIN:
                    return 0;

                case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */

                default:
                    perror("VIDIOC_DQBUF");
                    exit(EXIT_FAILURE);
            }
        }
        buf.index = 0;


        int rangeMode = 120;
        float floatFpaTmp;
        float fix;
        float Refltmp;
        float Airtmp;
        float humi;
        float emiss;
        unsigned short distance;
        unsigned short *orgData = (unsigned short *) buffer;

/*        if(delayy<20){
        delayy++;
        printf("delayy:%d\n",delayy);
        }else{

             thermometryT(   IMAGEWIDTH,
                     IMAGEHEIGHT,
                    temperatureTable,
                    orgData,
                    temperatureData,
                     needRefreshTable,
                     rangeMode,
                    &floatFpaTmp,
                    & fix,
                    & Refltmp,
                    & Airtmp,
                    & humi,
                    & emiss,
                    & distance);
            //needRefreshTable=0;
                 }*/

//printf ("1:%f\n", temperatureTable[6000]);

//        printf("temperatureData[384*144+72]:%f,temperatureData[1]:%f", temperatureData[384 * 144 + 72],
//               temperatureData[1]);
//
//        printf("orgData0:%d ,orgData0:%d,orgData0:%d,orgData0:%d \n", orgData[384 * 288], orgData[384 * 288 + 1],
//               orgData[384 * 288 + 2], orgData[384 * 288 + 3]);
//        cv::Mat yuvImg;
//        yuvImg.create(IMAGEHEIGHT, IMAGEWIDTH, CV_8UC2);
//        memcpy(yuvImg.data, (unsigned char*)buffer, IMAGEHEIGHT*2* IMAGEWIDTH*sizeof(unsigned char));
        cv::Mat rgbImg(IMAGEHEIGHT, IMAGEWIDTH, CV_8UC4);
//        cv::cvtColor(yuvImg, rgbImg, CV_YUV2BGR_YUYV);
        mSimplePictureProcessing->Compute(orgData, rgbImg.data, 3);//0-5 to change platte

        cv::imshow("rgbImg", rgbImg);

//        printf("VIDIOC_DQBUF~~\n");
        ioctl(fd, VIDIOC_QBUF, &buf);                      //在 driver 内部管理着两个 buffer queues ，一个输入队列，一个输出队列。
        //对于 capture device 来说，当输入队列中的 buffer 被塞满数据以后会自动变为输出队列，
//        printf("VIDIOC_QBUF~~\n");                                 //等待调用 VIDIOC_DQBUF 将数据进行处理以后重新调用 VIDIOC_QBUF 将 buffer 重新放进输入队列.
//        if ((cvWaitKey(1) & 255) == 27) exit(0);
        char key = cvWaitKey(1);
        if( key == 27 )
            exit(0);
//        switch(key)
//        {
//            case 's':
//                std::cout << "save a frame." << std::endl;
//                cv::imwrite("test.jpg", rgbImg);
//                break;
//            default:
//                ;
//        }
        if (b_gSaved) {
            std::cout << "save a frame." << std::endl;
            std::string filename = tasks[task_id] + "_" + person_name + "_" + std::to_string(save_count) + "_t3s" + ".jpg";
            cv::imwrite(filename, rgbImg);
            save_count++;
            b_gSaved = false;
        }

        t = (double) cvGetTickCount() - t;
        printf("used time is %gms\n", (t / (cvGetTickFrequency() * 1000)));
    }

    ioctl(fd, VIDIOC_STREAMOFF, &type);         // 停止视频采集命令，应用程序调用VIDIOC_ STREAMOFF停止视频采集命令后，视频设备驱动程序不在采集视频数据。
    return 0;
}

int init_v4l2(void) {
    if ((fd = open(FILE_VIDEO1, O_RDWR)) == -1) {               //打开video1
        printf("Opening video device error\n");
        return FALSE;
    }
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {               // 查询视频设备的功能
        printf("unable Querying Capabilities\n");
        return FALSE;
    } else
/*
        {
        printf( "Driver Caps:\n"
                "  Driver: \"%s\"\n"
                "  Card: \"%s\"\n"
                "  Bus: \"%s\"\n"
                "  Version: %d\n"
                "  Capabilities: %x\n",
                cap.driver,
                cap.card,
                cap.bus_info,
                cap.version,
                cap.capabilities);
        }
        if((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE){//是否支持V4L2_CAP_VIDEO_CAPTURE
            printf("Camera device %s: support capture\n",FILE_VIDEO1);
        }
        if((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING){//是否支持V4L2_CAP_STREAMING
            printf("Camera device %s: support streaming.\n",FILE_VIDEO1);
        }
*/
        fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format: \n");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {        // 获取当前视频设备支持的视频格式
        printf("\t%d. %s\n", fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }
    //set fmt
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = IMAGEWIDTH;
    fmt.fmt.pix.height = IMAGEHEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//使用V4L2_PIX_FMT_YUYV
    //fmt.fmt.pix.field = V4L2_FIELD_NONE;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {    // 设置视频设备的视频数据格式，例如设置视频图像数据的长、宽，图像格式（JPEG、YUYV格式）
        printf("Setting Pixel Format error\n");
        return FALSE;
    }
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {   //获取图像格式
        printf("Unable to get format\n");
        return FALSE;
    }
//        else

/*        {
            printf("fmt.type:\t%d\n",fmt.type);         //可以输出图像的格式
            printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF,(fmt.fmt.pix.pixelformat >> 8) & 0xFF,\
                   (fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
            printf("pix.height:\t%d\n",fmt.fmt.pix.height);
            printf("pix.field:\t%d\n",fmt.fmt.pix.field);
        }
*/
/*
        setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        setfps.parm.capture.timeperframe.numerator = 100;
        setfps.parm.capture.timeperframe.denominator = 100;
        printf("init %s is OK\n",FILE_VIDEO1);
*/
    return TRUE;
}

int v4l2_grab(void) {
    //struct v4l2_requestbuffers req = {0};
    //4  request for 1buffers
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)        //开启内存映射或用户指针I/O
    {
        printf("Requesting Buffer error\n");
        return FALSE;
    }
    //5 mmap for buffers
    buffer = (unsigned char *) malloc(req.count * sizeof(*buffer));
    if (!buffer) {
        printf("Out of memory\n");
        return FALSE;
    }
    unsigned int n_buffers;
    for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
        //struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) { // 查询已经分配的V4L2的视频缓冲区的相关信息，包括视频缓冲区的使用状态、在内核空间的偏移地址、缓冲区长度等。
            printf("Querying Buffer error\n");
            return FALSE;
        }

        buffer = (unsigned char *) mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (buffer == MAP_FAILED) {
            printf("buffer map error\n");
            return FALSE;
        }
        printf("Length: %d\nAddress: %p\n", buf.length, buffer);
        // printf("Image Length: %d\n", buf.bytesused);
    }
    //6 queue
    for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
        buf.index = n_buffers;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_QBUF, &buf)) {    // 投放一个空的视频缓冲区到视频缓冲区输入队列中
            printf("query buffer error\n");
            return FALSE;
        }
    }
    //7 starting
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) { //
        printf("stream on error\n");
        return FALSE;
    }
    return TRUE;
}

int v4l2_control(int value) {
    ctrl.id = V4L2_CID_ZOOM_ABSOLUTE;
    ctrl.value = value;//change output mode 0x8004
    //shutter 0x8001
    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == -1) {
        printf("v4l2_control error\n");
        return FALSE;
    }
    return TRUE;
}

// keyboard function for UIOHOOK library
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
                        exit(0);
                }
            }
            else if (event->data.keyboard.keycode == VC_S)
                save_t3s_frame();
            else if (event->data.keyboard.keycode == VC_P) {
                // not thread safe because keyboardIO thread is modifying the person_name while the cameraID is reading the person_name.
//                std::cout << "Please input the person name: \n" << std::endl;
//                std::string newPersong;
//                getline(std::cin, newPersong);
//                person_name = newPersong;
            }
            else if (event->data.keyboard.keycode == VC_UP) {
                task_id--;
                if (task_id < 0) task_id = tasks.size() - 1;
            }
            else if (event->data.keyboard.keycode == VC_DOWN) {
                task_id++;
                if (task_id >= tasks.size()) task_id = 0;
            }
            else if (event->data.keyboard.keycode >= VC_1 && event->data.keyboard.keycode <= VC_9) {
                int tmp = task_id;
                task_id = event->data.keyboard.keycode - VC_1;
                if (task_id < 0 || task_id >= tasks.size()) {
                    printf("ERROR: Please choose the task ID from %d to %d \n", 1, tasks.size());
                    task_id = tmp;
                }
            }

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

//    fprintf(stdout, "%s\n",     buffer);　// keycode debug
}

void keyboardIO_thread() {

    keyboard_monitor(hotkeys);
}

void cameraIO_thread(int argc, char** argv) {
    t3s_process();
}

int main (int argc, char** argv) {
    tasks = load_tasks("./config.txt");

    std::vector<std::thread> workers;
    workers.push_back(std::thread(cameraIO_thread, argc, argv));
    workers.push_back(std::thread(keyboardIO_thread));
    for (auto &worker: workers) {
        worker.join();
    }

    return 0;
}
