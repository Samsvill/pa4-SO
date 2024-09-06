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
    char *outputFile = argv[1];  // Usar el archivo de salida que se pasa como argumento

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

    int startRow, endRow;
    if (strcmp(argv[1], "half1") == 0) {
        startRow = 0;
        endRow = abs(shared_data->image.norm_height) / 2;
    } else {
        startRow = abs(shared_data->image.norm_height) / 2;
        endRow = abs(shared_data->image.norm_height);
    }

    BMP_Image *imageIn = &(shared_data->image);
    BMP_Image *imageOut = initializeImageOut(imageIn);

    // Crear hilos para aplicar el filtro
    printf("Aplicando filtro en la mitad %s con %d hilos...\n", argv[1], numThreads);
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    int rowsPerThread = endRow - startRow / numThreads;
    printf("Filas por hilo: %d\n", rowsPerThread);

    printf("Creando hilos...\n");
    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].startRow = startRow + i * rowsPerThread;
        threadArgs[i].endRow = (i == numThreads - 1) ? endRow : threadArgs[i].startRow + rowsPerThread;
        threadArgs[i].imageIn = imageIn;
        threadArgs[i].imageOut = imageOut;
        threadArgs[i].filter = (strcmp(argv[0], "./realzador") == 0) ? simplifiedSobelFilter : blurFilter;
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
    printf("Escribiendo en la memoria compartida...\n");

    // Copiar los píxeles procesados de nuevo a la memoria compartida
    printf("Copiando píxeles...\n");
    memcpy(&shared_data->pixels[startRow * imageIn->header.width_px],
           &imageOut->pixels[startRow * imageIn->header.width_px],
           (endRow - startRow) * imageIn->header.width_px * sizeof(Pixel));
    printf("Píxeles escritos en la memoria compartida\n");
    // Marcar como procesado y enviar la señal correspondiente
    printf("Marcando como procesado y enviando señal...\n");
    if (strcmp(argv[1], "half1") == 0) {
        shared_data->half1_done = 1;
        pthread_cond_signal(&(shared_data->cond_half1));
    } else {
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

    // Liberar la imagen de salida
    freeImage(imageOut);

}
