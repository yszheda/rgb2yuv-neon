#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("<prog> <input-file> <output-1> <output-2>");
        return 0;
    }

    FILE* fp1;
    if ((fp1 = fopen(argv[1], "r")) == NULL) {
        printf("Error open file: %s!\n", argv[1]);
        exit(1);
    }
    int width, height;
    fscanf(fp1, "%d %d", &width, &height);
    // int channel_num = width * height * 3;
    int channel_num = width * height * 4;
    int channel_datasize = channel_num * sizeof(unsigned char);
    uint8_t *pixels1 = (uint8_t*) malloc(channel_datasize);
    uint8_t *pixels2 = (uint8_t*) malloc(channel_datasize);
    uint8_t *pixels3 = (uint8_t*) malloc(channel_datasize);
    if (fread(pixels1, sizeof(uint8_t), channel_num, fp1) == EOF) {
        printf("fread error!\n");
        exit(0);
    }
    fclose(fp1);

    FILE* fp2;
    if ((fp2 = fopen(argv[2], "r")) == NULL) {
        printf("Error open file: %s!\n", argv[2]);
        exit(1);
    }
    if (fread(pixels2, sizeof(uint8_t), channel_num, fp2) == EOF) {
        printf("fread error!\n");
        exit(0);
    }
    fclose(fp2);

    FILE* fp3;
    if ((fp3 = fopen(argv[3], "r")) == NULL) {
        printf("Error open file: %s!\n", argv[3]);
        exit(1);
    }
    if (fread(pixels3, sizeof(uint8_t), channel_num, fp3) == EOF) {
        printf("fread error!\n");
        exit(0);
    }
    fclose(fp3);

    int i;
    for (i = 0; i < channel_num; ++i) {
        if (pixels2[i] != pixels3[i]) {
            char channel_name;
            if (i % 3 == 0) {
                channel_name = 'y';
            } else if (i % 3 == 1) {
                channel_name = 'u';
            } else if (i % 3 == 2) {
                channel_name = 'v';
            }

            // int pixel_idx = (int) i / 3;
            // uint8_t r = pixels1[pixel_idx * 3];
            // uint8_t g = pixels1[pixel_idx * 3 + 1];
            // uint8_t b = pixels1[pixel_idx * 3 + 2];

            int pixel_idx = (int) i / 4;
            uint8_t r = pixels1[pixel_idx * 4 + 1];
            uint8_t g = pixels1[pixel_idx * 4 + 2];
            uint8_t b = pixels1[pixel_idx * 4 + 3];

            printf("%d pixel [rgb] %d %d %d [yuv] %c : %d %d\n", pixel_idx, r, g, b, channel_name, pixels2[i], pixels3[i]);
        }
    }

    free(pixels1);
    free(pixels2);
    free(pixels3);

    return 0;
}
