#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bmp.h"
#include "filter.h"

void printError(int error) {
  switch (error) {
    case ARGUMENT_ERROR:
      printf("Usage: ex5 <source> <destination>\n");
      break;
    case FILE_ERROR:
      printf("Unable to open file!\n");
      break;
    case MEMORY_ERROR:
      printf("Unable to allocate memory!\n");
      break;
    case VALID_ERROR:
      printf("BMP file not valid!\n");
      break;
    default:
      break;
  }
}

void readImageData(FILE* srcFile, BMP_Image* image, int dataSize) {
  
  int bytesPorPixel = image->header.bits_per_pixel / 8;
  int paddingSize = (4 - (image->header.width_px * bytesPorPixel) % 4) % 4;
  uint8_t padding[3] = {0};

  for (int i = 0; i < image->norm_height; i++) {
    fread(image->pixels[i], bytesPorPixel, image->header.width_px, srcFile);
    fread(padding, sizeof(uint8_t), paddingSize, srcFile);  
  }
}


void readImage(FILE* srcFile, BMP_Image* dataImage) {
  readImageData(srcFile, dataImage, dataImage->header.width_px * dataImage->norm_height);
}


BMP_Image* createBMPImage(FILE *fptr) {
    if (fptr == NULL) {
        fprintf(stderr, "Error: Puntero de archivo nulo en createBMPImage\n");
        return NULL;
    }

    BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (image == NULL) {
        fprintf(stderr, "Error al asignar memoria para BMP_Image\n");
        return NULL;
    }

    // Leer el encabezado de la imagen
    fread(&image->header, sizeof(BMP_Header), 1, fptr);
    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    // Asignar memoria para los píxeles
    image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
    if (image->pixels == NULL) {
        fprintf(stderr, "Error al asignar memoria para los píxeles\n");
        free(image);
        return NULL;
    }

    int padding = (4 - (image->header.width_px * sizeof(Pixel)) % 4) % 4;

    for (int i = 0; i < image->norm_height; i++) {
        image->pixels[i] = (Pixel *)malloc(image->header.width_px * sizeof(Pixel));
        if (image->pixels[i] == NULL) {
            fprintf(stderr, "Error al asignar memoria para la fila de píxeles %d\n", i);
            for (int j = 0; j < i; j++) {
                free(image->pixels[j]);
            }
            free(image->pixels);
            free(image);
            return NULL;
        }
    fread(image->pixels[i], sizeof(Pixel), image->header.width_px, fptr);
    fseek(fptr, padding, SEEK_CUR);
    }

    return image;
}



void writeImage(char* destFileName, BMP_Image* dataImage) {
  FILE* destFile = fopen(destFileName, "wb");
  if (destFile == NULL) {
    printError(FILE_ERROR);
    exit(EXIT_FAILURE);
  }

  // Escribir el encabezado en el archivo de destino
  fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile);

  // Calcular el tamaño del padding
  int paddingSize = (4 - (dataImage->header.width_px * dataImage->bytes_per_pixel) % 4) % 4;

  // Escribir datos de imagen fila por fila con padding
  for (int i = 0; i < dataImage->norm_height; i++) {
    fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, destFile);
    for (int j = 0; j < paddingSize; j++) {
      fputc(0x00, destFile);
    }
  }
  
  fclose(destFile);
}

void freeImage(BMP_Image* image) {
  if (image == NULL) {
    return;
  }

  // Se libera la memoria para cada fila de píxeles
  for (int i = 0; i < image->norm_height; i++) {
    free(image->pixels[i]);
  }

  free(image->pixels);

  // Se libera memoria para BMP_Image
  free(image);
}

int checkBMPValid(BMP_Header* header) {
  
  if (header->type != 0x4d42) {
    return FALSE;
  }
  
  if (header->bits_per_pixel != 32) {
    return FALSE;
  }
  
  if (header->planes != 1) {
    return FALSE;
  }
  
  if (header->compression != 0) {
    return FALSE;
  }
  return TRUE;
}

void printBMPHeader(BMP_Header* header) {
  printf("file type (should be 0x4d42): %x\n", header->type);
  printf("file size: %d\n", header->size);
  printf("offset to image data: %d\n", header->offset);
  printf("header size: %d\n", header->header_size);
  printf("width_px: %d\n", header->width_px);
  printf("height_px: %d\n", header->height_px);
  printf("planes: %d\n", header->planes);
  printf("bits: %d\n", header->bits_per_pixel);
}

void printBMPImage(BMP_Image* image) {
  printf("data size is %ld\n", sizeof(image->pixels));
  printf("norm_height size is %d\n", image->norm_height);
  printf("bytes per pixel is %d\n", image->bytes_per_pixel);
}
