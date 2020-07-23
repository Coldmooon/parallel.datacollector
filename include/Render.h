// Improved by Coldmooon

#ifndef _IMI_SAMPLE_RENDER_H_
#define _IMI_SAMPLE_RENDER_H_

#include <string>
#include <stdint.h>
#include <string.h>

//Imi Head File
#include "ImiNect.h"
#include "ImiDefines.h"
#include "ImiSkeleton.h"

#include "Utill.h"

typedef struct WindowHint
{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;

	WindowHint()
	{
		memset(this, 0, sizeof(*this));
	}

	WindowHint(uint32_t param_x, uint32_t param_y, uint32_t param_w, uint32_t param_h):
			  x(param_x),y(param_y),w(param_w),h(param_h)
	{
	}

}WinHint;

typedef struct _POSITION
{
	int32_t x;
	int32_t y;
	
	_POSITION()
	{
		memset(this, 0, sizeof(*this));
	}
}POSITION;


template<typename T>
using NeedImageCb = bool (*) (T *);
using RenderWindow = int32_t;

template<typename T>
using keyCallback = void (*) (T key, int32_t x, int32_t y);


class SampleRender
{
public:
	SampleRender(const char* pRenderName, const uint32_t winSizeX, const uint32_t winSizeY);
	virtual ~SampleRender();

	bool init(int32_t argc, char **argv);
	void setDataCallback(NeedImageCb<SampleRender> needImage, void* pData);
	void setDataCallback_multithread(NeedImageCb<SampleRender> needImage);
	bool run();

	void initViewPort();
	void update();

	RenderWindow createSubWin(WinHint& winhint);
	void setSubWinPosition(WinHint& winhint);

	virtual void setKeyCallback(keyCallback<unsigned char> keyhandle) {m_keyboard = keyhandle;};

	virtual void draw(uint8_t* pRgbBuffer, uint32_t size, const WinHint& rect);
    virtual void draw(RenderWindow win, uint8_t* pRgbBuffer, uint32_t size, uint32_t width, uint32_t height, ImiImageFrame** pFrame=NULL);
    virtual void drawSkeleton(const ImiImageFrame* pFrame, WinHint& winhint);
	virtual void drawBone(const ImiSkeletonData* pSkeletonData, ImiSkeletonPositionIndex first, ImiSkeletonPositionIndex second, int32_t width, int32_t height, WinHint& winhint);
	virtual void drawCursorXYZValue(ImiImageFrame** pFrame);
	virtual void drawString(const char* str, uint32_t x, uint32_t y, void* font = (void *)0x0002);
	virtual void drawString(const char* str, uint32_t x, uint32_t y, float red, float green, float blue);

	int m_camera_id;
    int32_t m_glWin;

protected:
	virtual void display();
	virtual void display_cameraID();
	virtual bool initOpenGL(int32_t argc, char **argv);

    template <typename T>
    void onKey(T key, int32_t x, int32_t y);

private:
	SampleRender(const SampleRender&);
	SampleRender& operator=(SampleRender&);

	static void glutIdle();
	static void glutIdle_cameraID();
	static void glutDisplay();
	static void glutMouse(int button, int state, int x, int y);
	static void glutXYMouse(int x, int y);

	template <typename T>
	static void glutKeyboard(T key, int x, int y);
	static void glutWindowReshape(int width, int height);
    static void glutFuncKeyboard(int key, int x, int y);

	static SampleRender* g_pSampleRender;

	std::string	    m_strRenderName;

	uint32_t        m_nTexMapX;
    uint32_t        m_nTexMapY;

	RGB888Pixel*	m_pTexMap;

	NeedImageCb<SampleRender>     m_NeedImage;
	void*			m_pUserCookie;

	WinHint 		m_hint;
    // keyCallback is a bridge between system event and user-defined behavior.
    // 1. design a bridge formula keyCallback. 2. define an instance of the keyCallback, m_keyboardm
    // 3. link the instance m_keyboard to the user-defined behavior function, keyboardFun().
    // 4. link the instance m_keyboard to the system event response, onKey().
    // after the above steps, the system event reponse is linked to the user-defined behavior.
	keyCallback<unsigned char> m_keyboard;

	static POSITION	m_CursorPos;

	int m_nWindowWidth;
	int m_nWindowHeight;
};

#endif 
