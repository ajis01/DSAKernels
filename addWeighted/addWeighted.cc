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
#include"lineartextdata/128x128_gray.h"

#define TYPE int64_t 
#define CACHE_WARMUP 2
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
#define N row_size*col_size
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif
void addWeightedCpu(const TYPE* image1, int image1Rows, int image1Cols, int image1Step, int image1Channels,
		    const TYPE* image2, int image2Rows, int image2Cols, int image2Step, int image2Channels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, double ALPHA, double BETA, double GAMMA){

	int k = 0;
            for(int i=0; i<image1Rows; ++i){
                for(int j=0; j<image1Cols; ++j){
                    int32_t firstcmp = image1[i*image1Step + j*image1Channels + k] * ALPHA;
                    int32_t secondcmp = image2[i*image1Step + j*image1Channels + k] * BETA;
                    int16_t t = (firstcmp + secondcmp + GAMMA);
                    if (t > 255) {
                        t = 255;
                    } else if (t < 0) {
                        t = 0;
                    }
                    dest[i*destStep + j*image1Channels + k] = (TYPE)t;
                }
            }
}


void addWeighted(const TYPE* image1, int image1Rows, int image1Cols, int image1Step, int image1Channels,
		    const TYPE* image2, int image2Rows, int image2Cols, int image2Step, int image2Channels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, double ALPHA, double BETA, double GAMMA){

#pragma ss config
    {
	int k = 0;
	    #pragma ss stream 
            for(int i=0; i<image1Rows; ++i){
		#pragma ss dfg dedicated unroll (U)
                for(int j=0; j<image1Cols; ++j){
                    int32_t firstcmp = image1[i*image1Step + j*image1Channels + k] * ALPHA;
                    int32_t secondcmp = image2[i*image1Step + j*image1Channels + k] * BETA;
                    int16_t t = (firstcmp + secondcmp + GAMMA);
                    if (t > 255) {
                        t = 255;
                    } else if (t < 0) {
                        t = 0;
                    }
                    dest[i*destStep + j*image1Channels + k] = (TYPE)t;
                }
            }
     }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  double ALPHA = 0.2;
  double BETA = 0.3;
  double GAMMA = 0.0;
  addWeightedCpu(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, ALPHA, BETA, GAMMA);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        addWeighted(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, ALPHA, BETA, GAMMA);
        //addWeighted(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  addWeighted(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, ALPHA, BETA, GAMMA);
  //addWeighted(image, dest, ALPHA, temp, temp1);
  end_roi();
  sb_stats();

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

  return 0;
}
