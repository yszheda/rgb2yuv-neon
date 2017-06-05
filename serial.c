#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// #ifdef DEBUG 1

// https://en.wikipedia.org/wiki/YUV
// Full swing for BT.601
void RGB2YUV_serial(unsigned char *yuv, unsigned char *rgb, int pixel_num)
{
    int i;
    for (i = 0; i < pixel_num; ++i) {
        uint8_t r = rgb[i * 3];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

        if (i == 0) {
            printf("r, g, b: %d %d %d\n", r, g, b);
        }

        uint16_t y_tmp = 76 * r + 150 * g + 29 * b;
        int16_t u_tmp = -43 * r - 84 * g + 127 * b;
        int16_t v_tmp = 127 * r - 106 * g - 21 * b;

        if (i == 0) {
            printf("y: %d\n", y_tmp);
        }

        y_tmp = (y_tmp + 128) >> 8;
        u_tmp = (u_tmp + 128) >> 8;
        v_tmp = (v_tmp + 128) >> 8;

        if (i == 0) {
            printf("y: %d\n", y_tmp);
        }

        yuv[i * 3] = (uint8_t) y_tmp;
        yuv[i * 3 + 1] = (uint8_t) (u_tmp + 128);
        yuv[i * 3 + 2] = (uint8_t) (v_tmp + 128);

        if (i == 0) {
            printf("y: %d\n", yuv[i * 3]);
        }
    }
}

void RGB2YUV_serial1(unsigned char *yuv, unsigned char *rgb, int pixel_num)
{
    int i;
    for (i = 0; i < pixel_num; ++i) {
        float r = rgb[i * 3];
        float g = rgb[i * 3 + 1];
        float b = rgb[i * 3 + 2];
        float y = 0.299*r + 0.587*g + 0.114*b;
        float u = 0.492 * (b - y);
        float v = 0.877 * (r - y);

        yuv[i * 3] = (unsigned char) y;
        yuv[i * 3 + 1] = (unsigned char) u;
        yuv[i * 3 + 2] = (unsigned char) v;
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
    double totalTime;
    clock_gettime(CLOCK_REALTIME,&start);
    RGB2YUV_serial(yuv, rgb, width * height);
    clock_gettime(CLOCK_REALTIME,&end);
    totalTime = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_nsec - start.tv_nsec) / (double)1000000L;
    printf("RGB2YUV_serial: %fms\n", totalTime);


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
