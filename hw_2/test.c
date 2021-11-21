#include <stdio.h>
#include <stdlib.h>
//#include "implementation.h"

#define SIZE_1 ((size_t) 16)
#define SIZE_2 ((size_t) 32)
#define SIZE_3 (18777216) // 16 MB

int main(int argc, char *argv[]){
    char *c_1, *c_2, *c_3, *c_4;
    int *i_1, *i_2;
    //c_1 = (char *) __malloc_impl(SIZE_1);
    c_1 = (char *) malloc(SIZE_1);
    for (int i = 0; i < SIZE_1; i++){
        c_1[i] = 'A';
    }
    c_1[SIZE_1-1] = '\0';
    //c_2 = (char *) __malloc_impl(SIZE_2);
    c_2 = (char *) malloc(SIZE_2);

    for (int i = 0; i < SIZE_2; i++){
        c_2[i] = 'B';
    }
    c_2[SIZE_2-1] = '\0';

    //c_3 = (char *) __malloc_impl(SIZE_2);
    c_3 = (char *) malloc(SIZE_2);
    for (int i = 0; i < SIZE_2; i++){
        c_3[i] = 'C';
    }
    c_3[SIZE_2-1] = '\0';

    printf("%s\n", c_1);
    printf("%s\n", c_2);
    printf("%s\n", c_3);

    //__free_impl(c_2);
    //__free_impl(c_3);
    free(c_2);

    i_1 = (int *) malloc(SIZE_1);
    for (int i = 0; i < SIZE_1; i++){
        i_1[i] = i;
        printf("%d ", i_1[i]);
    }
    printf("\n");

    i_1 = (int *) realloc(i_1, SIZE_2);
    for (int i = 0; i < SIZE_2; i++){
        i_1[i] = i;
        printf("%d ", i_1[i]);
    } 
    printf("\n");

    i_2 = (int *) calloc(SIZE_2, sizeof(int));
    for (int i = 0; i < SIZE_1; i++){
        printf("%d ", i_2[i]);
    } 
    printf("\n");

    c_3 = (char *) realloc(c_3, (size_t) 50);
    for (int i = SIZE_2-1; i < 50; i++)
        c_3[i] = 'C';

    c_3[49] = '\0';
    printf("%s\n", c_3);
    free(c_3);
    free(i_1);
    free(i_2);

    
    //c_4 = (char *) __malloc_impl(SIZE_2);
    c_4 = (char *) malloc(SIZE_3);
    for (int i = 0; i < SIZE_3; i++){
        c_4[i] = 'D';
    }
    c_4[SIZE_3-1] = '\0';

    //printf("%s\n", c_4);

    free(c_1);
    free(c_4);
    //__free_impl(c_4);
    //__free_impl(c_4);

    return 0;
}
