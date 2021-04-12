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
void bgr2grayscaleCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels){

	for(int i=0; i<srcRows; ++i){
            for(int j=0; j<srcCols; ++j){
                int indexSrc = i*srcStep + j;
                int indexDest = i*destStep + j;
                int indexColor = i*srcStep + j*srcChannels;
                dest[indexDest] =
                    0.0722 * src[indexColor] +
                    0.7152 * src[indexColor+1] +
                    0.2126 * src[indexColor+2];
            }
        }

}


void bgr2grayscale(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels){

#pragma ss config
    {
	#pragma ss stream 
	for(int i=0; i<srcRows; ++i){
	    #pragma ss dfg dedicated unroll (U) 
            for(int j=0; j<srcCols; ++j){
                int indexSrc = i*srcStep + j;
                int indexDest = i*destStep + j;
                int indexColor = i*srcStep + j*srcChannels;
                dest[indexDest] =
                    0.0722 * src[indexColor] +
                    0.7152 * src[indexColor+1] +
                    0.2126 * src[indexColor+2];
            }
        }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      //dest[i*col_size + j] = 0;
      dest1[i*col_size + j] = 0;
    }
  }
  bgr2grayscaleCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
  	bgr2grayscale(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        //bgr2grayscale(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  bgr2grayscale(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  //bgr2grayscale(image, dest, ALPHA, temp, temp1);
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
