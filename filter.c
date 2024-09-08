#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Definición del filtro de realce de bordes (Edge Enhance)
int simplifiedSobelFilter[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

int simpleEdgeEnhanceFilter[3][3] = {
    {0, -1, 0},
    {-1, 4, -1},
    {0, -1, 0}
};

//Gaussiano!
int blurFilter[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};

int sharpenFilter[3][3] = {
    {0, -1, 0},
    {-1, 5, -1},
    {0, -1, 0}
};

// Función que aplica un filtro a una parte de la imagen (en paralelo)
void *applyFilter(void *args) {
    ThreadArgs *tArgs = (ThreadArgs *)args;
    int startRow = tArgs->startRow;
    int endRow = tArgs->endRow;
    BMP_Image *imageIn = tArgs->imageIn;
    BMP_Image *imageOut = tArgs->imageOut;
    int (*filter)[3] = tArgs->filter;

    int width = imageIn->header.width_px;  // El ancho de la imagen

    // Sumar los valores del filtro para normalización
    int filterSum = 0;
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            filterSum += filter[x][y];
        }
    }

    // Evitar divisiones por 0 si el filtro tiene sumas nulas
    
    
    for (int row = startRow; row < endRow; row++) {
        printf("dentro del primer for\n");
        
        for (int col = 0; col < width; col++) {
            int sumBlue = 0, sumGreen = 0, sumRed = 0;
            printf("EDentro del segundo for\n");
            // Aplicar el filtro sobre la vecindad 3x3
            
            for (int x = -1; x <= 1; x++) {
                printf("Dentro del tercer for\n");
                for (int y = -1; y <= 1; y++) {
                    printf("Dentro del cuarto for\n");
                    int newRow = row + x;
                    int newCol = col + y;

                    printf("newRow: %d, newCol: %d\n", newRow, newCol);
                    printf("newros: %d imageIn->norm_height abs: %d || newcol: %d imageIn->header.width_px: %d\n", newRow, abs(imageIn->norm_height), newCol, imageIn->header.width_px);
                    if (newRow >= 0 && newRow < abs(imageIn->norm_height) &&
                        newCol >= 0 && newCol < width) {
                        // Acceder al píxel usando el bloque contiguo en pixels_data
                        Pixel *p = &imageIn->pixels_data[newRow * width + newCol];
                        sumBlue += filter[x + 1][y + 1] * p->blue;
                        sumGreen += filter[x + 1][y + 1] * p->green;
                        sumRed += filter[x + 1][y + 1] * p->red;
                        
                        printf("sumBlue: %d, sumGreen: %d, sumRed: %d\n", sumBlue, sumGreen, sumRed);
                    }
                }
            }

            // Limitar los valores entre 0 y 255

            Pixel *outputPixel = &imageOut->pixels_data[row * width + col];
            outputPixel->blue = (sumBlue < 0) ? 0 : (sumBlue > 255) ? 255 : sumBlue;
            outputPixel->green = (sumGreen < 0) ? 0 : (sumGreen > 255) ? 255 : sumGreen;
            outputPixel->red = (sumRed < 0) ? 0 : (sumRed > 255) ? 255 : sumRed;
            outputPixel->alpha = 255;  // Asumimos que siempre es opaco

            printf("Fila: %d, Columna: %d\n", row, col);
            printf("Blue: %d, Green: %d, Red: %d\n", outputPixel->blue, outputPixel->green, outputPixel->red);
        }
    }
    pthread_exit(NULL);
}

