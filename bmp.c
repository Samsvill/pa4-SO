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
BMP_Image *createBMPImage(FILE *fptr, void *shared_mem) {
    if (fptr == NULL || shared_mem == NULL) {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    BMP_Image *image = (BMP_Image *)shared_mem;  // El encabezado se almacena en la memoria compartida
    fread(&(image->header), sizeof(BMP_Header), 1, fptr);

    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    // Puntero a la memoria contigua para almacenar todos los píxeles
    image->pixels_data = (Pixel *)(shared_mem + sizeof(BMP_Image));

    // Asignar el puntero doble a las filas
    image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
    if (image->pixels == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Apuntar cada fila a la parte correspondiente en el bloque contiguo
    for (int i = 0; i < image->norm_height; i++) {
        image->pixels[i] = &image->pixels_data[i * image->header.width_px];
        fread(image->pixels[i], image->bytes_per_pixel, image->header.width_px, fptr);  // Leer cada fila
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
    //int padding = (4 - (dataImage->header.width_px * dataImage->bytes_per_pixel) % 4) % 4;
    //uint8_t paddingBytes[3] = {0, 0, 0};  // El padding es solo ceros

    // Escribir los píxeles fila por fila, agregando el padding necesario
    for (int i = 0; i < dataImage->norm_height; i++) {
        if (fwrite(&dataImage->pixels[i * abs(dataImage->header.width_px)], 
                    dataImage->bytes_per_pixel,
                    dataImage->header.width_px, 
                    destFile) != dataImage->header.width_px) {
            printError(FILE_ERROR);
            fclose(destFile);
            exit(EXIT_FAILURE);
        }
        //fwrite(paddingBytes, sizeof(uint8_t), padding, destFile);  // Escribir el padding
    }

    fclose(destFile);
}


// Función para liberar la memoria de una imagen BMP
void freeImage(BMP_Image *image) {
    if (image == NULL) {
        return;
    }

    // Solo se libera el puntero de punteros (no la memoria compartida)
    if (image->pixels != NULL) {
        free(image->pixels);
    }
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
