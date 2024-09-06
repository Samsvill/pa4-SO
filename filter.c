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
    int needsNormalization = (filterSum != 0);  // Solo normalizamos si el filtro lo requiere

    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < width; col++) {
            int sumBlue = 0, sumGreen = 0, sumRed = 0;

            // Aplicar el filtro sobre la vecindad 3x3
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    int newRow = row + x;
                    int newCol = col + y;

                    if (newRow >= 0 && newRow < abs(imageIn->norm_height) &&
                        newCol >= 0 && newCol < width) {
                        Pixel *p = &imageIn->pixels[newRow][newCol];
                        sumBlue += filter[x + 1][y + 1] * p->blue;
                        sumGreen += filter[x + 1][y + 1] * p->green;
                        sumRed += filter[x + 1][y + 1] * p->red;
                    }
                }
            }

            // Normalizar solo si es necesario
            if (needsNormalization) {
                sumBlue /= filterSum;
                sumGreen /= filterSum;
                sumRed /= filterSum;
            }

            // Limitar los valores entre 0 y 255
            imageOut->pixels[row][col].blue = (sumBlue < 0) ? 0 : (sumBlue > 255) ? 255 : sumBlue;
            imageOut->pixels[row][col].green = (sumGreen < 0) ? 0 : (sumGreen > 255) ? 255 : sumGreen;
            imageOut->pixels[row][col].red = (sumRed < 0) ? 0 : (sumRed > 255) ? 255 : sumRed;
            imageOut->pixels[row][col].alpha = 255;  // Asumimos que siempre es opaco
        }
    }
    pthread_exit(NULL);
}
