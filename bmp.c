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

// Función para leer una imagen BMP completa, manejando el padding
BMP_Image* createBMPImage(FILE* fptr) {
    if (fptr == NULL) {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (image == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Leer el encabezado
    fread(&(image->header), sizeof(BMP_Header), 1, fptr);

    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    printf("Entrando al malloc de pixels_data\n");
    printf("Image size: %d x %d\n", image->header.width_px, image->norm_height);
    printf("Bits per pixel: %d\n", image->header.bits_per_pixel);
    printf("Malloc size: %d\n", abs(image->norm_height) * image->header.width_px * sizeof(Pixel));
    
    // Asignar memoria para todos los píxeles de la imagen en un bloque contiguo
    image->pixels_data = (Pixel *)malloc(abs(image->norm_height) * image->header.width_px * sizeof(Pixel));
    if (image->pixels_data == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    printf("Entrando al malloc de pixels\n");
    printf("norm height: %d\n", image->norm_height);
    printf("size of pixel: %d\n", sizeof(Pixel));
    printf("Malloc size: %d\n", abs(image->norm_height) * sizeof(Pixel *));
    
    // Asignar memoria para el doble puntero `pixels` (para las filas)
    image->pixels = (Pixel **)malloc(abs(image->norm_height) * sizeof(Pixel *));
    if (image->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < image->norm_height; i++) {
        image->pixels[i] = &image->pixels_data[i * image->header.width_px];
    }

    // Leer los datos de los píxeles desde el archivo
    for (int i = 0; i < image->norm_height; i++) {
        fread(image->pixels[i], image->bytes_per_pixel, image->header.width_px, fptr);
    }

    return image;
}


// Función para escribir una imagen BMP en un archivo, manejando el padding
void writeImage(char *destFileName, BMP_Image *dataImage) {
    FILE *destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    // Escribir el encabezado
    if (fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile) != 1) {
        printError(FILE_ERROR);
        fclose(destFile);
        exit(EXIT_FAILURE);
    }

    // Calcular el tamaño del padding
    int padding = (4 - (dataImage->header.width_px * dataImage->bytes_per_pixel) % 4) % 4;
    uint8_t paddingBytes[3] = {0, 0, 0};  // El padding es solo ceros

    // Escribir los píxeles directamente desde `pixels_data`
    for (int i = 0; i < dataImage->norm_height; i++) {
        if (fwrite(&dataImage->pixels_data[i * dataImage->header.width_px], 
                    dataImage->bytes_per_pixel, 
                    dataImage->header.width_px, 
                    destFile) != dataImage->header.width_px) {
            printError(FILE_ERROR);
            fclose(destFile);
            exit(EXIT_FAILURE);
        }
        fwrite(paddingBytes, sizeof(uint8_t), padding, destFile);  // Escribir el padding
    }

    fclose(destFile);
}


// Función para liberar la memoria de una imagen BMP
void freeImage(BMP_Image *image) {
    if (image == NULL) {
        return;
    }

    // Liberar el bloque contiguo de píxeles
    if (image->pixels_data != NULL) {
        free(image->pixels_data);
    }

    // Liberar el doble puntero
    if (image->pixels != NULL) {
        free(image->pixels);
    }

    // Finalmente, liberar la estructura de la imagen
    free(image);
}


// Función para inicializar una imagen de salida con las mismas dimensiones que la de entrada
BMP_Image *initializeImageOut(BMP_Image *imageIn) {
    // Asignar memoria para la estructura de la imagen de salida
    BMP_Image *imageOut = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imageOut == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Copiar el encabezado
    imageOut->header = imageIn->header;
    imageOut->norm_height = abs(imageIn->header.height_px);
    imageOut->bytes_per_pixel = imageIn->header.bits_per_pixel / 8;

    imageOut->pixels_data = (Pixel *)malloc(imageOut->norm_height * imageOut->header.width_px * sizeof(Pixel));
    if (imageOut->pixels_data == NULL) {
        printError(MEMORY_ERROR);
        free(imageOut);
        exit(EXIT_FAILURE);
    }

    imageOut->pixels = (Pixel **)malloc(imageOut->norm_height * sizeof(Pixel *));
    if (imageOut->pixels == NULL) {
        printError(MEMORY_ERROR);
        free(imageOut->pixels_data);  // Liberar la memoria contigua si falla aquí
        free(imageOut);
        exit(EXIT_FAILURE);
    }

    // Inicializar el doble puntero `pixels` para que apunte a las filas dentro de `pixels_data`
    for (int i = 0; i < imageOut->norm_height; i++) {
        imageOut->pixels[i] = &imageOut->pixels_data[i * imageOut->header.width_px];
    }

    return imageOut;
}

