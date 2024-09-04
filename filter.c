#include "filter.h"
#include "bmp.h"
#include <pthread.h>
#include <stdio.h>

void apply(BMP_Image *imageIn, BMP_Image *imageOut) {
  int boxFilter[3][3] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
  int numThreads = 100;
  applyParallel(imageIn, imageOut, boxFilter, numThreads);
}

void *filterThreadWorker(void *args) {  
  FilterThreadArgs *threadArgs = (FilterThreadArgs *)args;

  BMP_Image *imageIn = threadArgs->imageIn;
  BMP_Image *imageOut = threadArgs->imageOut;
  int tid = threadArgs->tid;
  int numThreads = threadArgs->numThreads;

  int calculo = (imageOut->norm_height + numThreads - 1);
  int zona = calculo / numThreads;

  int p1 = zona * tid;
  int suma = p1 + zona;

  if (suma > imageOut->norm_height)
    suma = imageOut->norm_height;

  int cBlue, cGreen, cRed, cAlpha;

  for (int k = p1; k < suma; k++) {
    for (int w = 0; w < imageIn->header.width_px; w++) {
      cBlue = 0;
      cGreen = 0;
      cRed = 0;
      cAlpha = 0;

      printf("[%d][%d]\n", k, w);
      for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
          if ((k - 1 + x < 0) || (k - 1 + x >= imageOut->norm_height) ||
              (w - 1 + y < 0) || (w - 1 + y >= imageOut->header.width_px)) {
            cBlue = cBlue + threadArgs->filter[x][y] * imageIn->pixels[k][w].blue;
            cAlpha = cAlpha + threadArgs->filter[x][y] * imageIn->pixels[k][w].alpha;
            cRed = cRed + threadArgs->filter[x][y] * imageIn->pixels[k][w].red;
            cGreen = cGreen + threadArgs->filter[x][y] *  imageIn->pixels[k][w].green;
          } else {
            cBlue = cBlue + threadArgs->filter[x][y] * imageIn->pixels[k - 1 + x][w - 1 + y].blue;
            cAlpha = cAlpha + threadArgs->filter[x][y] * imageIn->pixels[k - 1 + x][w - 1 + y].alpha;
            cRed = cRed + threadArgs->filter[x][y] * imageIn->pixels[k - 1 + x][w - 1 + y].red;
            cGreen = cGreen + threadArgs->filter[x][y] * imageIn->pixels[k - 1 + x][w - 1 + y].green;
          }
        }
      }
      cBlue = cBlue / (int)threadArgs->norm;
      cAlpha = cAlpha / (int)threadArgs->norm;
      cRed = cRed / (int)threadArgs->norm;
      cGreen = cGreen / (int)threadArgs->norm;

      cBlue = (cBlue > 255) ? 255: (cBlue < 0)? 0 : cBlue;
      cAlpha = (cAlpha > 255) ? 255: (cAlpha < 0)? 0 : cAlpha;
      cRed = (cRed > 255) ? 255: (cRed < 0)? 0 : cRed;
      cGreen = (cGreen > 255) ? 255: (cGreen < 0)? 0 : cGreen;

      imageOut->pixels[k][w].blue = (uint8_t)cBlue;
      imageOut->pixels[k][w].alpha = (uint8_t)cAlpha;
      imageOut->pixels[k][w].red = (uint8_t)cRed;
      imageOut->pixels[k][w].green = (uint8_t)cGreen;
      

    }
  }

  return NULL;
}

void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int boxFilter[3][3], int numThreads) {

  pthread_t *threads = malloc(numThreads * sizeof(pthread_t));

  FilterThreadArgs *threadArgs = malloc(numThreads * sizeof(FilterThreadArgs));

  int rowsPerThread = imageOut->norm_height / numThreads;

  for (int i = 0; i < numThreads; i++) {
    threadArgs[i].imageIn = imageIn;
    threadArgs[i].imageOut = imageOut;
    threadArgs[i].filter = boxFilter;
    threadArgs[i].numThreads = numThreads;
    threadArgs[i].tid = i;
    threadArgs[i].norm = 9;

    int startRow = i * rowsPerThread;
    int endRow = (i == numThreads - 1) ? imageOut->norm_height : startRow + rowsPerThread;

    threadArgs[i].startRow = startRow;
    threadArgs[i].endRow = endRow;

    pthread_create(&threads[i], NULL, filterThreadWorker, (void *)&threadArgs[i]);
  }

  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }

  free(threads);
  free(threadArgs);
}
