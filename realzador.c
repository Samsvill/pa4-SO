#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"
#include "filter.h"

#define SHM_KEY 1234  // Clave para la memoria compartida

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <ruta_salida> <número_hilos>\n", argv[0]);
        return 1;
    }

    int numThreads = atoi(argv[2]);
    char *outputFile = argv[1];  // Usar el archivo de salida que se pasa como argumento

    // Acceder a la memoria compartida (el publicador ya debería haber cargado la imagen)
    int shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid == -1) {
        perror("Error al acceder a la memoria compartida");
        return 1;
    }

    // Adjuntar la memoria compartida
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        return 1;
    }

    // Bloquear el mutex antes de acceder a la imagen
    pthread_mutex_lock(&(shared_data->mutex));

    BMP_Image *imageIn = &(shared_data->image);
    BMP_Image *imageOut = initializeImageOut(imageIn);

    // Crear hilos para aplicar el filtro
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = abs(imageIn->norm_height) / numThreads;

    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? abs(imageIn->norm_height) : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].filter = simplifiedSobelFilter;  // Usar el filtro adecuado
        pthread_create(&threads[i], NULL, applyFilter, &threadArgs[i]);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Escribir la imagen de salida
    writeImage(outputFile, imageOut);

    // Desbloquear el mutex después de escribir la imagen
    pthread_mutex_unlock(&(shared_data->mutex));

    // Desconectar de la memoria compartida
    shmdt(shared_data);

    // Liberar la memoria de la imagen de salida
    freeImage(imageOut);

    return 0;
}
