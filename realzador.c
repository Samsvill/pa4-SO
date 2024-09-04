#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"
#include "filter.h"

#define SHM_SIZE sizeof(BMP_Image)

// Estructura para pasar los argumentos a cada hilo
typedef struct {
    int startRow;
    int endRow;
    BMP_Image *imageIn;
    BMP_Image *imageOut;
} ThreadArgs;

// Filtro de realce de bordes
int edgeEnhanceFilter[3][3] = {
    {-1, -1, -1},
    {-1,  5, -1},
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

            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    int newRow = row + x;
                    int newCol = col + y;

                    // Asegurarse de que estamos dentro de los límites de la imagen
                    if (newRow >= 0 && newRow < imageIn->header.height_px && newCol >= 0 && newCol < imageIn->header.width_px) {
                        Pixel *p = &imageIn->pixels[newRow][newCol];
                        sumBlue += edgeEnhanceFilter[x + 1][y + 1] * p->blue;
                        sumGreen += edgeEnhanceFilter[x + 1][y + 1] * p->green;
                        sumRed += edgeEnhanceFilter[x + 1][y + 1] * p->red;
                    }
                }
            }

            // Normalizar el valor de los píxeles y asegurarse de que estén entre 0 y 255
            Pixel *pOut = &imageOut->pixels[row][col];
            pOut->blue = (sumBlue < 0) ? 0 : (sumBlue > 255) ? 255 : sumBlue;
            pOut->green = (sumGreen < 0) ? 0 : (sumGreen > 255) ? 255 : sumGreen;
            pOut->red = (sumRed < 0) ? 0 : (sumRed > 255) ? 255 : sumRed;
            pOut->alpha = 255;  // Asignar valor alfa como opaco
        }
    }
    return NULL;
}

// Función principal
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
        fprintf(stderr, "Error al cargar la imagen\n");
        fclose(imageFile);
        return 1;
    }
    fclose(imageFile);

    // Crear la imagen de salida con las mismas dimensiones
    BMP_Image *imageOut = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imageOut == NULL) {
        fprintf(stderr, "Error al asignar memoria para la imagen de salida\n");
        freeImage(imageIn);
        return 1;
    }
    imageOut->header = imageIn->header;
    imageOut->norm_height = imageIn->norm_height;
    imageOut->bytes_per_pixel = imageIn->bytes_per_pixel;
    imageOut->pixels = (Pixel **)malloc(imageOut->norm_height * sizeof(Pixel *));
        if (imageOut->pixels == NULL) {
            fprintf(stderr, "Error al asignar memoria para los píxeles de la imagen de salida\n");
            freeImage(imageIn);
            free(imageOut);
            return 1;
    }

    for (int i = 0; i < imageOut->norm_height; i++) {
    imageOut->pixels[i] = (Pixel *)malloc(imageOut->header.width_px * sizeof(Pixel));
    if (imageOut->pixels[i] == NULL) {
        fprintf(stderr, "Error al asignar memoria para la fila de píxeles %d de la imagen de salida\n", i);
        // Liberar la memoria ya asignada antes de salir
        for (int j = 0; j < i; j++) {
            free(imageOut->pixels[j]);
        }
        free(imageOut->pixels);
        freeImage(imageIn);
        free(imageOut);
        return 1;
    }
}

    // Crear y lanzar los hilos para procesar la imagen en paralelo
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = imageIn->header.height_px / numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? imageIn->header.height_px : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        pthread_create(&threads[i], NULL, applyEdgeEnhance, &threadArgs[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Guardar la imagen resultante
    char outputFile[] = "output_enhanced.bmp";
    writeImage(outputFile, imageOut);

    printf("Imagen con filtro de realce guardada en: %s\n", outputFile);

    // Liberar la memoria
    freeImage(imageIn);
    freeImage(imageOut);

    return 0;
}
