/*
Implementation based on algorithm described in:
The cache performance and optimizations of blocked algorithms
M. D. Lam, E. E. Rothberg, and M. E. Wolf
ASPLOS 1991
*/

#include "sim_timing.h"
#include "assert.h"
#include<climits>
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


void minMaxLocCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
		 double &minVal, double &maxVal, int &minXLoc, int &minYLoc, int &maxXLoc, int &maxYLoc){

	for(int i=0; i<srcRows; ++i){
            for(int j=0; j<srcCols; ++j){
               double t = src[i*srcStep + j*srcChannels];
               if(t<minVal){
                   minVal = t;
                   minXLoc = i;
                   minYLoc = j;
               }
               if(t>maxVal){
                   maxVal = t;
                   maxXLoc = i;
                   maxYLoc = j;
               }
            }
        }
}


void minMaxLoc(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
		 double &minValDSA, double &maxValDSA, int &minXLocDSA, int &minYLocDSA, int &maxXLocDSA, int &maxYLocDSA){

#pragma ss config
    {
	double t;
	#pragma ss stream
	for(int i=0; i<srcRows; ++i){
	    //#pragma ss dfg dedicated unroll (U) 
            for(int j=0; j<srcCols; ++j){
               t = src[i*srcStep + j*srcChannels];
               if(t<minValDSA){
                   minValDSA = t;
                   minXLocDSA = i;
                   minYLocDSA = j;
               }
               if(t>maxValDSA){
                   maxValDSA = t;
                   maxXLocDSA = i;
                   maxYLocDSA = j;
               }
            }
        }
    }
}


int main() {
  double minVal = INT_MAX, maxVal = INT_MIN;
  int minXLoc;
  int minYLoc;
  int maxXLoc;
  int maxYLoc;
  double minValDSA = INT_MAX, maxValDSA = INT_MIN;
  int minXLocDSA;
  int minYLocDSA;
  int maxXLocDSA;
  int maxYLocDSA;
  minMaxLocCpu(image, row_size, col_size, col_size, C, minVal, maxVal, minXLoc, minYLoc, maxXLoc, maxYLoc);

  //cache warmup
  for(int i=0;i<CACHE_WARMUP;++i)
        minMaxLoc(image, row_size, col_size, col_size, C, minValDSA, maxValDSA, minXLocDSA, minYLocDSA, maxXLocDSA, maxYLocDSA);
        //minMaxLoc(image, dest, ALPHA, temp, temp1);


  begin_roi();
  minMaxLoc(image, row_size, col_size, col_size, C, minValDSA, maxValDSA, minXLocDSA, minYLocDSA, maxXLocDSA, maxYLocDSA);
  //minMaxLoc(image, dest, ALPHA, temp, temp1);
  end_roi();
  sb_stats();

#if DEBUG
  /////// Kernel output ////////
  std::cout << "DSA-Minvalue = " << minValDSA << std::endl;
  std::cout << "DSA-Maxvalue = " << maxValDSA << std::endl;
  //std::cout << "DSA-Min Location.x = " << minXLocDSA << "  DSA-Min Location.y = " << minYLocDSA << std::endl;
  //std::cout << "DSA-Max Location.x = " << maxXLocDSA << "  DSA-Max Location.y = " << maxYLocDSA << std::endl;

  /////// printing the difference in min and max, values and locations of both OpenCV and Kernel function /////
  printf("Difference in Minimum value: %lf \n", (minVal - minValDSA));
  printf("Difference in Maximum value: %lf \n", (maxVal - maxValDSA));
  printf("Difference in Minimum value location: (%d,%d) \n", (minYLoc - minYLocDSA), (minXLoc - minXLocDSA));
  printf("Difference in Maximum value location: (%d,%d) \n", (maxYLoc - maxYLocDSA), (maxXLoc - maxXLocDSA));

  if (((minYLoc - minYLocDSA) > 1) | ((minXLoc - minYLocDSA) > 1)) {
      fprintf(stderr, "ERROR: Test Failed.\n ");
      return -1;
  }

#endif
  return 0;
}
