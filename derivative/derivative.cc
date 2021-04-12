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


#define DEBUG 1
void derivativeCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
		   TYPE* derH1, TYPE* derV1, TYPE* Ix1, TYPE* Iy1, TYPE* Ixy1){

	for(int r=1; r<srcRows-1; r++) {
            for(int c=0; c<srcCols; c++) {
                int rowOffset = r*srcStep;
                TYPE a1 = src[rowOffset-srcStep+c];
                TYPE a2 = src[rowOffset+c];
                TYPE a3 = src[rowOffset+srcStep+c];

                derV1[rowOffset-srcStep+c] = a1 + a2 + a2 + a3;
            }
        }

        for(int r=0; r<srcRows; r++) {
            for(int c=1; c<srcCols-1; c++) {
                int rowOffset = r*srcStep;
                TYPE a1 = src[rowOffset + c-1];
                TYPE a2 = src[rowOffset + c];
                TYPE a3 = src[rowOffset + c+1];

                derH1[rowOffset+c-1] = a1 + a2 + a2 + a3;
            }
        }

        for(int r=0; r<srcRows-2; r++) {
            for(int c=0; c<srcCols-2; c++) {
                int rowOffset = r*srcStep;
                Ix1[rowOffset+c] = derH1[rowOffset+c] - derH1[rowOffset+2*srcStep+c];
                Iy1[rowOffset+c] = - derV1[rowOffset+c] + derV1[rowOffset+c+2];
                Ixy1[rowOffset+c] = Ix1[rowOffset+c] * Iy1[rowOffset+c];
            }
        }

}


void derivative(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
		   TYPE* derH, TYPE* derV, TYPE* Ix, TYPE* Iy, TYPE* Ixy){

#pragma ss config
    {
	TYPE r1;
	#pragma ss stream
	for(int r=1; r<srcRows-1; r++) {
	    //#pragma ss dfg dedicated unroll (U)
            for(int c=0; c<srcCols; c++) {
                r1 = r*srcStep;
                TYPE a1 = src[r1-srcStep+c];
                TYPE a2 = src[r1+c];
                TYPE a3 = src[r1+srcStep+c];

                derV[r1-srcStep+c] = a1 + a2 + a2 + a3;
            }
        }

	#pragma ss stream
        for(int r=0; r<srcRows; r++) {
	    //#pragma ss dfg dedicated unroll (U)
            for(int c=1; c<srcCols-1; c++) {
                int rowOffset = r*srcStep;
                TYPE a1 = src[rowOffset + c-1];
                TYPE a2 = src[rowOffset + c];
                TYPE a3 = src[rowOffset + c+1];

                derH[rowOffset+c-1] = a1 + a2 + a2 + a3;
            }
        }

	#pragma ss stream
        for(int r=0; r<srcRows-2; r++) {
	    //#pragma ss dfg dedicated unroll (U)
            for(int c=0; c<srcCols-2; c++) {
                int rowOffset = r*srcStep;
                Ix[rowOffset+c] = derH[rowOffset+c] - derH[rowOffset+2*srcStep+c];
                Iy[rowOffset+c] = - derV[rowOffset+c] + derV[rowOffset+c+2];
                Ixy[rowOffset+c] = Ix[rowOffset+c] * Iy[rowOffset+c];
            }
        }
    }
}

TYPE Ix[N];
TYPE Iy[N];
TYPE Ixy[N];

TYPE Ix1[N];
TYPE Iy1[N];
TYPE Ixy1[N];

TYPE derH[N];
TYPE derV[N];

TYPE derH1[N];
TYPE derV1[N];

int main() {
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      Ix1[i*col_size + j] = 0;
      Iy1[i*col_size + j] = 0;
      Ixy1[i*col_size + j] = 0;
    }
  }
  derivativeCpu(image, row_size, col_size, col_size, C, derH1, derV1, Ix1, Iy1, Ixy1);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
  	derivative(image, row_size, col_size, col_size, C, derH, derV, Ix, Iy, Ixy);
        //derivative(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      Ix[i*col_size + j] = 0;
      Iy[i*col_size + j] = 0;
      Ixy[i*col_size + j] = 0;
    }
  }

  begin_roi();
  derivative(image, row_size, col_size, col_size, C, derH, derV, Ix, Iy, Ixy);
  //derivative(image, dest, ALPHA, temp, temp1);
  end_roi();
  sb_stats();

#if DEBUG
  double err_cnt = 0;
  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      if(Ixy[i*col_size + j] != Ixy1[i*col_size + j]){
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
