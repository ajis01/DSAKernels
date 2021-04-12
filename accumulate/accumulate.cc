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
#define U 1 
#endif
void accumulateCpu(const TYPE* image1, int imageRows, int imageCols, int imageStep, int imageChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels){
    assert(imageRows == destRows && imageCols == destCols && imageStep == destStep && imageChannels==destChannels);

	int64_t jj, i, j, k;
	TYPE temp1, temp2;
	int64_t rows = imageRows; 
	int64_t cols = imageCols; 
	int64_t channels = imageChannels;
	//GRAY
            for (i=0; i<rows; ++i){
	      for (j=0; j<cols; ++j){
                dest[i*cols + j] += image1[i*cols + j];
	      }
	    }
}


void accumulate(TYPE src[N], TYPE dest[N]){

    #pragma ss config
    {
	int64_t jj, i, j, k;
	TYPE temp1, temp2;
	TYPE temp[N];
	int64_t rows = row_size; 
	int64_t cols = col_size; 
	int64_t channels = C;
	//GRAY
          #pragma ss stream nonblock 
            for (i=0; i<rows; ++i){
              #pragma ss dfg dedicated unroll(U)
	        for (j=0; j<cols; ++j){
                  dest[i*cols + j] += src[i*cols + j]; 
		  //temp1 = src[i*cols + j];
		  //temp1 = temp1*temp1;
                  //dest[i*cols + j] += temp1;
	        }
	    }
	////COLOR
        //  #pragma ss stream 
        //    for (i=0; i<rows; ++i){
        //      #pragma ss dfg dedicated unroll(U) 
	//      for (j=0; j<cols; ++j){
        //        dest[i*cols + j*C] += src[i*cols + j*C];
        //        //dest[i*cols + j*C + 1] += src[i*cols + j*C + 1];
        //        //dest[i*cols + j*C + 2] += src[i*cols + j*C + 2];
	//      }
	//    }
    }
}

TYPE dest[N];
TYPE dest1[N];

int main() {

  accumulateCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        //accumulate(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        accumulate(image, dest);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  //accumulate(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  accumulate(image, dest);
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
