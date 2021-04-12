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
#define CACHE_WARMUP 2
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
#define NGray row_size*col_size
#define CGray 1
#define NN N*N
#define block_size 8
#define NUMOFBLOCKS N/block_size/block_size

#ifndef U
#define U 8 
#endif

#define DEBUG 1
void lutCpu(const TYPE* image, int imageRows, int imageCols, int imageStep, int imageChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, TYPE correctionLut[256]){

        for(int i=0; i<imageRows; ++i){
            for(int j=0; j<imageCols; ++j){
		unsigned char t =  (unsigned char)image[i*imageStep + j*imageChannels ];
                dest[i*destStep + j*imageChannels ] = correctionLut[t];
                                                           //just for CV_8U
            }
        }
}


void lut(const TYPE* __restrict image, int imageRows, int imageCols, int imageStep, int imageChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels){

#pragma ss config
    {
  TYPE lutMatrix[256] = {
    0,   16,  23,  28,  32,  36,  39,  42,  45,  48,  50,  53,  55,  58,  60,  62,  64,  66,  68,  70,  71,  73,
    75,  77,  78,  80,  81,  83,  84,  86,  87,  89,  90,  92,  93,  94,  96,  97,  98,  100, 101, 102, 103, 105,
    106, 107, 108, 109, 111, 112, 113, 114, 115, 116, 117, 118, 119, 121, 122, 123, 124, 125, 126, 127, 128, 129,
    130, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 145, 146, 147, 148, 149,
    150, 151, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167,
    167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 181, 182, 183,
    183, 184, 185, 186, 186, 187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194, 195, 196, 196, 197, 198,
    198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211,
    212, 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 220, 220, 221, 221, 222, 222, 223, 224, 224,
    225, 225, 226, 226, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232, 233, 233, 234, 234, 235, 235, 236, 236,
    237, 237, 238, 238, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248,
    248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255};
	#pragma ss stream 
            for(int i=0; i<imageRows; ++i){
		//#pragma ss dfg dedicated
                for(int j=0; j<imageCols; ++j){
		    unsigned char t =  (unsigned char)image[i*imageStep + j*imageChannels ];
                    dest[i*destStep + j*imageChannels ] = lutMatrix[t];
                }
            }
    }
}

TYPE dest[N];
TYPE dest1[N];
  TYPE lutMat[256] = {
    0,   16,  23,  28,  32,  36,  39,  42,  45,  48,  50,  53,  55,  58,  60,  62,  64,  66,  68,  70,  71,  73,
    75,  77,  78,  80,  81,  83,  84,  86,  87,  89,  90,  92,  93,  94,  96,  97,  98,  100, 101, 102, 103, 105,
    106, 107, 108, 109, 111, 112, 113, 114, 115, 116, 117, 118, 119, 121, 122, 123, 124, 125, 126, 127, 128, 129,
    130, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 145, 146, 147, 148, 149,
    150, 151, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167,
    167, 168, 169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 181, 182, 183,
    183, 184, 185, 186, 186, 187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194, 195, 196, 196, 197, 198,
    198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211,
    212, 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 220, 220, 221, 221, 222, 222, 223, 224, 224,
    225, 225, 226, 226, 227, 228, 228, 229, 229, 230, 230, 231, 231, 232, 233, 233, 234, 234, 235, 235, 236, 236,
    237, 237, 238, 238, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248,
    248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255};

int main() {

  
  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      //dest[i*col_size + j] = 0;
      dest1[i*col_size + j] = 0;
    }
  }
  lutCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, lutMat);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
  	lut(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
        //lut(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  lut(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C);
  //lut(image, dest, ALPHA, temp, temp1);
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
  std::cout << "ERROR count = " << err_cnt << " out of " << row_size*col_size*C<< std::endl;
#endif

  return 0;
}
