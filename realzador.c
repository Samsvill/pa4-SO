#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"
#include "filter.h"

#define SHM_SIZE sizeof(BMP_Image)
#define FILTER_SIZE 3

// Estructura para pasar los argumentos a cada hilo
typedef struct
{
    int startRow;
    int endRow;
    BMP_Image *imageIn;
    BMP_Image *imageOut;
} ThreadArgs;

// Filtro de realce de bordes
int edgeEnhanceFilter[3][3] = {
    {-1, -1, -1},
    {-1, FILTER_SIZE, -1},
    {-1, -1, -1}
};

// Función que aplica el filtro de realce a una parte de la imagen
void *applyEdgeEnhance(void *args) {
    ThreadArgs *tArgs = (ThreadArgs *)args;
    int startRow = tArgs->startRow;
    int endRow = tArgs->endRow;
    BMP_Image *imageIn = tArgs->imageIn;
    BMP_Image *imageOut = tArgs->imageOut;

    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < imageIn->header.width_px; col++) {
            int sumBlue = 0, sumGreen = 0, sumRed = 0;

            // Aplicar el filtro de 3x3
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    int newRow = row + x;
                    int newCol = col + y;
                    if (newRow >= 0 && newRow < abs(imageIn->header.height_px) && 
                        newCol >= 0 && newCol < imageIn->header.width_px) {
                        sumBlue += edgeEnhanceFilter[x + 1][y + 1] * imageIn->pixels[newRow][newCol].blue;
                        sumGreen += edgeEnhanceFilter[x + 1][y + 1] * imageIn->pixels[newRow][newCol].green;
                        sumRed += edgeEnhanceFilter[x + 1][y + 1] * imageIn->pixels[newRow][newCol].red;
                    }
                }
            }

            // Normalizar y asegurar que los valores estén entre 0 y 255
            imageOut->pixels[row][col].blue = sumBlue / FILTER_SIZE;
            imageOut->pixels[row][col].green = sumGreen / FILTER_SIZE;
            imageOut->pixels[row][col].red = sumRed / FILTER_SIZE;
            imageOut->pixels[row][col].alpha = 255;  // Valor alfa fijo para opaco
        }
    }
    pthread_exit(NULL);
}

// Función para inicializar la imagen de salida con las mismas dimensiones que la de entrada
BMP_Image *initializeImageOut(BMP_Image *imageIn) {
    BMP_Image *imageOut = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imageOut == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    imageOut->header = imageIn->header;
    imageOut->norm_height = abs(imageIn->header.height_px);
    imageOut->bytes_per_pixel = imageIn->header.bits_per_pixel / 8;
    imageOut->pixels = (Pixel **)malloc(imageOut->norm_height * sizeof(Pixel *));
    if (imageOut->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < imageOut->norm_height; i++) {
        imageOut->pixels[i] = (Pixel *)malloc(imageOut->header.width_px * sizeof(Pixel));
        if (imageOut->pixels[i] == NULL) {
            printError(MEMORY_ERROR);
            exit(EXIT_FAILURE);
        }
    }

    return imageOut;
}

// Función para crear y lanzar los hilos para procesar la imagen
void createThreads(pthread_t *threads, ThreadArgs *threadArgs, BMP_Image *imageIn, BMP_Image *imageOut, int numThreads) {
    int rowsPerThread = abs(imageIn->header.height_px) / numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? abs(imageIn->header.height_px) : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        pthread_create(&threads[i], NULL, applyEdgeEnhance, &threadArgs[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <ruta_imagen> <número_hilos>\n", argv[0]);
        return 1;
    }

    // Cargar la imagen desde el archivo
    char *inputFile = argv[1];
    int numThreads = atoi(argv[2]);

    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL) {
        perror("Error al abrir el archivo de imagen");
        return 1;
    }

    BMP_Image *imageIn = createBMPImage(imageFile);
    if (imageIn == NULL) {
        printError(FILE_ERROR);
        return 1;
    }
    fclose(imageFile);

    // Inicializar la imagen de salida
    BMP_Image *imageOut = initializeImageOut(imageIn);

    // Crear y lanzar los hilos
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    createThreads(threads, threadArgs, imageIn, imageOut, numThreads);

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Guardar la imagen resultante
    writeImage("output_enhanced.bmp", imageOut);

    // Liberar la memoria
    freeImage(imageIn);
    freeImage(imageOut);

    printf("Imagen con filtro de realce guardada en: output_enhanced.bmp\n");
    return 0;
}
