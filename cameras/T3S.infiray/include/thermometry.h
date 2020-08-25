
#ifndef THERMOMETRY_H_
#define THERMOMETRY_H_

#include <stdbool.h>
#ifdef __cplusplus
extern "C"  //C++
{
#endif
   void thermometryT(  int width,//宽
					int height,//高
					float *temperatureTable,//温度表
					unsigned short *orgData,//原始数据
					float *temperatureData,//温度数据
					bool needRefreshTable,//是否需要更新温度表
					int rangeMode,//测温范围：120：温度范围为-20-120摄氏度；400：温度范围为-20-400摄氏度
					float *floatFpaTmp,//fpa温度
					float* fix, //修正
					float* Refltmp,//反射温度
					float* Airtmp,//环境温度
					float* humi,//湿度
					float* emiss,//发射率
					unsigned short* distance);//距离
					
#ifdef __cplusplus
}
#endif

#endif
 
