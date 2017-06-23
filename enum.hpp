/*
 * =====================================================================================
 *
 *       Filename:  enum.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017年06月20日 10时29分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuai YUAN (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#ifndef ENUM_H

enum class ColorConversionCodes {
    RGB2YUV444             = 0,
    RGBA2YUV444            = 1,
    // YUV420SP
    RGB2YUV420_NV21        = 2,
    RGBA2YUV420_NV21       = 3,
    RGB2YUV420_NV12        = 4,
    RGBA2YUV420_NV12       = 5,
    NUM                    = 6,
};

inline bool has_alpha_channel(ColorConversionCodes code) { return static_cast<int>(code) % 2 == 1; }

inline bool channel_num(ColorConversionCodes code)
{
    if (has_alpha_channel(code)) {
        return 4;
    } else {
        return 3;
    }
}

inline double yuv_size_factor(ColorConversionCodes code)
{
    if (static_cast<int>(code) < 2) {
        return 3;
    } else {
        return 1.5;
    }
}

#define ENUM_H 
#endif /* ENUM_H */
