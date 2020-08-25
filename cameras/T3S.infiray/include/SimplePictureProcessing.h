#ifndef SIMPLE_PICTURE_PROCESSING_H
#define SIMPLE_PICTURE_PROCESSING_H



#include<stdio.h>
#include<stdlib.h>
namespace xtherm {
class SimplePictureProcessing{
public:
    SimplePictureProcessing(int width,int weight);
    virtual ~SimplePictureProcessing();
	/*参数1：输入数据 参数2：输出数据 参数3：色板类型0-5*/
    void Compute(unsigned short* input,unsigned char* output,int kindOfPalette1);
	//设置参数 接口未完全开放
    void SetParameter(float a,float b,float c,float d,float e,float f);
	//设置用户色板，输入rgb数据256×3 typeOfPalette大于6
    void SetUserPalette(unsigned char* palette,int typeOfPalette);
private:
    void CH(unsigned short* input,int kindOfPalette1);
    void CH2(unsigned short* input,int kindOfPalette1);

private:
    int Width;
    int Height;
    int Total;
    int HP;
    int LP;
    int HT;
    int LT;
    int LPT;
    int HPT;
    int QT;
    unsigned short *BO;
    short *BSG;
    short *OSG;
    unsigned char UserPalette[256*3];
    int *SCR;
    int *AR;
    int *ARP;
    int *MT;
    unsigned char PaletteRainbow[224*3];
    unsigned char PaletteHighRainbow[448*3];

    void BFG(unsigned short *input, int nWidth, int nHeight,unsigned short *BFP, short *BFG,short *OSG);
    int SST[9];
    int SRT[20];
    float SS;
    float SR;



    void QS(int s[], int l, int r);
    int MF(unsigned short* inbuffer, int width, int height, unsigned short* outbuffer);
    int EG(int i,int x,int Max);



    void  IGFP(float DS);
    unsigned short* GSD;
    int NWS;
    int NC;
    double* PDK;
    double  DSM;
    void  GFG(unsigned short *input, int nWidth, int nHeight,unsigned short *output);
    void  GFG1D(unsigned short *A, int nWidth, int nHeight, int STD,float DS);

};//class SimplePictureProcessing
};// namespace xtherm
#endif
