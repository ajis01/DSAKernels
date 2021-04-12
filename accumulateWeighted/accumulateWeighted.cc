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
#define N row_size*col_size
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif
void accumulateWeightedCpu(const TYPE* image1, int imageRows, int imageCols, int imageStep, int imageChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, double ALPHA){

        double  temp = (ALPHA * ((1<<23) - 1));
        double  temp1 = ((1<<23) - 1) - temp + 1;
	int64_t twoPower23 = 8388608;

	int64_t jj, i, j, k;
	int64_t rows = imageRows; 
	int64_t cols = imageCols; 
	int64_t channels = imageChannels;
	//GRAY
            for (i=0; i<rows; ++i){
	      for (j=0; j<cols; ++j){
                  dest[i*cols + j] = (image1[i*cols + j]*temp + dest[i*cols + j]*temp1)/twoPower23;
	      }
	    }
}


void accumulateWeighted(TYPE src[N], TYPE dest[N], double ALPHA, double temp1, double temp2){

    #pragma ss config
    {
	int64_t twoPower23 = 8388608;
	int64_t jj, i, j, k;
	int64_t rows = row_size; 
	int64_t cols = col_size; 
	int64_t channels = C;
	int64_t firstcmp ;
        int64_t secondcmp ;
	//GRAY
          #pragma ss stream 
            for (i=0; i<rows; ++i){
              #pragma ss dfg dedicated unroll(U)
	        for (j=0; j<cols; ++j){
                  dest[i*cols + j] = (src[i*cols + j] *temp1+ dest[i*cols + j]*temp2)/twoPower23;
	        }
	    }
	//////COLOR
        ////  #pragma ss stream 
        ////    for (i=0; i<rows; ++i){
        ////      #pragma ss dfg dedicated unroll(U) 
	////      for (j=0; j<cols; ++j){
        ////        dest[i*cols + j*C] += src[i*cols + j*C];
        ////        //dest[i*cols + j*C + 1] += src[i*cols + j*C + 1];
        ////        //dest[i*cols + j*C + 2] += src[i*cols + j*C + 2];
	////      }
	////    }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {
  
  double ALPHA = 0.5;
  double temp = (ALPHA * ((1<<23) - 1));
  double temp1 = ((1<<23) - 1) - temp + 1;
  accumulateWeightedCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, ALPHA);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        //accumulateWeighted(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        accumulateWeighted(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  //accumulateWeighted(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  accumulateWeighted(image, dest, ALPHA, temp, temp1);
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
