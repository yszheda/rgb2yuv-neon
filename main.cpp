/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2017年06月20日 10时16分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuai YUAN (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <getopt.h>
#include <cassert>
#include <string>
#include <map>

#include "enum.hpp"
#include "yuv420sp.hpp"

using namespace std;


void show_help_info()
{
}


ColorConversionCodes get_conversion_code(std::string& codeName)
{
    std::map<std::string, ColorConversionCodes> validNames;
    validNames["RGB2YUV444"] = ColorConversionCodes::RGB2YUV444;
    validNames["RGBA2YUV444"] = ColorConversionCodes::RGBA2YUV444;
    validNames["RGB2YUV420_NV21"] = ColorConversionCodes::RGB2YUV420_NV21;
    validNames["RGBA2YUV420_NV21"] = ColorConversionCodes::RGBA2YUV420_NV21;
    validNames["RGB2YUV420_NV12"] = ColorConversionCodes::RGB2YUV420_NV12;
    validNames["RGBA2YUV420_NV12"] = ColorConversionCodes::RGBA2YUV420_NV12;
    return validNames[codeName];
}


int main(int argc, char* argv[])
{
    char *inFileName = NULL;
    char *outFileName = NULL;
    std::string codeName;
    ColorConversionCodes code = ColorConversionCodes::NUM;

    int option;
    while((option = getopt(argc, argv, "Ii:Oo:Cc")) != -1) {
        switch (option) {
            case 'I':
            case 'i':
                inFileName = optarg;
                break;

            case 'O':
            case 'o':
                outFileName = optarg;
                break;

            case 'C':
            case 'c':
                codeName = std::string(optarg);
                code = get_conversion_code(codeName);
                break;

            default:
                show_help_info();
                break;
        }
    }

    assert(inFileName != NULL);
    assert(outFileName != NULL);
    assert(code != ColorConversionCodes::NUM);

    FILE* fpIn;
    if ((fpIn = fopen(inFileName, "r")) == NULL) {
        printf("Error open input file!\n");
        exit(1);
    }

    int width, height;
    fscanf(fpIn, "%d %d", &width, &height);

    int rgbChannelNum = channel_num(code);
    int rgbDataSize = width * height * rgbChannelNum * sizeof(unsigned char);
    unsigned char *rgb = (unsigned char *) malloc(rgbDataSize);

    int yuvDataSize = width * height * yuv_size_factor(code) * sizeof(unsigned char);
    unsigned char *yuv = (unsigned char *) malloc(yuvDataSize);

    if (fread(rgb, sizeof(unsigned char), rgbDataSize, fpIn) == EOF) {
        printf("fread error!\n");
        exit(0);
    }

    fclose(fpIn);


    // Compute
    struct timespec start, end;
    double total_time;
    clock_gettime(CLOCK_REALTIME,&start);
    RGB2YUV420SP(yuv, rgb, width, height, code, true);
    clock_gettime(CLOCK_REALTIME,&end);
    total_time = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_nsec - start.tv_nsec) / (double)1000000L;
    printf("RGB2YUV_NEON: %f ms\n", total_time);


    FILE* fpOut;
    if ((fpOut = fopen(outFileName, "w")) == NULL) {
        printf("Error open output file!\n");
        exit(1);
    }
    if (fwrite(yuv, sizeof(unsigned char), yuvDataSize, fpOut) != yuvDataSize) {
        printf("fwrite error!\n");
        exit(0);
    }
    fclose(fpOut);

    free(rgb);
    free(yuv);

    return 0;
}
