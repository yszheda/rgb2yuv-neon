/*
 * =====================================================================================
 *
 *       Filename:  yuv444.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年06月23日 21时56分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuai YUAN (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef YUV444_HPP
#define YUV444_HPP 

#include "enum.hpp"

void RGB2YUV444(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int width, const int height, const ColorConversionCodes code, const bool useNEON);

#endif /* YUV444_HPP */
