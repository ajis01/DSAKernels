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

typedef enum{
	REDUCE_SUM,
	REDUCE_AVG,
	REDUCE_MAX,
	REDUCE_MIN
}reductionType_t;

void reduceCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    double* dest, int destRows, int destCols, int destStep, int destChannels, TYPE dimension, reductionType_t reduction){

	TYPE  internal_res;
        TYPE * line_buf = (TYPE*) malloc(srcCols * sizeof(TYPE));
        unsigned long long int p = 0, q = 0;
        TYPE max = 0;
        TYPE val_src, val_dst;

        if (dimension == 0) {
            for (int i = 0; i < (srcCols); i++) {
                line_buf[i] = src[i];
            }
        }

        for(int i=0; i<srcRows; ++i){

            if (reduction == REDUCE_MIN) {
                internal_res = -1;
                max = 255;
            } else {
                internal_res = 0;
                max = 0;
            }

            for(int j=0; j<srcCols; ++j){
                val_src = src[i*srcStep + j*srcChannels];

                if (dimension == 0) {
                    internal_res = line_buf[j];
                }

                switch (reduction) {
                    case REDUCE_SUM:
                        internal_res = internal_res + val_src;
                        break;
                    case REDUCE_AVG:
                        internal_res = internal_res + val_src;
                        break;
                    case REDUCE_MAX:
                        internal_res =
                            internal_res > val_src ? internal_res : val_src;
                        break;
                    case REDUCE_MIN:
                        internal_res =
                            internal_res < val_src ? internal_res : val_src;
                        break;
                }
                if (dimension == 1 && j == srcCols - 1) {
                    if (reduction == REDUCE_AVG) {
                        val_dst = internal_res / srcCols;
                    } else {
                        val_dst = internal_res;
                    }
                }
                if (dimension == 0) {
                    val_dst = internal_res;
                    line_buf[j] = val_dst;
                }
            }//end of j=jj
            if (dimension == 1) {
                dest[q] =  val_dst;
                q++;
            }
        }


        if (dimension == 0) {
            for (unsigned int out = 0; out < srcCols; out++) {
                if (reduction == REDUCE_SUM) {
                    dest[q] =  line_buf[out];
                    q++;
                } else if (reduction == REDUCE_AVG) {
                    dest[q] =  line_buf[out] / srcRows;
                    q++;
                } else {
                    dest[q] =  line_buf[out];
                    q = q + 1;
                }
            }
        }

        free(line_buf);
}




void reduce(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    double* dest, int destRows, int destCols, int destStep, int destChannels, TYPE dimension, reductionType_t reduction){
#pragma ss config
    {
	TYPE  internal_res;
        TYPE * line_buf = (TYPE*) malloc(srcCols * sizeof(TYPE));
        unsigned long long int p = 0, q = 0;
        TYPE max = 0;
        TYPE val_src, val_dst;

        if (dimension == 0) {
            for (int i = 0; i < (srcCols); i++) {
                line_buf[i] = src[i];
            }
        }

        #pragma ss stream
        for(int i=0; i<srcRows; ++i){

            if (reduction == REDUCE_MIN) {
                internal_res = -1;
                max = 255;
            } else {
                internal_res = 0;
                max = 0;
            }

            for(int j=0; j<srcCols; ++j){
                val_src = src[i*srcStep + j*srcChannels];

                if (dimension == 0) {
                    internal_res = line_buf[j];
                }

                switch (reduction) {
                    case REDUCE_SUM:
                        internal_res = internal_res + val_src;
                        break;
                    case REDUCE_AVG:
                        internal_res = internal_res + val_src;
                        break;
                    case REDUCE_MAX:
                        internal_res =
                            internal_res > val_src ? internal_res : val_src;
                        break;
                    case REDUCE_MIN:
                        internal_res =
                            internal_res < val_src ? internal_res : val_src;
                        break;
                }
                if (dimension == 1 && j == srcCols - 1) {
                    if (reduction == REDUCE_AVG) {
                        val_dst = internal_res / srcCols;
                    } else {
                        val_dst = internal_res;
                    }
                }
                if (dimension == 0) {
                    val_dst = internal_res;
                    line_buf[j] = val_dst;
                }
            }//end of j=jj
            if (dimension == 1) {
                dest[q] =  val_dst;
                q++;
            }
        }


        if (dimension == 0) {
            #pragma ss stream 
            for (unsigned int out = 0; out < srcCols; out++) {
                if (reduction == REDUCE_SUM) {
                    dest[q] =  line_buf[out];
                    q++;
                } else if (reduction == REDUCE_AVG) {
                    dest[q] =  line_buf[out] / srcRows;
                    q++;
                } else {
                    dest[q] =  line_buf[out];
                    q = q + 1;
                }
            }
        }

        free(line_buf);
    }
}

double dest[N];
double dest1[N];

int main() {
  
  TYPE dimension = 1;
  reductionType_t reduction = REDUCE_AVG;
  reduceCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, dimension, reduction);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        reduce(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, dimension, reduction);
        //reduce(image, dest, ALPHA, temp, temp1);


  for(int i=0; i<row_size; ++i){
    for(int j=0; j<col_size; ++j){
      dest[i*col_size + j] = 0;
      //dest1[i*col_size + j] = 0;
    }
  }

  begin_roi();
  reduce(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, dimension, reduction);
  //reduce(image, dest, ALPHA, temp, temp1);
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
