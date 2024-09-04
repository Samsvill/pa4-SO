#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bmp.h"
#include "filter.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <ruta_imagen> <número_hilos>\n", argv[0]);
        return 1;
    }

    char *inputFile = argv[1];
    int numThreads = atoi(argv[2]);

    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL) {
        printError(FILE_ERROR);
        return 1;
    }

    BMP_Image *imageIn = createBMPImage(imageFile);
    fclose(imageFile);

    BMP_Image *imageOut = initializeImageOut(imageIn);

    // Crear hilos para aplicar el filtro
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = abs(imageIn->header.height_px) / numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? abs(imageIn->header.height_px) : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].filter = blurFilter;
        //printf("Hilo %d: Procesando desde fila %d hasta fila %d\n", i, threadArgs[i].startRow, threadArgs[i].endRow);
        pthread_create(&threads[i], NULL, applyFilter, &threadArgs[i]);
    }


    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    writeImage("output_enhanced.bmp", imageOut);

    // Liberar la memoria
    freeImage(imageIn);
    freeImage(imageOut);

    return 0;
}
