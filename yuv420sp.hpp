/*
 * =====================================================================================
 *
 *       Filename:  yuv420sp.hpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2017年06月20日 15时00分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuai YUAN()
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef YUV420SP_HPP
#define YUV420SP_HPP 

#include "enum.hpp"

void RGB2YUV420SP(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int width, const int height, const ColorConversionCodes code, const bool useNEON);

#endif /* YUV420SP_HPP */
