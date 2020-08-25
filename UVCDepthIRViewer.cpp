// Created by coldmooon

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/time.h>
#endif

#include <string>
#include <vector>
#include <iostream>
#include <GL/freeglut.h>
#include <memory>
#include <thread>

// Camera modules
#include "modules/a200.hpp"

// UI
#include "UI/opengl/Render.h"
#include <X11/Xlib.h>

// Utils
#include "util/VCLab_utils.h"

// IO
#include "keyboard.hpp"
#include <inttypes.h>
#include <uiohook.h>
#include <wchar.h>

// -----------------------------------------------------------------------------
// window handle
SampleRender* g_pRender = nullptr;
std::vector<SampleRender *> g_pRenders;

// parse camera specified
uint32_t deviceCount = 0;
std::vector<int8_t> cameras;

// switch of frame sync
bool g_bNeedFrameSync = false;

// tasks
int8_t task_id = 0;
std::vector<std::string> tasks;
std::string person_name = "unknown";

// -----------------------------------------------------------------------------
static bool receiving_rendering_opengl(int winID) {

    const int8_t factor = 3; // can avoid signal aliasing ???? I guess.
    const int resolution = g_width * g_height;
    const int frame_length = resolution * factor;
    const int deviceCount = 1; // shadow the global variable for test

    // method 1： define a 2-D array at runtime by 'new' method. Note: do not delete them in this function if declare them static varables.
    static RGB888Pixel (*s_depthImages)[frame_length] = new RGB888Pixel[deviceCount][frame_length];
    static RGB888Pixel (*s_colorImages)[frame_length] = new RGB888Pixel[deviceCount][frame_length];
    static RGB888Pixel (*s_irImages)[frame_length] = new RGB888Pixel[deviceCount][frame_length];

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

    // don't declare it a static variable, or the whole streams will be dropped once some frame is lost.
    // camera_idx is actually the windowID.
    SampleRender * g_pRender = g_pRenders[winID];
    bool s_FrameOK = get_a200_frame(winID, g_pRender->m_camera_id, tasks[task_id], person_name, s_colorImages[0], s_depthImages[0], s_irImages[0]);

    if (s_FrameOK) {
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

        drawtexts(g_pRender, tasks, task_id);
//        g_pRender->drawCursorXYZValue(&imiFrame[0]);
    }
    else
        std::cerr << "\nLost a frame !" << std::endl;
        // Note: Do not return false here, or the program will stop running.

    return true;
}

// keyboard function for freeglut callback.
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
            Exit(deviceCount);
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

static bool assign_tasks(SampleRender* g_pRender) {
    // single thread
    int win = glutGetWindow();
    if (win - 1 < 0) {std::cerr << "Got negative window ID." << std::endl; Exit(deviceCount); return false;}

    bool isdone = receiving_rendering_opengl(win - 1);

    if (!isdone) {
        std::cerr << "\nRendering wrong. Please check receiving_rendering_single_a200()." << std::endl;
        Exit(deviceCount);
        return false;
    }
    glutSwapBuffers();
    glutPostRedisplay();
    g_pRender->initViewPort();

    return true;
}

void register_tasks(int argc, char** argv) {
    for (int i = 0; i < deviceCount; ++i){
        std::string window_name(argv[i + 1]);
//        window_name.push_back(argv[i + 1]);
        SampleRender * pRender = new SampleRender(("Camera " + window_name).c_str(), g_width * 3 + 300, g_height + 10);
        pRender->m_camera_id = cameras[i];

        pRender->setDataCallback_multithread(assign_tasks); // set display callback function
//        pRender->setKeyCallback(keyboardFun<unsigned char>); // set normal and functional keys callback
        pRender->init(argc, argv); // initialize OpenGL and create a window

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
                        Exit(deviceCount);
                        exit(0);
                }
            }
            else if (event->data.keyboard.keycode == VC_S)
                save_a200_frame();
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
    XInitThreads();
    glutInit(&argc, argv);

    register_tasks(argc, argv);

//    glutIdleFunc(idle);
    glutMainLoop();
    std::cout << "Exited glutMainLoop." << std::endl;
    Exit(deviceCount);
}

int main(int argc, char** argv)
{
    if (nullptr != argv[1])
        parse_cameras(argv[1], cameras);

    deviceCount += a200init(cameras);
    if(deviceCount == 0) {
        std::cerr << "Error Counting devices." << std::endl; exit(-1);
        getchar();
    }

    tasks = load_tasks("./config.txt");
    // Glut pipline:
    // 1. initialize the window properties --> 2. create the window --> 3. register the callback function when an event, e.g., a mouse move, occurs.
    // That is to tell GLUT which function to call back --> 4. enter the event processing loop
    // For each event type, GLUT provides a specific function to register the call back function.
    // Registering a callback function means to tell GLUT that it should use the function, e.g., needImage(), we just wrote/created for the rendering.
    // GLUT will call the function you choose whenever rendering is required.
    // Note: GLUT functions, as C functions, can not invoke a callback with instance specific information. So avoid registering memeber function as callback.
    std::vector<std::thread> workers;
    workers.push_back(std::thread(cameraIO_thread, argc, argv));
    workers.push_back(std::thread(keyboardIO_thread));
    for (auto &worker: workers) {
        worker.join();
    }

    return 0;
}
