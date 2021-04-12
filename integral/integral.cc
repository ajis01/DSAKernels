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
void integralCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int range){
        dest[0] = src[0];

        for (int i = 1; i < srcCols; i++) {
            dest[i] = 
                dest[i-1] 
                + src[i];
        }

        for (int j = 1; j < srcRows; j++) {
            dest[j*srcStep] = 
                dest[(j-1)*srcStep] 
                + src[j*srcStep];
        }    

        for (int i = 1; i < srcRows; i++) {
            for (int j = 1; j < srcCols; j++) {
                int rowOffset = i*srcStep;
                dest[rowOffset + j] = 
                    src[rowOffset + j]
                    +dest[rowOffset + j-1]
                    +dest[rowOffset-srcStep + j]
                    -dest[rowOffset-srcStep + j-1];
            }
        }
}


void integral(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int range){

#pragma ss config
    {
        dest[0] = src[0];

        #pragma ss stream
        //#pragma ss dfg dedicated unroll (U) 
        for (int i = 1; i < srcCols; i++) {
            dest[i] = 
                dest[i-1] 
                + src[i];
        }

        #pragma ss stream
        //#pragma ss dfg dedicated unroll (U) 
        for (int j = 1; j < srcRows; j++) {
            dest[j*srcStep] = 
                dest[(j-1)*srcStep] 
                + src[j*srcStep];
        }    

        #pragma ss stream
        for (int i = 1; i < srcRows; i++) {
            //#pragma ss dfg dedicated unroll (U) 
            for (int j = 1; j < srcCols; j++) {
                int rowOffset = i*srcStep;
                dest[rowOffset + j] = 
                    src[rowOffset + j]
                    +dest[rowOffset + j-1]
                    +dest[rowOffset-srcStep + j]
                    -dest[rowOffset-srcStep + j-1];
            }
        }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  int range = 3;
  integralCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, range);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        integral(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, range);
        //integral(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  integral(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, range);
  //integral(image, dest, ALPHA, temp, temp1);
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
