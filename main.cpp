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
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <chrono>

#include "enum.hpp"
#include "yuv420sp.hpp"


// Use type T to deduce const char
template<typename T, class F, class... Args>
void time_measure(T&& funcTag, F&& func, Args&&... args)
{
    const int cnt = 1000;
    auto begin = std::chrono::steady_clock::now();
    for (auto i = 0; i < cnt; i++) {
        std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << funcTag << " Time = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() / cnt << " ns." << std::endl;
}


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
    // NOTE: stlport_static do not support unique_ptr
    // std::unique_ptr<unsigned char []> rgb(new unsigned char[rgbDataSize]);
    unsigned char *rgb = (unsigned char *) malloc(rgbDataSize);

    int yuvDataSize = width * height * yuv_size_factor(code) * sizeof(unsigned char);
    // std::unique_ptr<unsigned char []>yuv(new unsigned char[yuvDataSize]);
    unsigned char *yuv = (unsigned char *) malloc(yuvDataSize);

    // if (fread(rgb.get(), sizeof(unsigned char), rgbDataSize, fpIn) == EOF) {
    if (fread(rgb, sizeof(unsigned char), rgbDataSize, fpIn) == EOF) {
        printf("fread error!\n");
        exit(0);
    }
    fclose(fpIn);


    time_measure<>("RGB2YUV420SP with NEON", YUV420SP::convert, yuv, rgb, width, height, code, true);


    FILE* fpOut;
    if ((fpOut = fopen(outFileName, "w")) == NULL) {
        printf("Error open output file!\n");
        exit(1);
    }
    // if (fwrite(yuv.get(), sizeof(unsigned char), yuvDataSize, fpOut) != yuvDataSize) {
    if (fwrite(yuv, sizeof(unsigned char), yuvDataSize, fpOut) != yuvDataSize) {
        printf("fwrite error!\n");
        exit(0);
    }
    fclose(fpOut);

    free(rgb);
    free(yuv);

    return 0;
}
