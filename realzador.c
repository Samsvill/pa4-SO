#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "bmp.h"
#include "filter.h"

#define SHM_KEY 1234  // Clave para la memoria compartida
#define SEM_KEY 5678  // Clave para el semáforo

// Función auxiliar para operar con el semáforo
void waitSemaphore(int semid) {
    struct sembuf sops = {0, -1, 0};  // Operación: restar 1
    semop(semid, &sops, 1);           // Esperar a que el semáforo esté en 0
}

void signalSemaphore(int semid) {
    struct sembuf sops = {0, 1, 0};   // Operación: sumar 1
    semop(semid, &sops, 1);           // Indicar que el recurso está disponible
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <número_hilos>\n", argv[0]);
        return 1;
    }

    int numThreads = atoi(argv[1]);

    // Acceder a la memoria compartida
    int shmid = shmget(SHM_KEY, sizeof(BMP_Image), 0666);
    if (shmid < 0) {
        perror("Error al acceder a la memoria compartida");
        exit(1);
    }

    BMP_Image *sharedImage = (BMP_Image *)shmat(shmid, NULL, 0);
    if (sharedImage == (BMP_Image *)-1) {
        perror("Error al adjuntar la memoria compartida");
        exit(1);
    }

    // Acceder al semáforo
    int semid = semget(SEM_KEY, 1, 0666);
    if (semid == -1) {
        perror("Error al acceder al semáforo");
        exit(1);
    }

    // Esperar a que el publicador termine de cargar la imagen
    waitSemaphore(semid);

    BMP_Image *imageOut = initializeImageOut(sharedImage);  // Crear imagen de salida en memoria local

    // Crear hilos para aplicar el filtro
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = abs(sharedImage->header.height_px) / (2 * numThreads);  // Solo procesamos la mitad inferior

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = sharedImage->norm_height / 2 + i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? sharedImage->norm_height : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = sharedImage;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].filter = simplifiedSobelFilter;
        //printf("Hilo %d: Procesando desde fila %d hasta fila %d\n", i, threadArgs[i].startRow, threadArgs[i].endRow);
        pthread_create(&threads[i], NULL, applyFilter, &threadArgs[i]);
    }


    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Escribir la mitad procesada de vuelta a la memoria compartida
    for (int i = sharedImage->norm_height / 2; i < sharedImage->norm_height; i++) {
        memcpy(sharedImage->pixels[i], imageOut->pixels[i], imageOut->header.width_px * sizeof(Pixel));
    }

    // Liberar la memoria compartida
    shmdt(sharedImage);

    // Notificar que el realzador ha terminado
    signalSemaphore(semid);

    // Liberar la memoria de la imagen de salida
    freeImage(imageOut);

    return 0;
}
