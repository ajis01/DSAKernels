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
#define N row_size*col_size*C
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif
#include <stdint.h>
#include <stdio.h>


#include <sys/time.h>

void sumCpu(const TYPE* image1, int imageRows, int imageCols, int imageStep, int imageChannels, TYPE dest1){

	int64_t jj, i, j, k;
	TYPE temp1, temp2;
	int64_t rows = imageRows; 
	int64_t cols = imageCols; 
	int64_t channels = imageChannels;
	//GRAY
            for (i=0; i<rows; ++i){
	      for (j=0; j<cols; ++j){
                dest1 += image1[i*cols + j];
	      }
	    }
}


void sum(TYPE src[N], TYPE dest){

    #pragma ss config
    {
	int64_t jj, i, j, k;
	int64_t acc = 0;
	TYPE temp1, temp2;
	int64_t rows = row_size; 
	int64_t cols = col_size; 
	int64_t channels = C;
	//GRAY
          #pragma ss stream nonblock 
            for (i=0; i<rows; ++i){
                #pragma ss dfg dedicated
	        for (j=0; j<cols; ++j){
                  acc +=  src[i*cols + j]; 
	      }
	    }
    }
}

TYPE dest=0;
TYPE dest1=0;

int main() {

  sumCpu(image, row_size, col_size, col_size, C, dest1);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        sum(image, dest);

  dest = 0;

  begin_roi();
  sum(image, dest);
  end_roi();
  sb_stats();

  
  std::cout << "ERROR = " << dest-dest1 << std::endl;

  return 0;
}
