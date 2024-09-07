#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/shm.h>
#include "bmp.h"

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <shmid>\n", argv[0]);
        return 1;
    }

    int shmid = atoi(argv[1]);

    // Adjuntar la memoria compartida
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        return 1;
    }

    printf("Memoria compartida accedida\n");

    // Verificar el estado inicial de half2_done
    printf("Estado inicial de half2_done: %d\n", shared_data->half2_done);

    // Bloquear el mutex antes de esperar la condición
    pthread_mutex_lock(&(shared_data->mutex));
    printf("Mutex bloqueado\n");

    // Esperar a que la segunda mitad esté lista
    printf("Esperando a que la segunda mitad esté lista...\n");
    while (!shared_data->half2_done) {
        printf("Todavía esperando la segunda mitad...\n");
        pthread_cond_wait(&(shared_data->cond_half2), &(shared_data->mutex));
    }

    // Verificar el estado final de half2_done
    printf("Estado final de half2_done: %d\n", shared_data->half2_done);

    // Desbloquear el mutex
    pthread_mutex_unlock(&(shared_data->mutex));
    printf("Mutex desbloqueado\n");

    // Desconectar de la memoria compartida
    shmdt(shared_data);
    return 0;
}