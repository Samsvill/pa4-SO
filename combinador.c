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
    key_t key = ftok(PATH_NAME, SHM_KEY);
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

    printf("Memoria compartida accedida\n");
    printf("adjuntando la memoria compartida\n");
    // Adjuntar la memoria compartida
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        return 1;
    }
    printf("Memoria compartida adjuntada\n");
    printf("Esperando a que ambas mitades estén procesadas\n");
    // Esperar a que ambas mitades estén procesadas
    pthread_mutex_lock(&(shared_data->mutex));
    printf("mutex lock\n");
    // Esperar a que la mitad 1 esté lista
    printf("Esperando a que la mitad 1 esté lista\n");
    while (!shared_data->half1_done) {
        pthread_cond_wait(&(shared_data->cond_half1), &(shared_data->mutex));  // Bloquearse hasta que se procese la mitad 1
    }
    printf("Mitad 1 procesada\n");
    printf("Esperando a que la mitad 2 esté lista\n");
    // Esperar a que la mitad 2 esté lista
    while (!shared_data->half2_done) {
        pthread_cond_wait(&(shared_data->cond_half2), &(shared_data->mutex));  // Bloquearse hasta que se procese la mitad 2
    }
    printf("Mitad 2 procesada\n");
    // Ambas mitades están listas, ahora podemos combinar la imagen
    printf("Ambas mitades procesadas. Escribiendo imagen completa en: %s\n", argv[1]);

    // Inicializar los punteros `pixels` para que apunten correctamente a los píxeles en la memoria compartida
    for (int i = 0; i < shared_data->image.norm_height; i++) {
        shared_data->image.pixels[i] = &shared_data->pixels[i * shared_data->image.header.width_px];
    }

    // Escribir la imagen combinada en el archivo de salida
    writeImage(argv[1], &(shared_data->image));

    pthread_mutex_unlock(&(shared_data->mutex));

    // Desconectar de la memoria compartida
    shmdt(shared_data);

    return 0;
}
