#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bmp.h"
#include "filter.h"

// Función para imprimir errores
void printError(int error)
{
    switch (error)
    {
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

BMP_Image *createBMPImage(FILE *fptr)
{
    if (fptr == NULL)
    {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    printf("Entrando al malloc de image\n");
    BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
    printf("BMP Image size disponible desde 0: %d\n", sizeof(image));

    if (image == NULL)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    fread(&(image->header), sizeof(BMP_Header), 1, fptr);

    image->norm_height = abs(image->header.height_px);
    image->bytes_per_pixel = image->header.bits_per_pixel / 8;

    printf("Header information:\n");
    printf("  Width: %d px\n", image->header.width_px);
    printf("  Height: %d px\n", image->header.height_px);
    printf("  Bits per pixel: %d\n", image->header.bits_per_pixel);
    printf("  File size: %d bytes\n", image->header.size);

    if (image->norm_height <= 0 || image->header.width_px <= 0)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }
    image->pixels_data = (Pixel *)malloc(image->norm_height * image->header.width_px * sizeof(Pixel));

    if (image->pixels_data == NULL)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    image->pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
    if (image->pixels == NULL)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Inicializar los punteros del doble puntero a las filas
    for (int i = 0; i < image->norm_height; i++)
    {
        image->pixels[i] = &image->pixels_data[i * image->header.width_px];
    }

    // Leer los datos de los píxeles desde el archivo
    for (int i = 0; i < image->norm_height; i++)
    {
        fread(image->pixels[i], image->bytes_per_pixel, image->header.width_px, fptr);
    }

    return image;
}

// Función para escribir una imagen BMP en un archivo
void writeImage(char *destFileName, BMP_Image *dataImage)
{
    FILE *destFile = fopen(destFileName, "wb");
    if (destFile == NULL)
    {
        printError(FILE_ERROR);
        exit(EXIT_FAILURE);
    }

    // Escribir el encabezado
    if (fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile) != 1)
    {
        printError(FILE_ERROR);
        fclose(destFile);
        exit(EXIT_FAILURE);
    }

    // Calcular el tamaño del padding (relleno)
    int width = dataImage->header.width_px;
    int height = abs(dataImage->header.height_px); // Usar siempre positivo
    int bytesPerPixel = dataImage->bytes_per_pixel;
    int padding = (4 - (width * bytesPerPixel) % 4) % 4;
    uint8_t paddingBytes[3] = {0, 0, 0}; // El padding es solo ceros

    // Escribir los píxeles directamente desde el bloque de memoria contigua
    Pixel *pixels = dataImage->pixels_data;

    for (int i = 0; i < height; i++)
    { // La imagen BMP empieza desde la primera fila
        // Escribir la fila de píxeles completa
        Pixel *pixels = &dataImage->pixels_data[i * width];

        if (pixels == NULL)
        {
            printError(MEMORY_ERROR);
            fclose(destFile);
            exit(EXIT_FAILURE);
        }

        printf("Escribiendo fila %d con %d píxeles\n", i, width);

        if (fwrite(pixels, bytesPerPixel, width, destFile) != width)
        {
            printError(FILE_ERROR);
            fclose(destFile);
            exit(EXIT_FAILURE);
        }

        // Verificar el padding
        if (padding > 0)
        {
            if (fwrite(paddingBytes, 1, padding, destFile) != padding)
            {
                printError(FILE_ERROR);
                fclose(destFile);
                exit(EXIT_FAILURE);
            }
        }
    }

    fclose(destFile);
}

// Función para liberar la memoria de una imagen BMP
void freeImage(BMP_Image *image)
{
    if (image == NULL)
    {
        return;
    }

    // Liberar el bloque contiguo de píxeles
    if (image->pixels_data != NULL)
    {
        free(image->pixels_data);
    }

    // Liberar el doble puntero
    if (image->pixels != NULL)
    {
        free(image->pixels);
    }

    // Finalmente, liberar la estructura de la imagen
    free(image);
}

// Función para inicializar una imagen de salida con las mismas dimensiones que la de entrada
BMP_Image *initializeImageOut(BMP_Image *imageIn)
{
    // Asignar memoria para la estructura de la imagen de salida
    BMP_Image *imageOut = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imageOut == NULL)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Copiar el encabezado
    imageOut->header = imageIn->header;
    imageOut->norm_height = abs(imageIn->header.height_px);
    imageOut->bytes_per_pixel = imageIn->header.bits_per_pixel / 8;

    imageOut->pixels_data = (Pixel *)malloc(imageOut->norm_height * imageOut->header.width_px * sizeof(Pixel));
    if (imageOut->pixels_data == NULL)
    {
        printError(MEMORY_ERROR);
        free(imageOut);
        exit(EXIT_FAILURE);
    }

    imageOut->pixels = (Pixel **)malloc(imageOut->norm_height * sizeof(Pixel *));
    if (imageOut->pixels == NULL)
    {
        printError(MEMORY_ERROR);
        free(imageOut->pixels_data); // Liberar la memoria contigua si falla aquí
        free(imageOut);
        exit(EXIT_FAILURE);
    }

    // Inicializar el doble puntero `pixels` para que apunte a las filas dentro de `pixels_data`
    for (int i = 0; i < imageOut->norm_height; i++)
    {
        imageOut->pixels[i] = &imageOut->pixels_data[i * imageOut->header.width_px];
    }

    return imageOut;
}
