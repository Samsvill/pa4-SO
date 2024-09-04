#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"
#include "filter.h"

#define SHM_SIZE sizeof(BMP_Image)
#define FILTER_SIZE 3

// Estructura para pasar los argumentos a cada hilo
typedef struct
{
    int startRow;
    int endRow;
    BMP_Image *imageIn;
    BMP_Image *imageOut;
} ThreadArgs;



// Función que aplica el filtro de realce a una parte de la imagen
void *applyEdgeEnhance(void *args)
{
    ThreadArgs *tArgs = (ThreadArgs *)args;
    int startRow = tArgs->startRow;
    int endRow = tArgs->endRow;
    BMP_Image *imageIn = tArgs->imageIn;
    BMP_Image *imageOut = tArgs->imageOut;

    // Filtro de realce de bordes
    int edgeEnhanceFilter[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 1, 1}};
    
    printf("filtro: Ancho de la imagen: %d\n", imageIn->header.width_px);
    printf("filtro: Alto de la imagen: %d\n", imageIn->header.height_px);
    printf("filtro: Inicio de la fila: %d\n", startRow);
    printf("filtro: Fin de la fila: %d\n", endRow);
    printf("filtro -------------------- aqui se entra al proceso de filtrado\n");
    for (int row = startRow; row < endRow; row++)
    {
        for (int col = 0; col < imageIn->header.width_px; col++)
        {
            int sumBlue = 0, sumGreen = 0, sumRed = 0, sumAlpha = 0;
            printf("Fila %d, Columna %d\n", row, col);
            for (int x = -1; x <= 1; x++)
            {
                printf("x: %d\n", x);
                for (int y = -1; y <= 1; y++)
                {
                    printf("y: %d\n", y);
                    int newRow = row + x;
                    int newCol = col + y;

                    if (newRow >= 0 && newRow < imageIn->header.height_px && 
                        newCol >= 0 && newCol < imageIn->header.width_px)
                    {
                        printf("edgeEnhanceFilter[%d][%d]: %d\n", x + 1, y + 1, edgeEnhanceFilter[x + 1][y + 1]);
                        printf("blue, green y red antes de multiplicar: %d, %d, %d\n", imageIn->pixels[newRow][newCol].blue, imageIn->pixels[newRow][newCol].green, imageIn->pixels[newRow][newCol].red);
                        printf("haciendo la multiplicacion\n");
                        sumBlue += edgeEnhanceFilter[x + 1][y + 1] * imageIn->pixels[newRow][newCol].blue;
                        sumGreen += edgeEnhanceFilter[x + 1][y + 1] * imageIn->pixels[newRow][newCol].green;
                        sumRed += edgeEnhanceFilter[x + 1][y + 1] * imageIn->pixels[newRow][newCol].red;
                        printf("blue, green y red despues de multiplicar: %d, %d, %d\n", sumBlue, sumGreen, sumRed);
                    }
                }
            }

            // Normalizar el valor de los píxeles y asegurarse de que estén entre 0 y 255
            imageOut->pixels[row][col].blue = (sumBlue / 9);
            imageOut->pixels[row][col].green = (sumGreen / 9);
            imageOut->pixels[row][col].red = (sumRed / 9);
            imageOut->pixels[row][col].alpha = (255);

            // Imprimir los valores procesados
        }
    }
    pthread_exit(NULL);
}

// Función principal
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Uso: %s <ruta_imagen> <número_hilos>\n", argv[0]);
        return 1;
    }

    // Cargar la imagen desde el archivo
    char *inputFile = argv[1];
    int numThreads = atoi(argv[2]);

    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL)
    {
        perror("Error al abrir el archivo de imagen");
        return 1;
    }

    BMP_Image *imageIn = createBMPImage(imageFile);
    if (imageIn == NULL)
    {
        printError(FILE_ERROR);
        return 1;
    }
    fclose(imageFile);

    // Crear la imagen de salida con las mismas dimensiones
    BMP_Image *imageOut = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imageOut == NULL)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }
    
    imageOut->header = imageIn->header;
    imageOut->norm_height = abs(imageIn->header.height_px);
    imageOut->bytes_per_pixel = imageIn->header.bits_per_pixel/8;
    imageOut->pixels = (Pixel **)malloc(imageOut->norm_height * sizeof(Pixel *));

    if (imageOut->pixels == NULL)
    {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
        
    }

    for (int i = 0; i < imageOut->norm_height; i++)
    {
        imageOut->pixels[i] = (Pixel *)malloc(imageOut->header.width_px * sizeof(Pixel));
        if (imageOut->pixels[i] == NULL)
        {
            printError(MEMORY_ERROR);
            exit(EXIT_FAILURE);
        }
    }

    //Crear y lanzar los hilos para procesar la imagen en paralelo
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = imageIn->header.height_px / numThreads;
    int remainingRows = imageIn->header.height_px % numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? abs(imageIn->header.height_px) : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;

        printf("Hilo %d: Procesando desde fila %d hasta fila %d\n", i, threadArgs[i].startRow, threadArgs[i].endRow);
        pthread_create(&threads[i], NULL, applyEdgeEnhance, &threadArgs[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    //Guardar la imagen resultante
    char outputFile[] = "output_enhanced.bmp";
    writeImage(outputFile, imageOut);


    printf("Imagen con filtro de realce guardada en: %s\n", outputFile);

    // Liberar la memoria
    freeImage(imageIn);
    freeImage(imageOut);

    return 0;
}
