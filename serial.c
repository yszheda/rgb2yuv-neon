#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// #ifdef DEBUG 1

// https://en.wikipedia.org/wiki/YUV
// Full swing for BT.601
void RGB2YUV_integer(unsigned char *yuv, unsigned char *rgb, int pixel_num)
{
    int i;
    for (i = 0; i < pixel_num; ++i) {
        uint8_t r = rgb[i * 3];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

#ifdef DEBUG
        if (i == 0) {
            printf("r, g, b: %d %d %d\n", r, g, b);
        }
#endif

        uint16_t y_tmp = 76 * r + 150 * g + 29 * b;
        int16_t u_tmp = -43 * r - 84 * g + 127 * b;
        int16_t v_tmp = 127 * r - 106 * g - 21 * b;

#ifdef DEBUG
        if (i == 0) {
            printf("y, u, v: %d %d %d\n", y_tmp, u_tmp, v_tmp);
        }
#endif

        y_tmp = (y_tmp + 128) >> 8;
        u_tmp = (u_tmp + 128) >> 8;
        v_tmp = (v_tmp + 128) >> 8;

#ifdef DEBUG
        if (i == 0) {
            printf("y, u, v: %d %d %d\n", y_tmp, u_tmp, v_tmp);
        }
#endif

        yuv[i * 3] = (uint8_t) y_tmp;
        yuv[i * 3 + 1] = (uint8_t) (u_tmp + 128);
        yuv[i * 3 + 2] = (uint8_t) (v_tmp + 128);

#ifdef DEBUG
        if (i == 0) {
            printf("y, u, v: %d %d %d\n", yuv[i * 3], yuv[i * 3 + 1], yuv[i * 3 + 2]);
        }
#endif
    }
}

void RGB2YUV_float(unsigned char *yuv, unsigned char *rgb, int pixel_num)
{
    int i;
    for (i = 0; i < pixel_num; ++i) {
        uint8_t r = rgb[i * 3];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];
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

int main(int argc, char* argv[])
{
    if (argc != 3 || argc != 4) {
        printf("<prog> <input> <output> [-f]");
        return 0;
    }

    FILE* fp_in;
    if ((fp_in = fopen(argv[1], "r")) == NULL) {
        printf("Error open input file!\n");
        exit(1);
    }

    int width, height;
    fscanf(fp_in, "%d %d", &width, &height);

    int channel_num = width * height * 3;
    int channel_datasize = channel_num * sizeof(unsigned char);
    unsigned char *rgb = (unsigned char *) malloc(channel_datasize);
    unsigned char *yuv = (unsigned char *) malloc(channel_datasize);

    if (fread(rgb, sizeof(unsigned char), channel_num, fp_in) == EOF) {
        printf("fread error!\n");
        exit(0);
    }

    fclose(fp_in);


    // Compute
    struct timespec start, end;
    double total_time;
    clock_gettime(CLOCK_REALTIME,&start);
    if (strcmp(argv[3], "-f") == 0) {
        RGB2YUV_float(yuv, rgb, width * height);
    } else {
        RGB2YUV_integer(yuv, rgb, width * height);
    }
    clock_gettime(CLOCK_REALTIME,&end);
    total_time = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_nsec - start.tv_nsec) / (double)1000000L;
    printf("RGB2YUV_serial: %f ms\n", total_time);


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
