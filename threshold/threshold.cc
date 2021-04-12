/*
Implementation based on algorithm described in:
The cache performance and optimizations of blocked algorithms
M. D. Lam, E. E. Rothberg, and M. E. Wolf
ASPLOS 1991
*/

#include "sim_timing.h"
#include "assert.h"
#include<cmath>
#include<stdlib.h>
#include<time.h>
#include<algorithm>
#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include"lineartextdata/128x128_gray.h"

#define TYPE int64_t 
#define CACHE_WARMUP 5
#undef N

//Algorithm Parameters
#define row_size 128 
#define col_size 128
#define GRAY 1
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

typedef enum{
	THRESH_BINARY,
	THRESH_BINARY_INV,
	THRESH_TRUNC,
	THRESH_TOZERO,
	THRESH_TOZERO_INV
}threshType_t;

void thresholdCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    double* dest, int destRows, int destCols, int destStep, int destChannels, TYPE maxval, TYPE thresh, threshType_t thresh_type){
            for(int i=0; i<srcRows; ++i){
                for(int j=0; j<srcCols; ++j){

                        switch (thresh_type)
                        {
                        case THRESH_BINARY:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    maxval : 0;
                            break;

                        case THRESH_BINARY_INV:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    0 : maxval;
                            break;

                        case THRESH_TRUNC:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    thresh : src[i*srcStep + j*srcChannels];
                            break;

                        case THRESH_TOZERO:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    src[i*srcStep + j*srcChannels] : 0;
                            break;

                        case THRESH_TOZERO_INV:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    0 : src[i*srcStep + j*srcChannels];
                            break;
                        
                        default:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels];
                            break;
                        }
                }
            }
}



void threshold(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    double* dest, int destRows, int destCols, int destStep, int destChannels, TYPE maxval, TYPE thresh, threshType_t thresh_type){
#pragma ss config
    {
            #pragma ss stream
            for(int i=0; i<srcRows; ++i){
                //#pragma ss dfg dedicated 
                for(int j=0; j<srcCols; ++j){

                        switch (thresh_type)
                        {
                        case THRESH_BINARY:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    maxval : 0;
                            break;

                        case THRESH_BINARY_INV:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    0 : maxval;
                            break;

                        case THRESH_TRUNC:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    thresh : src[i*srcStep + j*srcChannels];
                            break;

                        case THRESH_TOZERO:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    src[i*srcStep + j*srcChannels] : 0;
                            break;

                        case THRESH_TOZERO_INV:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels] > thresh ? 
                                                                    0 : src[i*srcStep + j*srcChannels];
                            break;
                        
                        default:
                            dest[i*destStep + j*srcChannels] = src[i*srcStep + j*srcChannels];
                            break;
                        }
                }
            }
    }
}

double dest[N];
double dest1[N];

int main() {
  
  TYPE maxval = 50;
  TYPE thresh = 100;
  threshType_t treshType = THRESH_TOZERO;
  thresholdCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, maxval, thresh, treshType);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        threshold(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, maxval, thresh, treshType);
        //threshold(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  threshold(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, maxval, thresh, treshType);
  //threshold(image, dest, ALPHA, temp, temp1);
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
