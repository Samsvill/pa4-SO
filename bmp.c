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
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    BMP_Image* image = (BMP_Image*)malloc(sizeof(BMP_Image));
    if (image == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Leer el encabezado de la imagen
    fread(&(image->header), sizeof(BMP_Header), 1, fptr);
    image->header.size = image->header.width_px * image->header.height_px * sizeof(Pixel) + sizeof(image->header);
    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    //MEMORIA PARA PIXELS
    image->pixels = (Pixel**)malloc(image->norm_height * sizeof(Pixel*));
    if (image->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }
    //MEMORIA PARA CADA FILA DE PIXELS
    for (int i = 0; i < image->norm_height; i++) {
        image->pixels[i] = (Pixel *)malloc(image->header.width_px * sizeof(Pixel));
        if (image->pixels[i] == NULL) {
            printError(MEMORY_ERROR);
            exit(EXIT_FAILURE);
        }
        fread(image->pixels[i], image->bytes_per_pixel, image->header.width_px, fptr);
        for (int j = 0; j < image->header.width_px; j++) {
            printf("Fila %d, Columna %d - R: %d, G: %d, B: %d\n", i, j, 
          image->pixels[i][j].red, 
          image->pixels[i][j].green, 
          image->pixels[i][j].blue);
        }
}
    
    printf("Fila %d, Columna %d - R: %d, G: %d, B: %d\n", 899, 599, 
          image->pixels[899][599].red, 
          image->pixels[899][599].green, 
          image->pixels[899][599].blue);

    //readImage(fptr, image);

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


  // Escribir datos de imagen fila por fila
  for (int i = 0; i < dataImage->norm_height; i++) {
    if (dataImage->pixels[i] == NULL){
      printError(MEMORY_ERROR);
      exit(EXIT_FAILURE);
    }
    fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, destFile);
    
    // imprimir los valores de los píxeles escritos
    printf("Write Image\n");
    for (int j = 0; j < dataImage->header.width_px; j++) {
      printf("Fila %d, Columna %d - R: %d, G: %d, B: %d\n", i, j, 
          dataImage->pixels[i][j].red, 
          dataImage->pixels[i][j].green, 
          dataImage->pixels[i][j].blue);
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
    if (image->pixels[i] != NULL) {
      free(image->pixels[i]);
    }
  }

  if (image->pixels != NULL) {
    free(image->pixels);
    image->pixels = NULL;
  }

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
