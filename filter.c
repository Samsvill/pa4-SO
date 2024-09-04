#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Definición del filtro de realce de bordes (Edge Enhance)
int edgeEnhanceFilter[3][3] = {
    {-1, -1, -1},
    {-1, 9, -1},
    {-1, -1, -1}
};


int blurFilter[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}
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
    int (*filter)[3] = tArgs->filter;  // Filtro que se aplicará

    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < imageIn->header.width_px; col++) {
            int sumBlue = 0, sumGreen = 0, sumRed = 0;

            // Aplicar el filtro sobre la vecindad 3x3
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    int newRow = row + x;
                    int newCol = col + y;

                    if (newRow >= 0 && newRow < abs(imageIn->header.height_px) &&
                        newCol >= 0 && newCol < imageIn->header.width_px) {
                        Pixel *p = &imageIn->pixels[newRow][newCol];
                        sumBlue += filter[x + 1][y + 1] * p->blue;
                        sumGreen += filter[x + 1][y + 1] * p->green;
                        sumRed += filter[x + 1][y + 1] * p->red;
                    }
                }
            }

            // Normalizar el valor de los píxeles
            imageOut->pixels[row][col].blue = (sumBlue < 0) ? 0 : (sumBlue > 255) ? 255 : sumBlue;
            imageOut->pixels[row][col].green = (sumGreen < 0) ? 0 : (sumGreen > 255) ? 255 : sumGreen;
            imageOut->pixels[row][col].red = (sumRed < 0) ? 0 : (sumRed > 255) ? 255 : sumRed;
            imageOut->pixels[row][col].alpha = 255;  // Asumimos que siempre es opaco
        }
    }
    pthread_exit(NULL);
}
