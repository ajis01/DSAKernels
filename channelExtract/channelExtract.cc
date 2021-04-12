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
#define CACHE_WARMUP 2
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
#define NGray row_size*col_size
#define CGray 1
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif

#define DEBUG 1
void channelExtractCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int channel){
            for(int i=0; i<srcRows; ++i){
                for(int j=0; j<srcCols; ++j){
                    dest[i*destStep + j] = src[i*srcStep + j*srcChannels + channel];
                }
            }

}


void channelExtract(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, int channel){

#pragma ss config
    {
	#pragma ss stream nonblock 
            for(int i=0; i<srcRows; ++i){
		//#pragma ss dfg dedicated
                for(int j=0; j<srcCols; ++j){
                    dest[i*destStep + j] = src[i*srcStep + j*srcChannels + channel];
                }
            }
    }
}

TYPE dest[NGray];
TYPE dest1[NGray];

int main() {

  int Channel = 2;
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      //dest[i*col_size + j] = 0;
      dest1[i*col_size + j] = 0;
    }
  }
  channelExtractCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, CGray, Channel);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
  	channelExtract(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, CGray, Channel);
        //channelExtract(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  channelExtract(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, CGray, Channel);
  //channelExtract(image, dest, ALPHA, temp, temp1);
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
  
  std::cout << "ERROR percent = " << err_cnt/(row_size*col_size*CGray) << std::endl;
  std::cout << "ERROR count = " << err_cnt << " out of " << row_size*col_size*CGray << std::endl;
#endif

  return 0;
}
