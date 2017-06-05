#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>

// #define DEBUG 1


////////////////////////////////////////////////////////////////////////////////
/*
 * Functions used for debugging
 */

void print_u8x8(uint8x8_t value)
{
    int i;
    for (i = 0; i < 8; ++i) {
        printf("%d ", value[i]);
    }
    printf("\n");
}

void print_s8x8(int8x8_t value)
{
    int i;
    for (i = 0; i < 8; ++i) {
        printf("%d ", value[i]);
    }
    printf("\n");
}

void print_u16x8(uint16x8_t value)
{
    int i;
    for (i = 0; i < 8; ++i) {
        printf("%d ", value[i]);
    }
    printf("\n");
}

void print_s16x8(int16x8_t value)
{
    int i;
    for (i = 0; i < 8; ++i) {
        printf("%d ", value[i]);
    }
    printf("\n");
}

void print_u8x16(uint8x16_t value)
{
    int i;
    for (i = 0; i < 16; ++i) {
        printf("%d ", value[i]);
    }
    printf("\n");
}

void print_s8x16(int8x16_t value)
{
    int i;
    for (i = 0; i < 16; ++i) {
        printf("%d ", value[i]);
    }
    printf("\n");
}

int16x8_t U8ToS16(uint8x8_t value)
{
    int16x8_t result;
    int i;
    for (i = 0; i < 8; ++i) {
        result[i] = value[i];
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////

// https://en.wikipedia.org/wiki/YUV
// Full swing for BT.601
void RGB2YUV_NEON(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, int pixel_num)
{
    const uint16x8_t u16_rounding = vdupq_n_u16(128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);
    const int8x16_t s8_rounding = vdupq_n_s8(128);

    int count = pixel_num / 16;

    int i;
    for (i = 0; i < count; ++i) {
        // Load rgb
        uint8x16x3_t pixel_rgb = vld3q_u8(rgb);

        uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
        uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
        // NOTE: vreinterpret will change the actual value
        // Use hand-written function instead
        int16x8_t signed_high_r = U8ToS16(high_r);
        int16x8_t signed_low_r = U8ToS16(low_r);

        uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
        uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
        int16x8_t signed_high_g = U8ToS16(high_g);
        int16x8_t signed_low_g = U8ToS16(low_g);

        uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
        uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);
        int16x8_t signed_high_b = U8ToS16(high_b);
        int16x8_t signed_low_b = U8ToS16(low_b);

#ifdef DEBUG
        printf("R\n");
        print_u8x8(high_r);
        print_u8x8(low_r);
        print_s16x8(signed_high_r);
        print_s16x8(signed_low_r);

        printf("G\n");
        print_u8x8(high_g);
        print_u8x8(low_g);
        print_s16x8(signed_high_g);
        print_s16x8(signed_low_g);

        printf("B\n");
        print_u8x8(high_b);
        print_u8x8(low_b);
        print_s16x8(signed_high_b);
        print_s16x8(signed_low_b);
#endif

        // 1. Multiply transform matrix (Y′: unsigned, U/V: signed)
        uint16x8_t high_y;
        uint16x8_t low_y;
        uint8x8_t scalar = vdup_n_u8(76);
        high_y = vmull_u8(high_r, scalar);
        low_y = vmull_u8(low_r, scalar);
        // NOTE: cannot use scalar multiplication here
        // because it cannot support uint8_t scalar.
        // The arm gcc compiler keeps complaining oddly...
        // error: incompatible types when assigning to type ‘uint16x8_t’ from type ‘int’
        // high_y = vmul_n_u8(high_r, 76);

#ifdef DEBUG
        printf("Y: r * 76\n");
        print_u16x8(high_y);
        print_u16x8(low_y);
#endif

        scalar = vdup_n_u8(150);
        high_y = vmlal_u8(high_y, high_g, scalar);
        low_y = vmlal_u8(low_y, low_g, scalar);

#ifdef DEBUG
        printf("Y: g * 150\n");
        print_u16x8(high_y);
        print_u16x8(low_y);
#endif

        scalar = vdup_n_u8(29);
        high_y = vmlal_u8(high_y, high_b, scalar);
        low_y = vmlal_u8(low_y, low_b, scalar);

#ifdef DEBUG
        printf("Y: b * 29\n");
        print_u16x8(high_y);
        print_u16x8(low_y);
#endif

        int16x8_t high_u;
        int16x8_t low_u;
        int16x8_t signed_scalar = vdupq_n_s16(-43);
        high_u = vmulq_s16(signed_high_r, signed_scalar);
        low_u = vmulq_s16(signed_low_r, signed_scalar);

#ifdef DEBUG
        printf("U: r * -43\n");
        print_s16x8(high_u);
        print_s16x8(low_u);
#endif

        signed_scalar = vdupq_n_s16(-84);
        high_u = vmlaq_s16(high_u, signed_high_g, signed_scalar);
        low_u = vmlaq_s16(low_u, signed_low_g, signed_scalar);

#ifdef DEBUG
        printf("U: g * -84\n");
        print_s16x8(high_u);
        print_s16x8(low_u);
#endif

        signed_scalar = vdupq_n_s16(127);
        high_u = vmlaq_s16(high_u, signed_high_b, signed_scalar);
        low_u = vmlaq_s16(low_u, signed_low_b, signed_scalar);

#ifdef DEBUG
        printf("U: b * 127\n");
        print_s16x8(high_u);
        print_s16x8(low_u);
#endif

        int16x8_t high_v;
        int16x8_t low_v;
        signed_scalar = vdupq_n_s16(127);
        high_v = vmulq_s16(signed_high_r, signed_scalar);
        low_v = vmulq_s16(signed_low_r, signed_scalar);

#ifdef DEBUG
        printf("V: r * 127\n");
        print_s16x8(high_v);
        print_s16x8(low_v);
#endif

        signed_scalar = vdupq_n_s16(-106);
        high_v = vmlaq_s16(high_v, signed_high_g, signed_scalar);
        low_v = vmlaq_s16(low_v, signed_low_g, signed_scalar);

#ifdef DEBUG
        printf("V: g * -106\n");
        print_s16x8(high_v);
        print_s16x8(low_v);
#endif

        signed_scalar = vdupq_n_s16(-21);
        high_v = vmlaq_s16(high_v, signed_high_b, signed_scalar);
        low_v = vmlaq_s16(low_v, signed_low_b, signed_scalar);

#ifdef DEBUG
        printf("V: b * -21\n");
        print_s16x8(high_v);
        print_s16x8(low_v);
#endif

        // 2. Scale down (">>8") to 8-bit values with rounding ("+128") (Y′: unsigned, U/V: signed)
        // 3. Add an offset to the values to eliminate any negative values (all results are 8-bit unsigned)
        uint8x16x3_t pixel_yuv;

        high_y = vaddq_u16(high_y, u16_rounding);
        low_y = vaddq_u16(low_y, u16_rounding);

#ifdef DEBUG
        printf("Y: y + 128\n");
        print_u16x8(high_y);
        print_u16x8(low_y);
#endif

        pixel_yuv.val[0] = vcombine_u8(vqshrn_n_u16(low_y, 8), vqshrn_n_u16(high_y, 8));


#ifdef DEBUG
        printf("Y: y >> 8\n");
        print_u8x8(vqshrn_n_u16(high_y, 8));
        print_u8x8(vqshrn_n_u16(low_y, 8));
        print_u8x16(pixel_yuv.val[0]);
#endif

        high_u = vaddq_s16(high_u, s16_rounding);
        low_u = vaddq_s16(low_u, s16_rounding);

#ifdef DEBUG
        printf("U: u + 128\n");
        print_s16x8(high_u);
        print_s16x8(low_u);
#endif

        int8x16_t u = vcombine_s8(vqshrn_n_s16(low_u, 8), vqshrn_n_s16(high_u, 8));

#ifdef DEBUG
        printf("U: u >> 8\n");
        print_s8x8(vqshrn_n_s16(low_u, 8));
        print_s8x8(vqshrn_n_s16(high_u, 8));
        print_s8x16(u);
#endif

        u = vaddq_s8(u, s8_rounding);
        pixel_yuv.val[1] = vreinterpretq_u8_s8(u);

#ifdef DEBUG
        printf("U: u + 128\n");
        print_s8x16(u);
        print_u8x16(pixel_yuv.val[1]);
#endif

        high_v = vaddq_s16(high_v, s16_rounding);
        low_v = vaddq_s16(low_v, s16_rounding);

#ifdef DEBUG
        printf("V: v + 128\n");
        print_s16x8(high_v);
        print_s16x8(low_v);
#endif

        int8x16_t v = vcombine_s8(vqshrn_n_s16(low_v, 8), vqshrn_n_s16(high_v, 8));

#ifdef DEBUG
        printf("V: v >> 8\n");
        print_s8x8(vqshrn_n_s16(low_v, 8));
        print_s8x8(vqshrn_n_s16(high_v, 8));
        print_s8x16(v);
#endif

        v = vaddq_s8(v, s8_rounding);
        pixel_yuv.val[2] = vreinterpretq_u8_s8(v);

#ifdef DEBUG
        printf("V: v + 128\n");
        print_s8x16(v);
        print_u8x16(pixel_yuv.val[2]);
#endif

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

int main(int argc, char* argv[])
{
    if(argc != 3) {
        printf("<prog> <input> <output>");
        return 0;
    }

    FILE* fp_in;
    if((fp_in = fopen(argv[1], "r")) == NULL)
    {
        printf("Error open input file!\n");
        exit(1);
    }

    int width, height;
    fscanf(fp_in, "%d %d", &width, &height);

    int channel_num = width * height * 3;
    int channel_datasize = channel_num * sizeof(unsigned char);
    unsigned char *rgb = (unsigned char *) malloc(channel_datasize);
    unsigned char *yuv = (unsigned char *) malloc(channel_datasize);

    if(fread(rgb, sizeof(unsigned char), channel_num, fp_in) == EOF)
    {
        printf("fread error!\n");
        exit(0);
    }

    fclose(fp_in);


    // Compute
    struct timespec start, end;
    double total_time;
    clock_gettime(CLOCK_REALTIME,&start);
    RGB2YUV_NEON(yuv, rgb, width * height);
    clock_gettime(CLOCK_REALTIME,&end);
    total_time = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_nsec - start.tv_nsec) / (double)1000000L;
    printf("RGB2YUV_NEON: %fms\n", total_time);


    FILE* fp_out;
    if((fp_out = fopen(argv[2], "w")) == NULL)
    {
        printf("Error open output file!\n");
        exit(1);
    }
    if(fwrite(yuv, sizeof(unsigned char), channel_num, fp_out) != channel_datasize)
    {
        printf("fwrite error!\n");
        exit(0);
    }
    fclose(fp_out);

    return 0;
}
