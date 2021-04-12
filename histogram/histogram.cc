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
void histogramCpu(const TYPE* image1, int imageRows, int imageCols, int imageStep, int imageChannels,
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
                dest[(unsigned char)image1[i*cols + j]] += 1;
	      }
	    }
}


void histogram(TYPE src[N], TYPE dest[N]){

    #pragma ss config
    {
	int64_t jj, i, j, k;
	TYPE temp1, temp2;
	TYPE temp[N];
	int64_t rows = row_size; 
	int64_t cols = col_size; 
	int64_t channels = C;
	//GRAY
          #pragma ss stream
            for (i=0; i<rows; ++i){
              #pragma ss dfg dedicated unroll(U)
	        for (j=0; j<cols; ++j){
                  dest[(unsigned char)src[i*cols + j]] += 1; 
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

TYPE dest[256];
TYPE dest1[256];

int main() {

  for(int i=0; i<256; ++i){
	  dest1[i] = 0;
  }
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){   
      int64_t t = image[i*col_size + j];
      t = t > 255 ? 255 : t;
      image[i*col_size + j] = t;
    }
  }

  histogramCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        //histogram(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        histogram(image, dest);


  //for(int i=0; i<row_size; ++i){
  //  for(int j=0; j<col_size; ++j){
  //    dest[i*col_size + j] = 0;
  //    //dest1[i*col_size + j] = 0;
  //  }
  //}
  for(int i=0; i<256; ++i){
	  dest[i] = 0;
  }

  begin_roi();
  //histogram(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  histogram(image, dest);
  end_roi();
  sb_stats();

  double err_cnt = 0;
  
  for(int i=0; i<256; ++i){
      if(dest[i] != dest1[i]){
              err_cnt++;
      }
  }
  
  std::cout << "ERROR percent = " << err_cnt/256 << std::endl;
  std::cout << "ERROR count = " << err_cnt << " out of 256" << std::endl;

  return 0;
}
