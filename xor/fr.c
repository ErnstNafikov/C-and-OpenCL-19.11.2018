
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>

void main() {
    int i = 0;
    for(i=0;i<4;i++) {
        //«апись данных
        int j = (int)(i%2);
        int in = (int)(i/2);
        char p[16];
        char s[16];
        itoa(i,p,15);
        float I[2] = {in, j};
        strcpy(s,"input/I");
        strcat(s,p);
        strcat(s,".bin");
        FILE* fd = fopen(s,"wb");
        fwrite(I, 1, sizeof(I), fd);
        fclose(fd);
        printf("start");
        system("cls");
        printf(s);
        Sleep(1000);
    }
    float O0[2] = {1, 0.5};
    FILE* fd = fopen("output/O1.bin", "wb");
    fwrite(O0, 1, sizeof(O0), fd);
    fclose(fd);
    fd = fopen("output/O2.bin", "wb");
    fwrite(O0, 1, sizeof(O0), fd);
    fclose(fd);
    float O1[2] = {0, 0.5};
    fd = fopen("output/O0.bin", "wb");
    fwrite(O1, 1, sizeof(O1), fd);
    fclose(fd);
    fd = fopen("output/O3.bin", "wb");
    fwrite(O1, 1, sizeof(O1), fd);
    fclose(fd);
    /*
    //„тение данных
    FILE* fd = fopen("I.bin", "rb");  //ключ должен быть "rb" - чтение бинарных данных
    if (fd == NULL)
        printf("Error opening file for reading");
    size_t result = fread(I, 1, sizeof(I), fd);
    if (result != sizeof(I))
        printf("Error reading file"); //прочитали количество байт не равное размеру массива
    fclose(fd);
    */
}

