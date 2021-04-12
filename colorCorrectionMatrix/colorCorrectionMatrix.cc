/*
Implementation based on algorithm described in:
The cache performance and optimizations of blocked algorithms
M. D. Lam, E. E. Rothberg, and M. E. Wolf
ASPLOS 1991
*/

#include "sim_timing.h"
#include "assert.h"
#include<stdlib.h>
#include<time.h>
#include<algorithm>
#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include"lineartextdata/128x128_color.h"

#define TYPE int64_t 
#define CACHE_WARMUP 5
#undef N

//Algorithm Parameters
#define row_size 128 
#define col_size 128
#define GRAY 0
#if GRAY
  #define C 1
#else 
  #define C 3
#endif
#define N row_size*col_size*C
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif

#define DEBUG 1

typedef enum {
        bt2020_bt709 = 0,
        bt709_bt2020,
        rgb_yuv_601,
        rgb_yuv_709,
        rgb_yuv_2020,
        yuv_rgb_601,
        yuv_rgb_709,
        yuv_rgb_2020,
        full_to_16_235,
        full_from_16_235
}ccmTypeEnum_t;

    const double bt2020_bt709_arr[3][3] = {
        {1.6605, -0.5876, -0.0728}, {-0.1246, 1.1329, -0.0083}, {-0.0182, -0.1006, 1.1187}};

    const double bt2020_bt709_off[3] = {0.0, 0.0, 0.0};

    const double bt709_bt2020_arr[3][3] = {{0.627, 0.329, 0.0433}, {0.0691, 0.92, 0.0113}, {0.0164, 0.088, 0.896}};

    const double bt709_bt2020_off[3] = {0.0, 0.0, 0.0};

    const double rgb_yuv_601_arr[3][3] = {{0.257, 0.504, 0.098}, {-0.148, -0.291, 0.439}, {0.439, -0.368, -0.071}};

    const double rgb_yuv_601_off[3] = {0.0625, 0.500, 0.500};

    const double rgb_yuv_709_arr[3][3] = {{0.183, 0.614, 0.062}, {-0.101, -0.338, 0.439}, {0.439, -0.399, -0.040}};

    const double rgb_yuv_709_off[3] = {0.0625, 0.500, 0.500};

    const double rgb_yuv_2020_arr[3][3] = {
        {0.225613, 0.582282, 0.050928}, {-0.119918, -0.309494, 0.429412}, {0.429412, -0.394875, -0.034537}};

    const double rgb_yuv_2020_off[3] = {0.062745, 0.500, 0.500};

    const double yuv_rgb_601_arr[3][3] = {{1.164, 0.000, 1.596}, {1.164, -0.813, -0.391}, {1.164, 2.018, 0.000}};

    const double yuv_rgb_601_off[3] = {-0.87075, 0.52925, -1.08175};

    const double yuv_rgb_709_arr[3][3] = {{1.164, 0.000, 1.793}, {1.164, -0.213, -0.534}, {1.164, 2.115, 0.000}};

    const double yuv_rgb_709_off[3] = {-0.96925, 0.30075, -1.13025};

    const double yuv_rgb_2020_arr[3][3] = {
        {1.164384, 0.000000, 1.717000}, {1.164384, -0.191603, -0.665274}, {1.164384, 2.190671, 0.000000}};

    const double yuv_rgb_2020_off[3] = {-0.931559, 0.355379, -1.168395};

    const double full_to_16_235_arr[3][3] = {
        {0.856305, 0.000000, 0.000000}, {0.000000, 0.856305, 0.000000}, {0.000000, 0.000000, 0.856305}};

    const double full_to_16_235_off[3] = {0.0625, 0.0625, 0.0625};

    const double full_from_16_235_arr[3][3] = {
        {1.167808, 0.000000, 0.000000}, {0.000000, 1.167808, 0.000000}, {0.000000, 0.000000, 1.167808}};

    const double full_from_16_235_off[3] = {-0.0729880, -0.0729880, -0.0729880};

    inline unsigned char satcast_ccm(int v) {
        v = (v > 255 ? 255 : v);
        v = (v < 0 ? 0 : v);
        return v;
    };


void colorCorrectionMatrixCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, ccmTypeEnum_t ccmType){
	TYPE ccm_matrix[3][3];
        TYPE offsetarray[3];

        switch (ccmType) {
            case bt2020_bt709:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = bt2020_bt709_arr[i][j];
                    }
                    offsetarray[i] = bt2020_bt709_off[i];
                }

                break;
            case bt709_bt2020:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = bt709_bt2020_arr[i][j];
                    }
                    offsetarray[i] = bt709_bt2020_off[i];
                }

                break;
            case rgb_yuv_601:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = rgb_yuv_601_arr[i][j];
                    }
                    offsetarray[i] = rgb_yuv_601_off[i];
                }

                break;
            case rgb_yuv_709:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = rgb_yuv_709_arr[i][j];
                    }
                    offsetarray[i] = rgb_yuv_709_off[i];
                }

                break;
            case rgb_yuv_2020:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = rgb_yuv_2020_arr[i][j];
                    }
                    offsetarray[i] = rgb_yuv_2020_off[i];
                }

                break;
            case yuv_rgb_601:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = yuv_rgb_601_arr[i][j];
                    }
                    offsetarray[i] = yuv_rgb_601_off[i];
                }

                break;
            case yuv_rgb_709:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = yuv_rgb_709_arr[i][j];
                    }
                    offsetarray[i] = yuv_rgb_709_off[i];
                }

                break;
            case yuv_rgb_2020:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = yuv_rgb_2020_arr[i][j];
                    }
                    offsetarray[i] = yuv_rgb_2020_off[i];
                }

                break;
            case full_to_16_235:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = full_to_16_235_arr[i][j];
                    }
                    offsetarray[i] = full_to_16_235_off[i];
                }

                break;
            case full_from_16_235:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = full_from_16_235_arr[i][j];
                    }
                    offsetarray[i] = full_from_16_235_off[i];
                }

                break;
            default:
                break;
        }


        TYPE r;
        TYPE g;
        TYPE b;

        TYPE value_r;
        TYPE value_g;
        TYPE value_b;

        TYPE value1;
        TYPE value2;
        TYPE value3;
        TYPE value4;
        TYPE value5;
        TYPE value6;
        TYPE value7;
        TYPE value8;
        TYPE value9;

        for(int i=0; i<srcRows; ++i){
            for(int j=0; j<srcCols; ++j){
                    r = src[i*srcStep + j*srcChannels];
                    g = src[i*srcStep + j*srcChannels + 1];
                    b = src[i*srcStep + j*srcChannels + 2];

                    value1 = (r * ccm_matrix[0][0]);
                    value2 = (g * ccm_matrix[0][1]);
                    value3 = (b * ccm_matrix[0][2]);

                    value4 = (r * ccm_matrix[1][0]);
                    value5 = (g * ccm_matrix[1][1]);
                    value6 = (b * ccm_matrix[1][2]);

                    value7 = (r * ccm_matrix[2][0]);
                    value8 = (g * ccm_matrix[2][1]);
                    value9 = (b * ccm_matrix[2][2]);

                    value_r = (value1 + value2 + value3 + offsetarray[0]);
                    value_g = (value4 + value5 + value6 + offsetarray[1]);
                    value_b = (value7 + value8 + value9 + offsetarray[2]);

                    dest[i*srcStep + j*srcChannels]     = satcast_ccm(value_r);
                    dest[i*srcStep + j*srcChannels + 1] = satcast_ccm(value_g);
                    dest[i*srcStep + j*srcChannels + 2] = satcast_ccm(value_b);

            }
        }
}


