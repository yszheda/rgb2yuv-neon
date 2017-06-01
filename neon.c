#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>


// https://en.wikipedia.org/wiki/YUV
// Full swing for BT.601
void RGB2YUV_NEON(unsigned char * __restrict__ yuv, unsigned char * __restrict__ rgb, const int pixel_num)
{
    const uint8x8_t rounding = vdup_n_u8(128);

    pixel_num /= 8;

    for (int i = 0; i < pixel_num; ++i) {
        uint8x8x3_t pixel_rgb = vld3_u8(rgb);
        uint8x8x3_t pixel_yuv;


        // 1. Multiply transform matrix (Y′: unsigned, U/V: signed)
        uint16x8_t y_tmp;
        y_tmp = vmull_n_u8(rgb.val[0], 76);
        y_tmp = vmlal_n_u8(y_tmp, rgb.val[1], 150);
        y_tmp = vmlal_n_u8(y_tmp, rgb.val[2], 29);

        int16x8_t u_tmp;
        u_tmp = vmul_n_s16(rgb.val[0], -43);
        u_tmp = vmla_n_s16(u_tmp, rgb.val[1], -84);
        u_tmp = vmla_n_s16(u_tmp, rgb.val[2], 127);

        int16x8_t v_tmp;
        v_tmp = vmul_n_s16(rgb.val[0], 127);
        v_tmp = vmla_n_s16(v_tmp, rgb.val[1], -106);
        v_tmp = vmla_n_s16(v_tmp, rgb.val[2], -21);


        // 2. Scale down (">>8") to 8-bit values with rounding ("+128") (Y′: unsigned, U/V: signed)
        // 3. Add an offset to the values to eliminate any negative values (all results are 8-bit unsigned)
        y_tmp = vadd_u16(y_tmp, rounding);
        pixel_yuv.val[0] = vshrn_n_u16(y_tmp, 8);

        u_tmp = vadd_s16(u_tmp, rounding);
        pixel_yuv.val[1] = vshrn_n_s16(u_tmp, 8);
        pixel_yuv.val[1] = vadd_u8(pixel_yuv.val[1], rounding);

        v_tmp = vadd_s16(v_tmp, rounding);
        pixel_yuv.val[2] = vshrn_n_s16(v_tmp, 8);
        pixel_yuv.val[2] = vadd_u8(pixel_yuv.val[2], rounding);


        vst1_u8(yuv, pixel_yuv);

        rgb += 3 * 8;
        yuv += 3 * 8;
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
    RGB2YUV_NEON(yuv, rgb, width, height);
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
