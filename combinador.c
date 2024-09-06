#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include "bmp.h"

#define SHM_KEY 1234
#define PATH_NAME "test.bmp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <ruta_salida>\n", argv[0]);
        return 1;
    }

    // Generar la clave con ftok
    key_t key = ftok(PATHNAME, SHM_KEY);
    if (key == -1) {
        perror("Error al generar la clave con ftok");
        return 1;
    }

    // Acceder a la memoria compartida
    int shmid = shmget(key, 0, 0666);
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

    // Esperar a que ambas mitades estén procesadas
    pthread_mutex_lock(&(shared_data->mutex));

    while (!shared_data->half1_done) {
        pthread_cond_wait(&(shared_data->cond_half1), &(shared_data->mutex));  // Espera a que la mitad 1 esté lista
    }
    while (!shared_data->half2_done) {
        pthread_cond_wait(&(shared_data->cond_half2), &(shared_data->mutex));  // Espera a que la mitad 2 esté lista
    }
    
    // Ambas mitades están listas, ahora podemos combinar la imagen
    writeImage(argv[1], &(shared_data->image));
    
    pthread_mutex_unlock(&(shared_data->mutex));

    // Desconectar de la memoria compartida
    shmdt(shared_data);

    return 0;
}
