#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include "bmp.h"

#define SHM_KEY 1234  // Clave para la memoria compartida
#define SEM_KEY 5678  // Clave para el semáforo

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <ruta_imagen>\n", argv[0]);
        return 1;
    }

    // Crear o acceder a la memoria compartida
    int shmid = shmget(SHM_KEY, sizeof(BMP_Image), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("Error al crear/acceder a la memoria compartida");
        exit(1);
    }

    // Adjuntar la memoria compartida
    BMP_Image *sharedImage = (BMP_Image *)shmat(shmid, NULL, 0);
    if (sharedImage == (BMP_Image *)-1) {
        perror("Error al adjuntar la memoria compartida");
        exit(1);
    }

    // Cargar la imagen desde el archivo
    char *inputFile = argv[1];
    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL) {
        printError(FILE_ERROR);
        exit(1);
    }

    BMP_Image *image = createBMPImage(imageFile);
    fclose(imageFile);

    // Copiar la imagen a la memoria compartida
    *sharedImage = *image;  // Copiar el encabezado
    for (int i = 0; i < image->norm_height; i++) {
        memcpy(sharedImage->pixels[i], image->pixels[i], image->header.width_px * sizeof(Pixel));
    }

    // Crear un semáforo
    int semid = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("Error al crear el semáforo");
        exit(1);
    }

    // Inicializar el semáforo a 1 (imagen lista)
    semctl(semid, 0, SETVAL, 1);

    // Imprimir la imagen para verificar
    printf("Imagen cargada en memoria compartida. Esperando a que el realzador procese...\n");

    // Esperar (el publicador se queda "vivo" mientras los otros procesos terminan)
    pause();

    // Liberar la memoria compartida
    shmdt(sharedImage);
    shmctl(shmid, IPC_RMID, NULL);  // Eliminar memoria compartida al terminar
    semctl(semid, 0, IPC_RMID);  // Eliminar el semáforo

    return 0;
}
