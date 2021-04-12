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

typedef enum {
        L1Norm = 0,
        L2Norm
}normTypeEnum_t;

void meanFilterCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int range){


        for (int r = range; r < srcRows-range; r++) {
            for (int c = range; c < srcStep-range; c++) {
                TYPE lowerLeft = (r+range)*srcStep + c-range;
                TYPE upperLeft = (r-range)*srcStep + c-range;
                TYPE lowerRight = (r+range)*srcStep + c+range;
                TYPE upperRight = (r-range)*srcStep + c+range;
                dest[upperLeft] =
                    src[lowerRight]
                    + src[upperLeft]
                    - src[lowerLeft]
                    - src[upperRight];
            }
        }
}


void meanFilter(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int range){

#pragma ss config
    {
	#pragma ss stream
        for (int r = range; r < srcRows-range; r++) {
	   // #pragma ss dfg dedicated unroll (U) 
            for (int c = range; c < srcStep-range; c++) {
                TYPE lowerLeft = (r+range)*srcStep + c-range;
                TYPE upperLeft = (r-range)*srcStep + c-range;
                TYPE lowerRight = (r+range)*srcStep + c+range;
                TYPE upperRight = (r-range)*srcStep + c+range;
                dest[upperLeft] =
                    src[lowerRight]
                    + src[upperLeft]
                    - src[lowerLeft]
                    - src[upperRight];
            }
        }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  int range = 3;
  meanFilterCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, range);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        meanFilter(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, range);
        //meanFilter(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  meanFilter(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, range);
  //meanFilter(image, dest, ALPHA, temp, temp1);
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
