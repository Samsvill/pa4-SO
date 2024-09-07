#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"
#include "filter.h"

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <shmid> <numThreads>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);
    int numThreads = atoi(argv[2]);

    // Adjuntar la memoria compartida
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        return 1;
    }

    printf("Memoria compartida accedida\n");
    printf("norm_height %d\n", shared_data->image.norm_height);

    // Siempre operar en la segunda mitad de la imagen
    int startRow = abs(shared_data->image.norm_height) / 2;
    int endRow = abs(shared_data->image.norm_height);

    BMP_Image *imageIn = &(shared_data->image);

    // Usar directamente la imagen en la memoria compartida para modificarla
    BMP_Image *imageOut = &(shared_data->image);  

    // Inicializar los punteros a las filas en la memoria compartida
    for (int i = 0; i < imageOut->norm_height; i++) {
        imageOut->pixels[i] = &shared_data->pixels[i * imageOut->header.width_px];
    }

    // Crear hilos para aplicar el filtro
    printf("Aplicando filtro en la segunda mitad con %d hilos...\n", numThreads);
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = (endRow - startRow) / numThreads;
    printf("Filas por hilo: %d\n", rowsPerThread);

    printf("Creando hilos...\n");
    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = startRow + i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? endRow : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].filter = simplifiedSobelFilter;  // Aplicar el filtro Sobel simplificado
        pthread_create(&threads[i], NULL, applyFilter, &threadArgs[i]);
    }

    printf("Hilos creados\n");
    printf("Esperando a que los hilos terminen...\n");

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Hilos terminados\n");
    printf("Escribiendo imagen de salida...\n");

    // Bloquear el mutex antes de escribir en la memoria compartida
    printf("Bloqueando mutex...\n");
    pthread_mutex_lock(&(shared_data->mutex));
    printf("Mutex bloqueado\n");

    // Marcar como procesado y enviar la señal correspondiente
    printf("Marcando como procesado y enviando señal para la segunda mitad...\n");
    shared_data->half2_done = 1;
    pthread_cond_signal(&(shared_data->cond_half2));
    printf("Listo\n");

    // Desbloquear el mutex
    printf("Desbloqueando mutex...\n");
    pthread_mutex_unlock(&(shared_data->mutex));
    printf("Mutex desbloqueado\n");

    // Desconectar de la memoria compartida
    shmdt(shared_data);
    return 0;
}