void colorCorrectionMatrix(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, ccmTypeEnum_t ccmType){

#pragma ss config
    {
	TYPE ccm_matrix[3][3];
        TYPE offsetarray[3];

        switch (ccmType) {
            case bt2020_bt709:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = bt2020_bt709_arr[i][j];
                    }
                    offsetarray[i] = bt2020_bt709_off[i];
                }

                break;
            case bt709_bt2020:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = bt709_bt2020_arr[i][j];
                    }
                    offsetarray[i] = bt709_bt2020_off[i];
                }

                break;
            case rgb_yuv_601:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = rgb_yuv_601_arr[i][j];
                    }
                    offsetarray[i] = rgb_yuv_601_off[i];
                }

                break;
            case rgb_yuv_709:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = rgb_yuv_709_arr[i][j];
                    }
                    offsetarray[i] = rgb_yuv_709_off[i];
                }

                break;
            case rgb_yuv_2020:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = rgb_yuv_2020_arr[i][j];
                    }
                    offsetarray[i] = rgb_yuv_2020_off[i];
                }

                break;
            case yuv_rgb_601:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = yuv_rgb_601_arr[i][j];
                    }
                    offsetarray[i] = yuv_rgb_601_off[i];
                }

                break;
            case yuv_rgb_709:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = yuv_rgb_709_arr[i][j];
                    }
                    offsetarray[i] = yuv_rgb_709_off[i];
                }

                break;
            case yuv_rgb_2020:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = yuv_rgb_2020_arr[i][j];
                    }
                    offsetarray[i] = yuv_rgb_2020_off[i];
                }

                break;
            case full_to_16_235:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = full_to_16_235_arr[i][j];
                    }
                    offsetarray[i] = full_to_16_235_off[i];
                }

                break;
            case full_from_16_235:
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        ccm_matrix[i][j] = full_from_16_235_arr[i][j];
                    }
                    offsetarray[i] = full_from_16_235_off[i];
                }

                break;
            default:
                break;
        }


        TYPE r;
        TYPE g;
        TYPE b;

        TYPE value_r;
        TYPE value_g;
        TYPE value_b;

        TYPE value1;
        TYPE value2;
        TYPE value3;
        TYPE value4;
        TYPE value5;
        TYPE value6;
        TYPE value7;
        TYPE value8;
        TYPE value9;

	#pragma ss stream nonblock
        for(int i=0; i<srcRows; ++i){
	    //#pragma ss dfg dedicated 
            for(int j=0; j<srcCols; ++j){
                    r = src[i*srcStep + j*srcChannels];
                    g = src[i*srcStep + j*srcChannels + 1];
                    b = src[i*srcStep + j*srcChannels + 2];

                    value1 = (r * ccm_matrix[0][0]);
                    value2 = (g * ccm_matrix[0][1]);
                    value3 = (b * ccm_matrix[0][2]);

                    value4 = (r * ccm_matrix[1][0]);
                    value5 = (g * ccm_matrix[1][1]);
                    value6 = (b * ccm_matrix[1][2]);

                    value7 = (r * ccm_matrix[2][0]);
                    value8 = (g * ccm_matrix[2][1]);
                    value9 = (b * ccm_matrix[2][2]);

                    value_r = (value1 + value2 + value3 + offsetarray[0]);
                    value_g = (value4 + value5 + value6 + offsetarray[1]);
                    value_b = (value7 + value8 + value9 + offsetarray[2]);

                    dest[i*srcStep + j*srcChannels]     = satcast_ccm(value_r);
                    dest[i*srcStep + j*srcChannels + 1] = satcast_ccm(value_g);
                    dest[i*srcStep + j*srcChannels + 2] = satcast_ccm(value_b);

            }
        }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {

 ccmTypeEnum_t ccmType = bt2020_bt709;
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      //dest[i*col_size + j] = 0;
      dest1[i*col_size + j] = 0;
    }
  }
  colorCorrectionMatrixCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, ccmType);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
  	colorCorrectionMatrix(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, ccmType);
        //colorCorrectionMatrix(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  colorCorrectionMatrix(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, ccmType);
  //colorCorrectionMatrix(image, dest, ALPHA, temp, temp1);
  end_roi();
  sb_stats();

#if DEBUG  

  double err_cnt = 0;
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      if(dest[i*col_size + j] != dest1[i*col_size + j]){
              err_cnt++;
              //std::cout << dest[i*col_size + j] << ":" << dest1[i*col_size + j] << std::endl;
      }
      //      std::cout << dest[i*col_size + j] << std::endl;
    }
  }
  
  std::cout << "ERROR percent = " << err_cnt/(row_size*col_size*C) << std::endl;
  std::cout << "ERROR count = " << err_cnt << " out of " << row_size*col_size*C << std::endl;
#endif

  return 0;
}
