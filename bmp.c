#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bmp.h"
#include "filter.h"

// Función para imprimir errores
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

// Función para leer los datos de imagen de un archivo BMP
void readImageData(FILE *srcFile, BMP_Image *image, int dataSize) {
    int bytesPorPixel = image->header.bits_per_pixel / 8;
    int paddingSize = (4 - (image->header.width_px * bytesPorPixel) % 4) % 4;
    uint8_t padding[3] = {0};

    for (int i = 0; i < image->norm_height; i++) {
        fread(image->pixels[i], bytesPorPixel, image->header.width_px, srcFile);
        fread(padding, sizeof(uint8_t), paddingSize, srcFile);
    }
}

// Función para leer una imagen BMP completa
BMP_Image *createBMPImage(FILE *fptr) {
    if (fptr == NULL) {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (image == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Leer el encabezado de la imagen
    fread(&(image->header), sizeof(BMP_Header), 1, fptr);
    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    // Asignar memoria para los píxeles
    image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
    if (image->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Leer los píxeles
    for (int i = 0; i < image->norm_height; i++) {
        image->pixels[i] = (Pixel *)malloc(image->header.width_px * sizeof(Pixel));
        if (image->pixels[i] == NULL) {
            printError(MEMORY_ERROR);
            exit(EXIT_FAILURE);
        }
        fread(image->pixels[i], image->bytes_per_pixel, image->header.width_px, fptr);
    }

    return image;
}

// Función para escribir una imagen BMP en un archivo
void writeImage(char *destFileName, BMP_Image *dataImage) {
    FILE *destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    // Escribir el encabezado
    fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile);

    // Escribir los datos de imagen
    for (int i = 0; i < dataImage->norm_height; i++) {
        fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px, destFile);
    }

    fclose(destFile);
}

// Función para liberar la memoria de una imagen BMP
void freeImage(BMP_Image *image) {
    if (image == NULL) {
        return;
    }

    // Liberar la memoria de cada fila de píxeles
    for (int i = 0; i < image->norm_height; i++) {
        if (image->pixels[i] != NULL) {
            free(image->pixels[i]);
        }
    }

    if (image->pixels != NULL) {
        free(image->pixels);
        image->pixels = NULL;
    }

    free(image);
}

// Función para inicializar una imagen de salida con las mismas dimensiones que la de entrada
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
