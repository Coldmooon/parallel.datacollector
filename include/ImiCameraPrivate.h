/********************************************************************************
Copyright (c) 2015-2017 Huajie IMI Technology Co., Ltd.
All rights reserved.

@File Name    : ImiCamera.h
@Author        : Yang Feiyi
@Date           : 2017-03-24
@Description  : ImiCamera Application Develop API
@Version       : 0.1.0

@History       :
1.2017-03-24 Yang Feiyi Created file
********************************************************************************/
#ifndef _IMI_CAMERA_PRIVATE_H_
#define _IMI_CAMERA_PRIVATE_H_

#include "ImiCameraDefines.h"

/*
初始化设备并获取设备列表
*/
IMICAMERA_EXP int32_t getCamAttrList(ImiCamAttribute** pList, int32_t* nCount);
/*
销毁设备列表并释放设备
*/
IMICAMERA_EXP int32_t destroyCamAttrList();

/*
使用uri打开设备
*/
IMICAMERA_EXP int32_t imiCamOpenURI(const char* pURI, ImiCameraHandle* pCameraDevice);

/*
关闭uri设备
*/
IMICAMERA_EXP int32_t imiCamCloseURI(ImiCameraHandle cameraDevice);


#endif
