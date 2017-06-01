#include<stdio.h>
#include<stdlib.h>
#include<time.h>

int main(int argc, char* argv[])
{
    srand(time(NULL));

    if(argc != 4) {
        printf("gen_testcase <width> <height> <filename>");
        return 0;
    }

    printf("generating test case %s...\n", argv[3]);

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    FILE* out = fopen(argv[3], "w");

    fprintf(out, "%d %d\n", width, height);

    int i, j;
    for(i = 0; i < height; i++){
        for(j = 0; j < width; j++){
            int k;
            for (k = 0; k < 3; k++) {
                unsigned char value = rand() % 256;
                fprintf(out, "%c", value);
            }
        }
    }

    fclose(out);

    return 0;
}
