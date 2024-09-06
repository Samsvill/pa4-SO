#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include "bmp.h"

// Función principal del publicador
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <ruta_imagen>\n", argv[0]);
        return 1;
    }

    char *inputFile = argv[1];

    // Abrir la imagen BMP desde el archivo
    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL) {
        printError(FILE_ERROR);
        return 1;
    }

    // Crear estructura BMP y cargar la imagen
    BMP_Image *imageIn = createBMPImage(imageFile);
    fclose(imageFile);

    // Calcular el tamaño total de la imagen (encabezado + datos de píxeles)
    size_t total_image_size = imageIn->norm_height * imageIn->header.width_px * sizeof(Pixel);

    // Crear memoria compartida para almacenar la imagen BMP completa
    int shm_id = shmget(IPC_PRIVATE, sizeof(BMP_Image) + total_image_size, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Adjuntar la memoria compartida al proceso
    void *shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Error al adjuntar la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Copiar el encabezado BMP a la memoria compartida
    memcpy(shm_ptr, imageIn, sizeof(BMP_Image));

    // Copiar los datos de los píxeles a la memoria compartida (después del encabezado)
    void *pixel_ptr = shm_ptr + sizeof(BMP_Image);
    for (int i = 0; i < imageIn->norm_height; i++) {
        memcpy(pixel_ptr + (i * imageIn->header.width_px * sizeof(Pixel)),
               imageIn->pixels[i],
               imageIn->header.width_px * sizeof(Pixel));
    }

    // Ahora leeremos la imagen de vuelta desde la memoria compartida para verificar que se cargó bien

    // Crear una nueva estructura BMP para almacenar la imagen leída de la memoria compartida
    BMP_Image *imageOut = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imageOut == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Copiar el encabezado BMP desde la memoria compartida
    memcpy(imageOut, shm_ptr, sizeof(BMP_Image));

    // Asignar memoria para los píxeles de la imagen leída
    imageOut->pixels = (Pixel **)malloc(imageOut->norm_height * sizeof(Pixel *));
    for (int i = 0; i < imageOut->norm_height; i++) {
        imageOut->pixels[i] = (Pixel *)malloc(imageOut->header.width_px * sizeof(Pixel));
        memcpy(imageOut->pixels[i], pixel_ptr + (i * imageOut->header.width_px * sizeof(Pixel)),
               imageOut->header.width_px * sizeof(Pixel));
    }

    // Guardar la imagen leída de la memoria compartida en un archivo de salida
    writeImage("output_shared_memory.bmp", imageOut);

    // Imprimir el ID de la memoria compartida para que otros procesos puedan acceder
    printf("ID de memoria compartida: %d\n", shm_id);

    // Liberar la memoria local
    freeImage(imageIn);
    freeImage(imageOut);

    // Separar el segmento de memoria compartida del proceso
    shmdt(shm_ptr);

    return 0;
}
