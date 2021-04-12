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
void phaseCpu(const TYPE* src1, int src1Rows, int src1Cols, int src1Step, int src1Channels,
		const TYPE* src2, int src2Rows, int src2Cols, int src2Step, int src2Channels,
                    double* dest, int destRows, int destCols, int destStep, int destChannels){
	
	for(int i=0; i<src1Rows; ++i){
            for(int j=0; j<src1Step; ++j){
               TYPE t1 = src1[i*src1Step + j*src1Channels];
               TYPE t2 = src2[i*src1Step + j*src1Channels];
               if(t1==0 && t2==0) dest[i*src1Step + j*src1Channels] = 0;
               else dest[i*src1Step + j*src1Channels] = std::atan2(t2,t1);
            }
        }
}



void phase(const TYPE* src1, int src1Rows, int src1Cols, int src1Step, int src1Channels,
		const TYPE* src2, int src2Rows, int src2Cols, int src2Step, int src2Channels,
                    double* dest, int destRows, int destCols, int destStep, int destChannels){
#pragma ss config
    {
	#pragma ss stream
	for(int i=0; i<src1Rows; ++i){
	    //#pragma ss dfg dedicated 
            for(int j=0; j<src1Step; ++j){
               TYPE t1 = src1[i*src1Step + j*src1Channels];
               TYPE t2 = src2[i*src1Step + j*src1Channels];
               if(t1==0 && t2==0) dest[i*src1Step + j*src1Channels] = 0;
               else dest[i*src1Step + j*src1Channels] = std::atan2(t2,t1);
            }
        }
    }
}

double dest[N];
double dest1[N];

int main() {
  
  phaseCpu(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        phase(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        //phase(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  phase(image, row_size, col_size, col_size, C, image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  //phase(image, dest, ALPHA, temp, temp1);
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
