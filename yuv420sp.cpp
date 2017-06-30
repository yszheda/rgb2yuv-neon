/*
 * =====================================================================================
 *
 *       Filename:  yuv420sp.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2017年06月20日 10时46分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuai YUAN (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <arm_neon.h>
#include <cassert>

#include "yuv420sp.hpp"


enum class Strategies {
    SERIAL              = 0,
    NEON_INTRINSICS     = 1,
};

namespace YUV420SP {

    bool is_nv21(ColorConversionCodes code)
    {
        return (code == ColorConversionCodes::RGB2YUV420_NV21)
            || (code == ColorConversionCodes::RGBA2YUV420_NV21);
    }


    bool is_nv12(ColorConversionCodes code)
    {
        return (code == ColorConversionCodes::RGB2YUV420_NV12)
            || (code == ColorConversionCodes::RGBA2YUV420_NV12);
    }


    bool is_valid_conversion(ColorConversionCodes code)
    {
        return is_nv21(code) || is_nv12(code);
    }


    int uv_order(ColorConversionCodes code)
    {
        if (is_nv21(code)) {
            return 1;
        } else {
            return 0;
        }
    }


    template <Strategies strategy>
        void RGB2YUV420SP_impl(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int width, const int height, const int rgbChannelNum, const int uvOrder)
        {
        }

    template <>
        void RGB2YUV420SP_impl<Strategies::SERIAL>(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int width, const int height, const int rgbChannelNum, const int uvOrder)
        {
            int frameSize = width * height;
            int yIndex = 0;
            int uvIndex = frameSize;

            int i, j;
            for (j = 0; j < height; j++) {
                for (i = 0; i < width; i++) {
                    int r = rgb[(j * width + i) * rgbChannelNum + 2];
                    int g = rgb[(j * width + i) * rgbChannelNum + 1];
                    int b = rgb[(j * width + i) * rgbChannelNum + 0];

                    int y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
                    int u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
                    int v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

                    yuv[yIndex++] = y;
                    if (j % 2 == 0 && i % 2 == 0) {
                        yuv[uvIndex + 1 + uvOrder] = u;
                        yuv[uvIndex + 2 - uvOrder] = v;
                        uvIndex += 2;
                    }
                }
            }
        }

    template <>
        void RGB2YUV420SP_impl<Strategies::NEON_INTRINSICS>(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, int width, int height, const int rgbChannelNum, const int uvOrder)
        {
            const uint16x8_t u16_rounding = vdupq_n_u16(128);
            const int16x8_t s16_rounding = vdupq_n_s16(128);
            const int8x8_t s8_rounding = vdup_n_s8(128);
            const uint8x16_t offset = vdupq_n_u8(16);
            const uint16x8_t mask = vdupq_n_s16(255);

            int frameSize = width * height;

            int yIndex = 0;
            int uvIndex = frameSize;

            int i;
            int j;
            for (j = 0; j < height; j++) {
                for (i = 0; i < width >> 4; i++) {
                    // Load rgb
                    uint8x16x3_t pixel_rgb;
                    if (rgbChannelNum == 3) {
                        pixel_rgb = vld3q_u8(rgb);
                    } else {
                        uint8x16x4_t pixel_argb = vld4q_u8(rgb);
                        pixel_rgb.val[0] = pixel_argb.val[0];
                        pixel_rgb.val[1] = pixel_argb.val[1];
                        pixel_rgb.val[2] = pixel_argb.val[2];
                    }
                    rgb += rgbChannelNum * 16;

                    uint8x8x2_t uint8_r;
                    uint8x8x2_t uint8_g;
                    uint8x8x2_t uint8_b;
                    uint8_r.val[0] = vget_low_u8(pixel_rgb.val[2]);
                    uint8_r.val[1] = vget_high_u8(pixel_rgb.val[2]);
                    uint8_g.val[0] = vget_low_u8(pixel_rgb.val[1]);
                    uint8_g.val[1] = vget_high_u8(pixel_rgb.val[1]);
                    uint8_b.val[0] = vget_low_u8(pixel_rgb.val[0]);
                    uint8_b.val[1] = vget_high_u8(pixel_rgb.val[0]);

                    uint16x8x2_t uint16_y;
                    uint8x8_t scalar = vdup_n_u8(66);
                    uint8x16_t y;

                    uint16_y.val[0] = vmull_u8(uint8_r.val[0], scalar);
                    uint16_y.val[1] = vmull_u8(uint8_r.val[1], scalar);
                    scalar = vdup_n_u8(129);
                    uint16_y.val[0] = vmlal_u8(uint16_y.val[0], uint8_g.val[0], scalar);
                    uint16_y.val[1] = vmlal_u8(uint16_y.val[1], uint8_g.val[1], scalar);
                    scalar = vdup_n_u8(25);
                    uint16_y.val[0] = vmlal_u8(uint16_y.val[0], uint8_b.val[0], scalar);
                    uint16_y.val[1] = vmlal_u8(uint16_y.val[1], uint8_b.val[1], scalar);

                    uint16_y.val[0] = vaddq_u16(uint16_y.val[0], u16_rounding);
                    uint16_y.val[1] = vaddq_u16(uint16_y.val[1], u16_rounding);

                    y = vcombine_u8(vqshrn_n_u16(uint16_y.val[0], 8), vqshrn_n_u16(uint16_y.val[1], 8));
                    y = vaddq_u8(y, offset);

                    vst1q_u8(yuv + yIndex, y);
                    yIndex += 16;

                    // Compute u and v in the even row
                    if (j % 2 == 0) {
                        int16x8_t u_scalar = vdupq_n_s16(-38);
                        int16x8_t v_scalar = vdupq_n_s16(112);

                        int16x8_t r = vreinterpretq_s16_u16(vandq_u16(vreinterpretq_u16_u8(pixel_rgb.val[2]), mask));
                        int16x8_t g = vreinterpretq_s16_u16(vandq_u16(vreinterpretq_u16_u8(pixel_rgb.val[1]), mask));
                        int16x8_t b = vreinterpretq_s16_u16(vandq_u16(vreinterpretq_u16_u8(pixel_rgb.val[0]), mask));

                        int16x8_t u;
                        int16x8_t v;
                        uint8x8x2_t uv;

                        u = vmulq_s16(r, u_scalar);
                        v = vmulq_s16(r, v_scalar);

                        u_scalar = vdupq_n_s16(-74);
                        v_scalar = vdupq_n_s16(-94);
                        u = vmlaq_s16(u, g, u_scalar);
                        v = vmlaq_s16(v, g, v_scalar);

                        u_scalar = vdupq_n_s16(112);
                        v_scalar = vdupq_n_s16(-18);
                        u = vmlaq_s16(u, b, u_scalar);
                        v = vmlaq_s16(v, b, v_scalar);

                        u = vaddq_s16(u, s16_rounding);
                        v = vaddq_s16(v, s16_rounding);

                        uv.val[0 + uvOrder] = vreinterpret_u8_s8(vadd_s8(vqshrn_n_s16(u, 8), s8_rounding));
                        uv.val[1 - uvOrder] = vreinterpret_u8_s8(vadd_s8(vqshrn_n_s16(v, 8), s8_rounding));

                        vst2_u8(yuv + uvIndex, uv);

                        uvIndex += 2 * 8;
                    }
                }

                // Handle leftovers
                for (i = ((width >> 4) << 4); i < width; i++) {
                    uint8_t r = rgb[2];
                    uint8_t g = rgb[1];
                    uint8_t b = rgb[0];

                    rgb += rgbChannelNum;

                    uint8_t y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
                    uint8_t u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
                    uint8_t v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

                    yuv[yIndex++] = y;
                    if (j % 2 == 0 && i % 2 == 0) {
                        yuv[uvIndex + 1 + uvOrder] = u;
                        yuv[uvIndex + 2 - uvOrder] = v;
                        uvIndex += 2;
                    }
                }
            }
        }


    void convert(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int width, const int height, const ColorConversionCodes code, const bool useNEON)
    {
        assert(is_valid_conversion(code));

        int rgbChannelNum = channel_num(code);
        int uvOrder = uv_order(code);

        if (useNEON) {
            RGB2YUV420SP_impl<Strategies::NEON_INTRINSICS>(yuv, rgb, width, height, rgbChannelNum, uvOrder);
        } else {
            RGB2YUV420SP_impl<Strategies::SERIAL>(yuv, rgb, width, height, rgbChannelNum, uvOrder);
        }
    }

}
