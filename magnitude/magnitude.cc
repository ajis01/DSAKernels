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

void magnitudeCpu(const TYPE* src1, int src1Rows, int src1Cols, int src1Step, int src1Channels,
		    TYPE* src2, int src2Rows, int src2Cols, int src2Step, int src2Channels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, normTypeEnum_t normType){

	TYPE p, q, result;

        for(int i=0; i<src1Rows; ++i){
            for(int j=0; j<src1Cols; ++j){
                    p = src1[i*src1Step + j*src1Channels];
                    q = src2[i*src1Step + j*src1Channels];
                    switch(normType){
                        case L1Norm:
                            result = std::abs(p) + std::abs(q);
                            break;
                        case L2Norm:
                            TYPE tempgx = p*p;
                            TYPE tempgy = q*q;
                            TYPE temp = std::sqrt(tempgx + tempgy);
                            result = temp;
                    }
                    dest[i*destStep + j*src1Channels] = result;
            }
        }

}


void magnitude(const TYPE* src1, int src1Rows, int src1Cols, int src1Step, int src1Channels,
		    TYPE* src2, int src2Rows, int src2Cols, int src2Step, int src2Channels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, normTypeEnum_t normType){

#pragma ss config
    {
	TYPE p, q, result;

        #pragma ss stream
        for(int i=0; i<src1Rows; ++i){
            //#pragma ss dfg dedicated 
            for(int j=0; j<src1Cols; ++j){
                    p = src1[i*src1Step + j*src1Channels];
                    q = src2[i*src1Step + j*src1Channels];
                    switch(normType){
                        case L1Norm:
                            result = std::abs(p) + std::abs(q);
                            break;
                        case L2Norm:
                            TYPE tempgx = p*p;
                            TYPE tempgy = q*q;
                            TYPE temp = std::sqrt(tempgx + tempgy);
                            result = temp;
                    }
                    dest[i*destStep + j*src1Channels] = result;
            }
        }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  normTypeEnum_t normType = L1Norm;
  magnitudeCpu(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, normType);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        magnitude(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, normType);
        //magnitude(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  magnitude(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, normType);
  //magnitude(image, dest, ALPHA, temp, temp1);
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
