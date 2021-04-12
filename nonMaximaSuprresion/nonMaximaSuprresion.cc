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
#include<vector>
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
typedef struct { int x, y; } xy;
typedef struct { float cornerResponse; int x, y; } cornerXY;
static bool by_cornerResponse(cornerXY const &left, cornerXY const &right) {
    return left.cornerResponse > right.cornerResponse;
}

void nonMaximaSuprresionCpu(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels, 
		    cornerXY* points, float percentage, int filterRange, int suppressionRadius){

        for (int r = 0; r < srcRows; r++) {
            for (int c = 0; c < srcCols; c++) {

                int index = r*srcStep + c;
                cornerXY d;
                d.cornerResponse = src[index];
                d.x = r;
                d.y = c;

                points[index]=d;
            }
        }

        // // Sort points by corner Response
	//std::sort(points, points+N, by_cornerResponse);

        //// Get top points, given by the percentage
        //int numberTopPoints = srcCols * srcRows * percentage;
        //std::vector<cornerXY> topPoints;

        //int i=0;
        //while(topPoints.size() < numberTopPoints) {
        //    if(i == points.size())
        //        break;

        //    int supRows = destRows;
        //    int supCols = destCols;

        //    // Check if point marked in maximaSuppression matrix
        //    if(dest[(points[i].x)*destStep + points[i].y] == 0) {
        //        for (int r = -suppressionRadius; r <= suppressionRadius; r++) {
        //            for (int c = -suppressionRadius; c <= suppressionRadius; c++) {
        //                int sx = points[i].x+c;
        //                int sy = points[i].y+r;

        //                // bound checking
        //                if(sx > supRows)
        //                    sx = supRows;
        //                if(sx < 0)
        //                    sx = 0;
        //                if(sy > supCols)
        //                    sy = supCols;
        //                if(sy < 0)
        //                    sy = 0;

        //                int index = (points[i].x+r)*destStep + points[i].y+c;
        //                if(index>=0 && ((points[i].x+r)*destStep)>=0 && (points[i].y+c)>=0
        //                    && ((points[i].x+r)*destStep)<destRows && (points[i].y+c)<destCols) dest[index] = 1;
        //            }
        //        }

        //        // Convert back to original image coordinate system
        //        points[i].x += 1 + filterRange;
        //        points[i].y += 1 + filterRange;
        //        topPoints.push_back(points[i]);
        //    }

        //    i++;
        //}
}


//void nonMaximaSuprresion(const TYPE* src, int srcRows, int srcCols, int srcStep, int srcChannels,
//                    TYPE* dest, int destRows, int destCols, int destStep, int destChannels,
//		    std::vector<cornerXY> &points, float percentage, int filterRange, int suppressionRadius){
//
//#pragma ss config
//    {
//        for (int r = 0; r < srcRows; r++) {
//            for (int c = 0; c < srcCols; c++) {
//
//                int index = r*srcStep + c;
//                cornerXY d;
//                d.cornerResponse = src[index];
//                d.x = r;
//                d.y = c;
//
//                points.push_back(d);
//            }
//        }
//
//        // // Sort points by corner Response
//        sort(points.begin(), points.end(), by_cornerResponse());
//
//        // Get top points, given by the percentage
//        int numberTopPoints = srcCols * srcRows * percentage;
//        std::vector<cornerXY> topPoints;
//
//        int i=0;
//        while(topPoints.size() < numberTopPoints) {
//            if(i == points.size())
//                break;
//
//            int supRows = destRows;
//            int supCols = destCols;
//
//            // Check if point marked in maximaSuppression matrix
//            if(dest[(points[i].x)*destStep + points[i].y] == 0) {
//                for (int r = -suppressionRadius; r <= suppressionRadius; r++) {
//                    for (int c = -suppressionRadius; c <= suppressionRadius; c++) {
//                        int sx = points[i].x+c;
//                        int sy = points[i].y+r;
//
//                        // bound checking
//                        if(sx > supRows)
//                            sx = supRows;
//                        if(sx < 0)
//                            sx = 0;
//                        if(sy > supCols)
//                            sy = supCols;
//                        if(sy < 0)
//                            sy = 0;
//
//                        int index = (points[i].x+r)*destStep + points[i].y+c;
//                        if(index>=0 && ((points[i].x+r)*destStep)>=0 && (points[i].y+c)>=0
//                            && ((points[i].x+r)*destStep)<destRows && (points[i].y+c)<destCols) dest[index] = 1;
//                    }
//                }
//
//                // Convert back to original image coordinate system
//                points[i].x += 1 + filterRange;
//                points[i].y += 1 + filterRange;
//                topPoints.push_back(points[i]);
//            }
//
//            i++;
//        }
//    }
//}

TYPE dest[N];
TYPE dest1[N];
cornerXY points[N];
cornerXY pointsDSA[N];

int main() {
  
  float percentage=1;
  int filterRange=3, suppressionRadius=10;


  nonMaximaSuprresionCpu(image, row_size, col_size, col_size, C, dest1, row_size, col_size, col_size, C, points, percentage, filterRange, suppressionRadius);

//  //cache warmup
//  for(int i=0;i<CACHE_WARMUP;++i)
//        nonMaximaSuprresion(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, pointsDSA, percentage, filterRange, suppressionRadius);
//        //nonMaximaSuprresion(image, dest, ALPHA, temp, temp1);
//
//
//  for(int i=0; i<row_size; ++i){
//    for(int j=0; j<col_size; ++j){
//      dest[i*col_size + j] = 0;
//      //dest1[i*col_size + j] = 0;
//    }
//  }
//
//  begin_roi();
//  nonMaximaSuprresion(image, row_size, col_size, col_size, C, dest, row_size, col_size, col_size, C, pointsDSA, percentage, filterRange, suppressionRadius);
//  //nonMaximaSuprresion(image, dest, ALPHA, temp, temp1);
//  end_roi();
//  sb_stats();
//
//#if DEBUG
//  
//  double err_cnt = 0;
//  for(int i=0; i<row_size; ++i){
//    for(int j=0; j<col_size; ++j){
//      if(dest[i*col_size + j] != dest1[i*col_size + j]){
//              err_cnt++;
//              //std::cout << dest[i*col_size + j] << ":" << dest1[i*col_size + j] << std::endl;
//      }
//      //      std::cout << dest[i*col_size + j] << std::endl;
//    }
//  }
//  
//  std::cout << "ERROR percent = " << err_cnt/(row_size*col_size*C) << std::endl;
//  std::cout << "ERROR count = " << err_cnt << " out of " << row_size*col_size*C << std::endl;
//#endif
  return 0;
}
