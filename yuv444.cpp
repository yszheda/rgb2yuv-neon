/*
 * =====================================================================================
 *
 *       Filename:  yuv444.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2017年06月20日 10时40分08秒
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

#include "yuv444.hpp"


enum class Strategies {
    SERIAL_FP           = 0,
    SERIAL_INT          = 1,
    NEON_INTRINSICS     = 2,
    NEON_ASSEMBLY1      = 3,
    NEON_ASSEMBLY2      = 4,
};


template <Strategies strategy>
void RGB2YUV444_impl(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int pixel_num, const int rgbChannelNum)
{
}

template <>
void RGB2YUV444_impl<Strategies::SERIAL_FP>(unsigned char *yuv, unsigned char *rgb, const int pixel_num, const int rgbChannelNum)
{
    int i;
    for (i = 0; i < pixel_num; ++i) {
        uint8_t r = rgb[i * rgbChannelNum + 2];
        uint8_t g = rgb[i * rgbChannelNum + 1];
        uint8_t b = rgb[i * rgbChannelNum + 0];
        uint8_t y = 0.299 * r + 0.587 * g + 0.114 * b;
        // Way 1
        // float u = 0.492 * (b - y);
        // float v = 0.877 * (r - y);
        // Way 2
        // float u = -0.14713 * r - 0.28886 * g + 0.436 * b;
        // float v = 0.615 * r - 0.51499 * g - 0.10001 * b;
        // Way 3
        // https://zh.wikipedia.org/wiki/YUV
        uint8_t u = -0.169 * r - 0.331 * g + 0.5 * b + 128;
        uint8_t v = 0.5 * r - 0.419 * g - 0.081 * b + 128;

        yuv[i * 3] = y;
        yuv[i * 3 + 1] = u;
        yuv[i * 3 + 2] = v;
    }
}

// https://en.wikipedia.org/wiki/YUV
// Full swing for BT.601
template <>
void RGB2YUV444_impl<Strategies::SERIAL_INT>(unsigned char *yuv, unsigned char *rgb, const int pixel_num, const int rgbChannelNum)
{
    int i;
    for (i = 0; i < pixel_num; ++i) {
        uint8_t r = rgb[i * rgbChannelNum + 2];
        uint8_t g = rgb[i * rgbChannelNum + 1];
        uint8_t b = rgb[i * rgbChannelNum + 0];

        uint16_t y_tmp = 76 * r + 150 * g + 29 * b;
        int16_t u_tmp = -43 * r - 84 * g + 127 * b;
        int16_t v_tmp = 127 * r - 106 * g - 21 * b;

        y_tmp = (y_tmp + 128) >> 8;
        u_tmp = (u_tmp + 128) >> 8;
        v_tmp = (v_tmp + 128) >> 8;

        yuv[i * 3] = (uint8_t) y_tmp;
        yuv[i * 3 + 1] = (uint8_t) (u_tmp + 128);
        yuv[i * 3 + 2] = (uint8_t) (v_tmp + 128);
    }
}

template <>
void RGB2YUV444_impl<Strategies::NEON_INTRINSICS>(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int pixel_num, const int rgbChannelNum)
{
    const uint8x8_t u8_zero = vdup_n_u8(0);
    const uint16x8_t u16_rounding = vdupq_n_u16(128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);
    const int8x16_t s8_rounding = vdupq_n_s8(128);

    int count = pixel_num / 16;

    int i;
    for (i = 0; i < count; ++i) {
        // Load rgb
        // TODO
        uint8x16x3_t pixel_rgb = vld3q_u8(rgb);

        uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
        uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
        uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
        uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
        uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
        uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);
        int16x8_t signed_high_r = vreinterpretq_s16_u16(vaddl_u8(high_r, u8_zero));
        int16x8_t signed_low_r = vreinterpretq_s16_u16(vaddl_u8(low_r, u8_zero));
        int16x8_t signed_high_g = vreinterpretq_s16_u16(vaddl_u8(high_g, u8_zero));
        int16x8_t signed_low_g = vreinterpretq_s16_u16(vaddl_u8(low_g, u8_zero));
        int16x8_t signed_high_b = vreinterpretq_s16_u16(vaddl_u8(high_b, u8_zero));
        int16x8_t signed_low_b = vreinterpretq_s16_u16(vaddl_u8(low_b, u8_zero));

        // NOTE:
        // declaration may not appear after executable statement in block
        uint16x8_t high_y;
        uint16x8_t low_y;
        uint8x8_t scalar = vdup_n_u8(76);
        int16x8_t high_u;
        int16x8_t low_u;
        int16x8_t signed_scalar = vdupq_n_s16(-43);
        int16x8_t high_v;
        int16x8_t low_v;
        uint8x16x3_t pixel_yuv;
        int8x16_t u;
        int8x16_t v;

        // 1. Multiply transform matrix (Y′: unsigned, U/V: signed)
        high_y = vmull_u8(high_r, scalar);
        low_y = vmull_u8(low_r, scalar);

        high_u = vmulq_s16(signed_high_r, signed_scalar);
        low_u = vmulq_s16(signed_low_r, signed_scalar);

        signed_scalar = vdupq_n_s16(127);
        high_v = vmulq_s16(signed_high_r, signed_scalar);
        low_v = vmulq_s16(signed_low_r, signed_scalar);

        scalar = vdup_n_u8(150);
        high_y = vmlal_u8(high_y, high_g, scalar);
        low_y = vmlal_u8(low_y, low_g, scalar);

        signed_scalar = vdupq_n_s16(-84);
        high_u = vmlaq_s16(high_u, signed_high_g, signed_scalar);
        low_u = vmlaq_s16(low_u, signed_low_g, signed_scalar);

        signed_scalar = vdupq_n_s16(-106);
        high_v = vmlaq_s16(high_v, signed_high_g, signed_scalar);
        low_v = vmlaq_s16(low_v, signed_low_g, signed_scalar);

        scalar = vdup_n_u8(29);
        high_y = vmlal_u8(high_y, high_b, scalar);
        low_y = vmlal_u8(low_y, low_b, scalar);

        signed_scalar = vdupq_n_s16(127);
        high_u = vmlaq_s16(high_u, signed_high_b, signed_scalar);
        low_u = vmlaq_s16(low_u, signed_low_b, signed_scalar);

        signed_scalar = vdupq_n_s16(-21);
        high_v = vmlaq_s16(high_v, signed_high_b, signed_scalar);
        low_v = vmlaq_s16(low_v, signed_low_b, signed_scalar);
        // 2. Scale down (">>8") to 8-bit values with rounding ("+128") (Y′: unsigned, U/V: signed)
        // 3. Add an offset to the values to eliminate any negative values (all results are 8-bit unsigned)

        high_y = vaddq_u16(high_y, u16_rounding);
        low_y = vaddq_u16(low_y, u16_rounding);

        high_u = vaddq_s16(high_u, s16_rounding);
        low_u = vaddq_s16(low_u, s16_rounding);

        high_v = vaddq_s16(high_v, s16_rounding);
        low_v = vaddq_s16(low_v, s16_rounding);

        pixel_yuv.val[0] = vcombine_u8(vqshrn_n_u16(low_y, 8), vqshrn_n_u16(high_y, 8));

        u = vcombine_s8(vqshrn_n_s16(low_u, 8), vqshrn_n_s16(high_u, 8));

        v = vcombine_s8(vqshrn_n_s16(low_v, 8), vqshrn_n_s16(high_v, 8));

        u = vaddq_s8(u, s8_rounding);
        pixel_yuv.val[1] = vreinterpretq_u8_s8(u);

        v = vaddq_s8(v, s8_rounding);
        pixel_yuv.val[2] = vreinterpretq_u8_s8(v);

        // Store
        vst3q_u8(yuv, pixel_yuv);

        rgb += 3 * 16;
        yuv += 3 * 16;
    }

    // Handle leftovers
    for (i = count * 16; i < pixel_num; ++i) {
        uint8_t r = rgb[i * 3];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

        uint16_t y_tmp = 76 * r + 150 * g + 29 * b;
        int16_t u_tmp = -43 * r - 84 * g + 127 * b;
        int16_t v_tmp = 127 * r - 106 * g - 21 * b;

        y_tmp = (y_tmp + 128) >> 8;
        u_tmp = (u_tmp + 128) >> 8;
        v_tmp = (v_tmp + 128) >> 8;

        yuv[i * 3] = (uint8_t) y_tmp;
        yuv[i * 3 + 1] = (uint8_t) (u_tmp + 128);
        yuv[i * 3 + 2] = (uint8_t) (v_tmp + 128);
    }
}

template <>
void RGB2YUV444_impl<Strategies::NEON_ASSEMBLY1>(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int pixel_num, const int rgbChannelNum)
{
    int count = pixel_num / 16;

    asm volatile(
            // const uint16x8_t u16_rounding = vdupq_n_u16(128);
            // const int16x8_t s16_rounding = vdupq_n_s16(128);
            "VMOV.I16 q3,#0x80\t\n"
            // const int8x16_t s8_rounding = vdupq_n_s8(128);
            "VMOV.I8  q4,#0x80\t\n"

            // i = 0
            "MOV      r2,#0\t\n"

            "LOOP: \t\n"

            // uint8x16x3_t pixel_rgb = vld3q_u8(rgb);
            "ADD      r12,r1,#0x18\t\n"
            "VLD3.8   {d0,d2,d4},[r1]\t\n"
            "VLD3.8   {d1,d3,d5},[r12]\t\n"

            // // rgb += 3 * 16;
            // "ADD      r1,r1,#0x30\t\n"

            // // i++
            // "ADD      r2,r2,#1\t\n"

            // int16x8_t signed_high_r = vreinterpretq_s16_u16(vmovl_u8(high_r));
            // int16x8_t signed_low_r = vreinterpretq_s16_u16(vmovl_u8(low_r));
            // int16x8_t signed_high_g = vreinterpretq_s16_u16(vmovl_u8(high_g));
            // int16x8_t signed_low_g = vreinterpretq_s16_u16(vmovl_u8(low_g));
            // int16x8_t signed_high_b = vreinterpretq_s16_u16(vmovl_u8(high_b));
            // int16x8_t signed_low_b = vreinterpretq_s16_u16(vmovl_u8(low_b));
            "VMOVL.U8 q5,d0\t\n"
            "VMOVL.U8 q6,d1\t\n"
            "VMOVL.U8 q7,d2\t\n"
            "VMOVL.U8 q8,d3\t\n"
            "VMOVL.U8 q9,d4\t\n"
            "VMOVL.U8 q10,d5\t\n"

            // uint8x8_t scalar = vdup_n_u8(76);
            // high_y = vmull_u8(high_r, scalar);
            // low_y = vmull_u8(low_r, scalar);
            "VMOV.I8  d26,#0x4c\t\n"
            "VMULL.U8 q11,d0,d26\t\n"
            "VMULL.U8 q12,d1,d26\t\n"

            // scalar = vdup_n_u8(150);
            // high_y = vmlal_u8(high_y, high_g, scalar);
            // low_y = vmlal_u8(low_y, low_g, scalar);
            "VMOV.I8  d26,#0x96\t\n"
            "VMLAL.U8 q11,d2,d26\t\n"
            "VMLAL.U8 q12,d3,d26\t\n"

            // scalar = vdup_n_u8(29);
            // high_y = vmlal_u8(high_y, high_b, scalar);
            // low_y = vmlal_u8(low_y, low_b, scalar);
            "VMOV.I8  d26,#0x1d\t\n"
            "VMLAL.U8 q11,d4,d26\t\n"
            "VMLAL.U8 q12,d5,d26\t\n"

            // int16x8_t signed_scalar = vdupq_n_s16(-43);
            // high_u = vmulq_s16(signed_high_r, signed_scalar);
            // low_u = vmulq_s16(signed_low_r, signed_scalar);
            "VMVN.I16 q1,#0x2a\t\n"
            "VMUL.I16 q13,q5,q1\t\n"
            "VMUL.I16 q14,q6,q1\t\n"

            // signed_scalar = vdupq_n_s16(127);
            // high_v = vmulq_s16(signed_high_r, signed_scalar);
            // low_v = vmulq_s16(signed_low_r, signed_scalar);
            "VMOV.I16 q2,#0x7f\t\n"
            "VMUL.I16 q15,q5,q2\t\n"
            "VMUL.I16 q0,q6,q2\t\n"

            // signed_scalar = vdupq_n_s16(-84);
            // high_u = vmlaq_s16(high_u, signed_high_g, signed_scalar);
            // low_u = vmlaq_s16(low_u, signed_low_g, signed_scalar);
            "VMVN.I16 q1,#0x53\t\n"
            "VMLA.I16 q13,q7,q1\t\n"
            "VMLA.I16 q14,q8,q1\t\n"

            // signed_scalar = vdupq_n_s16(-106);
            // high_v = vmlaq_s16(high_v, signed_high_g, signed_scalar);
            // low_v = vmlaq_s16(low_v, signed_low_g, signed_scalar);
            "VMVN.I16 q2,#0x69\t\n"
            "VMLA.I16 q15,q7,q2\t\n"
            "VMLA.I16 q0,q8,q2\t\n"

            // signed_scalar = vdupq_n_s16(127);
            // high_u = vmlaq_s16(high_u, signed_high_b, signed_scalar);
            // low_u = vmlaq_s16(low_u, signed_low_b, signed_scalar);
            "VMOV.I16 q1,#0x7f\t\n"
            "VMLA.I16 q13,q9,q1\t\n"
            "VMLA.I16 q14,q10,q1\t\n"

            // signed_scalar = vdupq_n_s16(-21);
            // high_v = vmlaq_s16(high_v, signed_high_b, signed_scalar);
            // low_v = vmlaq_s16(low_v, signed_low_b, signed_scalar);
            "VMVN.I16 q2,#0x14\t\n"
            "VMLA.I16 q15,q9,q2\t\n"
            "VMLA.I16 q0,q10,q2\t\n"

            // high_y = vaddq_u16(high_y, u16_rounding);
            // low_y = vaddq_u16(low_y, u16_rounding);
            "VADD.I16 q11,q11,q3\t\n"
            "VADD.I16 q12,q12,q3\t\n"

            // high_u = vaddq_s16(high_u, s16_rounding);
            // low_u = vaddq_s16(low_u, s16_rounding);
            "VADD.I16 q13,q13,q3\t\n"
            "VADD.I16 q14,q14,q3\t\n"

            // high_v = vaddq_s16(high_v, s16_rounding);
            // low_v = vaddq_s16(low_v, s16_rounding);
            "VADD.I16 q15,q15,q3\t\n"
            "VADD.I16 q0,q0,q3\t\n"

            // pixel_yuv.val[0] = vcombine_u8(vqshrn_n_u16(low_y, 8), vqshrn_n_u16(high_y, 8));
            "VQSHRN.U16 d10,q11,#8\t\n"
            "VQSHRN.U16 d11,q12,#8\t\n"

            // u = vcombine_s8(vqshrn_n_s16(low_u, 8), vqshrn_n_s16(high_u, 8));
            "VQSHRN.S16 d12,q13,#8\t\n"
            "VQSHRN.S16 d13,q14,#8\t\n"

            // v = vcombine_s8(vqshrn_n_s16(low_v, 8), vqshrn_n_s16(high_v, 8));
            "VQSHRN.S16 d14,q15,#8\t\n"
            "VQSHRN.S16 d15,q0,#8\t\n"

            // u = vaddq_s8(u, s8_rounding);
            // v = vaddq_s8(v, s8_rounding);
            "VADD.I8  q6,q6,q4\t\n"
            "VADD.I8  q7,q7,q4\t\n"

            // vst3q_u8(yuv, pixel_yuv);
            "ADD      r12,r0,#0x18\t\n"
            "VST3.8   {d10,d12,d14},[r0]\t\n"
            "VST3.8   {d11,d13,d15},[r12]\t\n"
            // rgb += 3 * 16;
            // yuv += 3 * 16;
            "ADD      r1,r1,#0x30\t\n"
            "ADD      r0,r0,#0x30\t\n"
            // i++
            "ADD      r2,r2,#1\t\n"
            // i < count
            "CMP      r2,r3\t\n"
            "BLT      LOOP\t\n"
            : [r0] "+r" (yuv), [r1] "+r" (rgb), [r3] "+r" (count)
            :
            : "r2", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7","d8","d9","d10","d11","d12","d13","d14","d15","d16","d17","d18","d19","d20","d21","d22","d23","d24","d25","d26","d27","d28","d29","d30","d31"
                    );


    // Handle leftovers
    int i;
    for (i = count * 16; i < pixel_num; ++i) {
        uint8_t r = rgb[i * 3];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

        uint16_t y_tmp = 76 * r + 150 * g + 29 * b;
        int16_t u_tmp = -43 * r - 84 * g + 127 * b;
        int16_t v_tmp = 127 * r - 106 * g - 21 * b;

        y_tmp = (y_tmp + 128) >> 8;
        u_tmp = (u_tmp + 128) >> 8;
        v_tmp = (v_tmp + 128) >> 8;

        yuv[i * 3] = (uint8_t) y_tmp;
        yuv[i * 3 + 1] = (uint8_t) (u_tmp + 128);
        yuv[i * 3 + 2] = (uint8_t) (v_tmp + 128);
    }
}

template <>
void RGB2YUV444_impl<Strategies::NEON_ASSEMBLY2>(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int pixel_num, const int rgbChannelNum)
{
    int count = pixel_num / 16;

    asm volatile(
            "VMOV.I16 q3,#0x80\t\n"
            "VMOV.I8  q4,#0x80\t\n"
            "MOV      r2,#0\t\n"
            "LOOP1: \t\n"
            "ADD      r12,r1,#0x18\t\n"
            "VLD3.8   {d0,d2,d4},[r1]\t\n"
            "VLD3.8   {d1,d3,d5},[r12]\t\n"
            "VMOV.I8  d26,#0x4c\t\n"
            "VMOVL.U8 q5,d0\t\n"
            "VMOVL.U8 q6,d1\t\n"
            "VMULL.U8 q11,d0,d26\t\n"
            "VMULL.U8 q12,d1,d26\t\n"
            "VMOV.I8  d26,#0x96\t\n"
            "VMOVL.U8 q7,d2\t\n"
            "VMOVL.U8 q8,d3\t\n"
            "VMLAL.U8 q11,d2,d26\t\n"
            "VMLAL.U8 q12,d3,d26\t\n"
            "VMOV.I8  d26,#0x1d\t\n"
            "VMOVL.U8 q9,d4\t\n"
            "VMOVL.U8 q10,d5\t\n"
            "VMLAL.U8 q11,d4,d26\t\n"
            "VMLAL.U8 q12,d5,d26\t\n"
            "VMVN.I16 q1,#0x2a\t\n"
            "VMOV.I16 q2,#0x7f\t\n"
            "VMUL.I16 q13,q1,q5\t\n"
            "VMUL.I16 q14,q1,q6\t\n"
            "VMUL.I16 q15,q2,q5\t\n"
            "VMUL.I16 q0,q2,q6\t\n"
            "VMVN.I16 q1,#0x53\t\n"
            "VMVN.I16 q2,#0x69\t\n"
            "VMLA.I16 q13,q1,q7\t\n"
            "VMLA.I16 q14,q1,q8\t\n"
            "VMLA.I16 q15,q2,q7\t\n"
            "VMLA.I16 q0,q2,q8\t\n"
            "VMOV.I16 q1,#0x7f\t\n"
            "VMVN.I16 q2,#0x14\t\n"
            "VMLA.I16 q13,q9,q1\t\n"
            "VMLA.I16 q14,q10,q1\t\n"
            "VMLA.I16 q15,q9,q2\t\n"
            "VMLA.I16 q0,q10,q2\t\n"
            "VADD.I16 q11,q11,q3\t\n"
            "VADD.I16 q12,q12,q3\t\n"
            "VADD.I16 q13,q13,q3\t\n"
            "VADD.I16 q14,q14,q3\t\n"
            "VADD.I16 q15,q15,q3\t\n"
            "VADD.I16 q0,q0,q3\t\n"
            "VQSHRN.S16 d12,q13,#8\t\n"
            "VQSHRN.S16 d13,q14,#8\t\n"
            "VQSHRN.S16 d14,q15,#8\t\n"
            "VQSHRN.S16 d15,q0,#8\t\n"
            "VQSHRN.U16 d10,q11,#8\t\n"
            "VQSHRN.U16 d11,q12,#8\t\n"
            "VADD.I8  q6,q6,q4\t\n"
            "VADD.I8  q7,q7,q4\t\n"
            "ADD      r12,r0,#0x18\t\n"
            "ADD      r1,r1,#0x30\t\n"
            "VST3.8   {d10,d12,d14},[r0]\t\n"
            "VST3.8   {d11,d13,d15},[r12]\t\n"
            "ADD      r2,r2,#1\t\n"
            "ADD      r0,r0,#0x30\t\n"
            "CMP      r2,r3\t\n"
            "BLT      LOOP1\t\n"
            : [r0] "+r" (yuv), [r1] "+r" (rgb), [r3] "+r" (count)
            :
            : "r2", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7","d8","d9","d10","d11","d12","d13","d14","d15","d16","d17","d18","d19","d20","d21","d22","d23","d24","d25","d26","d27","d28","d29","d30","d31"
                );
}


bool is_valid_conversion(ColorConversionCodes code)
{
    return code == ColorConversionCodes::RGB2YUV444 || code == ColorConversionCodes::RGBA2YUV444;
}


void RGB2YUV444(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int width, const int height, const ColorConversionCodes code, const bool useNEON)
{
    assert(is_valid_conversion(code));

    int rgbChannelNum = channel_num(code);

    if (useNEON) {
        RGB2YUV444_impl<Strategies::NEON_INTRINSICS>(yuv, rgb, width * height, rgbChannelNum);
    } else {
        RGB2YUV444_impl<Strategies::SERIAL_INT>(yuv, rgb, width * height, rgbChannelNum);
    }
}
