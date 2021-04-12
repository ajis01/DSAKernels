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

#define DEBUG 1
void convertBitdepthCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels){

	TYPE max, min;
        max = 255;
        min = 0;
        TYPE val;



	int k=0;
        for(int i=0; i<srcRows; ++i){
            for(int j=0; j<srcCols; ++j){

                TYPE t = src[i*srcCols    + j*srcChannels + k];
                if(t<min) val = min;
                else if(t>max) val = max;
                else val = t;
                dest[i*destStep + j*srcChannels + k] = t;
            }
        }


}


void convertBitdepth(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels){

#pragma ss config
    {
	TYPE max, min;
        max = 255;
        min = 0;
        TYPE val;
	int i,j;
	int k=0;
	#pragma ss stream 
	for(i=0; i<srcRows; ++i){
	    //#pragma ss dfg dedicated unroll (U) 
            for(j=0; j<srcCols; ++j){
		  //TYPE t = src[i*srcCols    + j*srcChannels + k];
                  //if(src[i*srcCols    + j*srcChannels + k] < 0) dest[i*destStep + j*srcChannels + k] = 0;
                  //else if(src[i*srcCols    + j*srcChannels + k] > 255) dest[i*destStep + j*srcChannels + k] = 255;
                  //dest[i*destStep + j*srcChannels ] = src[i*srcCols    + j*srcChannels ];
		  TYPE t = src[i*col_size    + j*C + k];
                  if(src[i*col_size    + j*C + k] < 0) dest[i*col_size + j*C + k] = 0;
                  else if(src[i*col_size    + j*C + k] > 255) dest[i*col_size + j*C + k] = 255;
                  dest[i*col_size + j*C ] = src[i*col_size    + j*C ];
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
  convertBitdepthCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
  	convertBitdepth(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        //convertBitdepth(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  convertBitdepth(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  //convertBitdepth(image, dest, ALPHA, temp, temp1);
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
