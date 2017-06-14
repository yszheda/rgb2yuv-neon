#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>

void encodeYUV420SP(unsigned char * yuv420sp, unsigned char * argb, int width, int height)
{
    int frameSize = width * height;

    int yIndex = 0;
    int uvIndex = frameSize;

    int index = 0;
    int i, j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            uint8_t A = argb[i * 4];
            uint8_t R = argb[i * 4 + 1];
            uint8_t G = argb[i * 4 + 2];
            uint8_t B = argb[i * 4 + 3];

            // well known RGB to YUV algorithm
            uint8_t Y = (( 66 * R + 129 * G +  25 * B + 128) >> 8) +  16;
            uint8_t U = ((-38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
            uint8_t V = ((112 * R -  94 * G -  18 * B + 128) >> 8) + 128;

            // NV21 has a plane of Y and interleaved planes of VU each sampled by a factor of 2
            //    meaning for every 4 Y pixels there are 1 V and 1 U.  Note the sampling is every other
            //    pixel AND every other scanline.
            yuv420sp[yIndex++] = (unsigned char) ((Y < 0)? 0: ((Y > 255) ? 255 : Y));
            if (j % 2 == 0 && index % 2 == 0) {
                yuv420sp[uvIndex++] = (unsigned char)((U < 0) ? 0 : ((U > 255) ? 255 : U));
                yuv420sp[uvIndex++] = (unsigned char)((V < 0) ? 0 : ((V > 255) ? 255 : V));
            }

            index ++;
        }
    }
}


void encodeYUV420SP_NEON_Intrinsics(unsigned char * __restrict__ yuv420sp, unsigned char * __restrict__ argb, int width, int height)
{
    const uint16x8_t u16_rounding = vdupq_n_u16(128);
    const int16x8_t s16_rounding = vdupq_n_s16(128);
    const int8x8_t s8_rounding = vdup_n_s8(128);
    const uint8x16_t offset = vdupq_n_u8(16);

    int frameSize = width * height;

    int yIndex = 0;
    int uvIndex = frameSize;

    int i;
    int j;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width >> 4; i++) {
            // Load rgb
            uint8x16x4_t pixel_argb = vld4q_u8(argb);
            argb += 4 * 16;

            uint8x8_t high_r = vget_high_u8(pixel_argb.val[1]);
            uint8x8_t low_r = vget_low_u8(pixel_argb.val[1]);
            uint8x8_t high_g = vget_high_u8(pixel_argb.val[2]);
            uint8x8_t low_g = vget_low_u8(pixel_argb.val[2]);
            uint8x8_t high_b = vget_high_u8(pixel_argb.val[3]);
            uint8x8_t low_b = vget_low_u8(pixel_argb.val[3]);

            // NOTE:
            // declaration may not appear after executable statement in block
            uint16x8_t high_y;
            uint16x8_t low_y;

            uint8x8_t scalar = vdup_n_u8(66);
            uint8x16_t y;

            high_y = vmull_u8(high_r, scalar);
            low_y = vmull_u8(low_r, scalar);
            scalar = vdup_n_u8(129);
            high_y = vmlal_u8(high_y, high_g, scalar);
            low_y = vmlal_u8(low_y, low_g, scalar);
            scalar = vdup_n_u8(25);
            high_y = vmlal_u8(high_y, high_b, scalar);
            low_y = vmlal_u8(low_y, low_b, scalar);

            high_y = vaddq_u16(high_y, u16_rounding);
            low_y = vaddq_u16(low_y, u16_rounding);

            y = vcombine_u8(vqshrn_n_u16(low_y, 8), vqshrn_n_u16(high_y, 8));
            y = vaddq_u8(y, offset);

            vst1q_u8(yuv420sp + yIndex, y);
            yIndex += 16;

            // Compute u and v in the even row
            if (j % 2 == 0) {
                int16x8_t u_scalar = vdupq_n_s16(-38);
                int16x8_t v_scalar = vdupq_n_s16(112);

                int16x8_t r = vreinterpretq_s16_u16(vmovl_u8(vqshrn_n_u16(vshlq_n_u16(vreinterpretq_u16_u8(pixel_argb.val[1]), 8), 8)));
                int16x8_t g = vreinterpretq_s16_u16(vmovl_u8(vqshrn_n_u16(vshlq_n_u16(vreinterpretq_u16_u8(pixel_argb.val[2]), 8), 8)));
                int16x8_t b = vreinterpretq_s16_u16(vmovl_u8(vqshrn_n_u16(vshlq_n_u16(vreinterpretq_u16_u8(pixel_argb.val[3]), 8), 8)));

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

                uv.val[0] = vreinterpret_u8_s8(vadd_s8(vqshrn_n_s16(u, 8), s8_rounding));
                uv.val[1] = vreinterpret_u8_s8(vadd_s8(vqshrn_n_s16(v, 8), s8_rounding));

                vst2_u8(yuv420sp + uvIndex, uv);

                uvIndex += 2 * 8;
            }
        }

        // Handle leftovers
        for (i = ((width >> 4) << 4); i < width; i++) {
            uint8_t R = argb[1];
            uint8_t G = argb[2];
            uint8_t B = argb[3];
            argb += 4;

            // well known RGB to YUV algorithm
            uint8_t Y = (( 66 * R + 129 * G +  25 * B + 128) >> 8) +  16;
            uint8_t U = ((-38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
            uint8_t V = ((112 * R -  94 * G -  18 * B + 128) >> 8) + 128;

            // NV21 has a plane of Y and interleaved planes of VU each sampled by a factor of 2
            //    meaning for every 4 Y pixels there are 1 V and 1 U.  Note the sampling is every other
            //    pixel AND every other scanline.
            yuv420sp[yIndex++] = Y;
            if (j % 2 == 0 && i % 2 == 0) {
                yuv420sp[uvIndex++] = U;
                yuv420sp[uvIndex++] = V;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("<prog> <input> <output>");
        return 0;
    }

    FILE* fp_in;
    if ((fp_in = fopen(argv[1], "r")) == NULL) {
        printf("Error open input file!\n");
        exit(1);
    }

    int width, height;
    fscanf(fp_in, "%d %d", &width, &height);

    int channel_num = width * height * 4;
    int channel_datasize = channel_num * sizeof(unsigned char);
    int channel_num_yuv = width * height * 6 / 4;
    int channel_datasize_yuv = channel_num_yuv * sizeof(unsigned char);
    unsigned char *rgb = (unsigned char *) malloc(channel_datasize);
    unsigned char *yuv = (unsigned char *) malloc(channel_datasize_yuv);

    if (fread(rgb, sizeof(unsigned char), channel_num, fp_in) == EOF) {
        printf("fread error!\n");
        exit(0);
    }

    fclose(fp_in);


    // Compute
    struct timespec start, end;
    double total_time;
    clock_gettime(CLOCK_REALTIME, &start);
    encodeYUV420SP_NEON_Intrinsics(yuv, rgb, width, height);
    clock_gettime(CLOCK_REALTIME,&end);
    total_time = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_nsec - start.tv_nsec) / (double)1000000L;
    printf("RGB2YUV_NEON: %f ms\n", total_time);


    FILE* fp_out;
    if ((fp_out = fopen(argv[2], "w")) == NULL) {
        printf("Error open output file!\n");
        exit(1);
    }
    if (fwrite(yuv, sizeof(unsigned char), channel_num_yuv, fp_out) != channel_datasize_yuv) {
        printf("fwrite error!\n");
        exit(0);
    }
    fclose(fp_out);

    return 0;
}
