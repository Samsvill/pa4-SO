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
    if (fread(&(image->header), sizeof(BMP_Header), 1, fptr) != 1) {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }
    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    // Calcular el tamaño del padding (si el ancho no es múltiplo de 4)
    int padding = (4 - (image->header.width_px * image->bytes_per_pixel) % 4) % 4;

    // Asignar memoria contigua para todos los píxeles
    image->pixels = (Pixel *)malloc(image->norm_height * image->header.width_px * sizeof(Pixel));
    if (image->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Leer los píxeles, fila por fila, manejando el padding
    for (int i = 0; i < image->norm_height; i++) {
        if (fread(&image->pixels[i * image->header.width_px],
                image->bytes_per_pixel, 
                image->header.width_px, 
                fptr)
                != image->header.width_px) {
            printError(FILE_ERROR);
            exit(EXIT_FAILURE);
        }
        fseek(fptr, padding, SEEK_CUR);  // Saltar el padding al final de cada fila
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

    // Escribir los píxeles fila por fila, agregando el padding necesario
    for (int i = 0; i < dataImage->norm_height; i++) {
        if (fwrite(&dataImage->pixels[i * dataImage->header.width_px], 
                    dataImage->bytes_per_pixel, dataImage->header.width_px, 
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

    // Liberar la memoria de los píxeles
    if (image->pixels != NULL) {
        free(image->pixels);
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

    // Copiar el encabezado
    imageOut->header = imageIn->header;
    imageOut->norm_height = abs(imageIn->header.height_px);
    imageOut->bytes_per_pixel = imageIn->header.bits_per_pixel / 8;

    // Asignar memoria contigua para los píxeles
    imageOut->pixels = (Pixel *)malloc(imageOut->norm_height * imageOut->header.width_px * sizeof(Pixel));
    if (imageOut->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    return imageOut;
}
