#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>


// https://en.wikipedia.org/wiki/YUV
// Full swing for BT.601
void RGB2YUV_NEON(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, int pixel_num)
{
    const uint16x8_t u16_rounding = vdupq_n_u16(128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);
    const int8x16_t s8_rounding = vdupq_n_s8(128);

    pixel_num /= 16;

    int i;
    for (i = 0; i < pixel_num; ++i) {
        // Load
        uint8x16x3_t pixel_rgb = vld3q_u8(rgb);

        uint8x8_t high_r = vget_high_u8(pixel_rgb.val[0]);
        uint8x8_t low_r = vget_low_u8(pixel_rgb.val[0]);
        int8x8_t signed_high_r = vreinterpret_s8_u8(high_r);
        int8x8_t signed_low_r = vreinterpret_s8_u8(low_r);

        uint8x8_t high_g = vget_high_u8(pixel_rgb.val[1]);
        uint8x8_t low_g = vget_low_u8(pixel_rgb.val[1]);
        int8x8_t signed_high_g = vreinterpret_s8_u8(high_g);
        int8x8_t signed_low_g = vreinterpret_s8_u8(low_g);

        uint8x8_t high_b = vget_high_u8(pixel_rgb.val[2]);
        uint8x8_t low_b = vget_low_u8(pixel_rgb.val[2]);
        int8x8_t signed_high_b = vreinterpret_s8_u8(high_b);
        int8x8_t signed_low_b = vreinterpret_s8_u8(low_b);


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

        scalar = vdup_n_u8(150);
        high_y = vmlal_u8(high_y, high_g, scalar);
        low_y = vmlal_u8(low_y, low_g, scalar);

        scalar = vdup_n_u8(29);
        high_y = vmlal_u8(high_y, high_b, scalar);
        low_y = vmlal_u8(low_y, low_b, scalar);


        int16x8_t high_u;
        int16x8_t low_u;
        int8x8_t signed_scalar = vdup_n_s8(-43);
        // NOTE: vreinterpret will not change the value?
        high_u = vmull_s8(signed_high_r, signed_scalar);
        low_u = vmull_s8(signed_low_r, signed_scalar);

        signed_scalar = vdup_n_s8(-84);
        high_u = vmlal_s8(high_u, signed_high_g, signed_scalar);
        low_u = vmlal_s8(low_u, signed_low_g, signed_scalar);

        signed_scalar = vdup_n_s8(127);
        high_u = vmlal_s8(high_u, signed_high_b, signed_scalar);
        low_u = vmlal_s8(low_u, signed_low_b, signed_scalar);


        int16x8_t high_v;
        int16x8_t low_v;
        signed_scalar = vdup_n_s8(127);
        high_v = vmull_s8(signed_high_r, signed_scalar);
        low_v = vmull_s8(signed_low_r, signed_scalar);

        signed_scalar = vdup_n_s8(-106);
        high_v = vmlal_s8(high_v, signed_high_g, signed_scalar);
        low_v = vmlal_s8(low_v, signed_low_g, signed_scalar);

        signed_scalar = vdup_n_s8(-21);
        high_v = vmlal_s8(high_v, signed_high_b, signed_scalar);
        low_v = vmlal_s8(low_v, signed_low_b, signed_scalar);

        // 2. Scale down (">>8") to 8-bit values with rounding ("+128") (Y′: unsigned, U/V: signed)
        // 3. Add an offset to the values to eliminate any negative values (all results are 8-bit unsigned)
        uint8x16x3_t pixel_yuv;

        high_y = vaddq_u16(high_y, u16_rounding);
        low_y = vaddq_u16(low_y, u16_rounding);
        pixel_yuv.val[0] = vcombine_u8(vqrshrn_n_u16(high_y, 8), vqrshrn_n_u16(low_y, 8));

        high_u = vaddq_s16(high_u, s16_rounding);
        low_u = vaddq_s16(low_u, s16_rounding);
        int8x16_t u = vcombine_s8(vqrshrn_n_s16(high_u, 8), vqrshrn_n_s16(low_u, 8));
        u = vaddq_s8(u, s8_rounding);
        pixel_yuv.val[1] = vreinterpretq_u8_s8(u);

        high_v = vaddq_s16(high_v, s16_rounding);
        low_v = vaddq_s16(low_v, s16_rounding);
        int8x16_t v = vcombine_s8(vqrshrn_n_s16(high_v, 8), vqrshrn_n_s16(low_v, 8));
        v = vaddq_s8(v, s8_rounding);
        pixel_yuv.val[2] = vreinterpretq_u8_s8(v);


        // Store
        vst3q_u8(yuv, pixel_yuv);

        rgb += 3 * 16;
        yuv += 3 * 16;
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
