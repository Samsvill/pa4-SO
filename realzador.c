#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"
#include "filter.h"

#define SHM_KEY 1234  // Clave para la memoria compartida
#define PATH_NAME "test.bmp"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <half1|half2> <número_hilos>\n", argv[0]);
        return 1;
    }

    int numThreads = atoi(argv[2]);

    // Acceder a la memoria compartida (el publicador ya debería haber cargado la imagen)
    key_t key = ftok(PATH_NAME, SHM_KEY);
    int shmid = shmget(key, 0, 0666);
    if (shmid == -1) {
        perror("Error al acceder a la memoria compartida, ¿ejecutó el publicador?");
        return 1;
    }

    // Adjuntar la memoria compartida
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        return 1;
    }

    printf("Memoria compartida accedida\n");
    printf("norm_height %d", shared_data->image.norm_height);

    int startRow, endRow;
    if (strcmp(argv[1], "half2") == 0) {
        startRow = abs(shared_data->image.norm_height) / 2;
        endRow = abs(shared_data->image.norm_height);
    }

    BMP_Image *imageIn = &(shared_data->image);

    // Usamos directamente la imagen en la memoria compartida para modificarla.
    BMP_Image *imageOut = &(shared_data->image);  

    // Inicializar los punteros a las filas en la memoria compartida
    for (int i = 0; i < imageOut->norm_height; i++) {
        imageOut->pixels[i] = &shared_data->pixels[i * imageOut->header.width_px];
    }

    // Crear hilos para aplicar el filtro
    printf("Aplicando filtro en la mitad %s con %d hilos...\n", argv[1], numThreads);
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
        threadArgs[i].filter = simplifiedSobelFilter;  // El filtro que quieres aplicar
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
    printf("Marcando como procesado y enviando señal...\n");
    if (strcmp(argv[1], "half2") == 0) {
        shared_data->half2_done = 1;
        pthread_cond_signal(&(shared_data->cond_half2));
    }
    printf("Listo\n");

    // Desbloquear el mutex
    printf("Desbloqueando mutex...\n");
    pthread_mutex_unlock(&(shared_data->mutex));
    printf("Mutex desbloqueado\n");

    // Desconectar de la memoria compartida
    shmdt(shared_data);

    return 0;
}
