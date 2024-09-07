#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/shm.h>
#include "bmp.h"

#define SHM_KEY 1234 // Clave para la memoria compartida
#define PATH_NAME "test.bmp"

int main(int argc, char *argv[])
{
    // Verificar argumentos
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <output_file>\n", argv[0]);
    }

    char *output_file = argv[1];
    // Adjuntar la memoria compartida
    key_t key = ftok(PATH_NAME, SHM_KEY);
    if (key == -1)
    {
        perror("Error al generar la clave con ftok");
        return 1;
    }
    int shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        perror("Error al acceder a la memoria compartida");
        return 1;
    }
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        return 1;
    }

    printf("Memoria compartida accedida\n");

    // Verificar el estado inicial de half2_done
    printf("Estado inicial de half1_done: %d\n", shared_data->half1_done);
    printf("Estado inicial de half2_done: %d\n", shared_data->half2_done);

    // Esperar a que ambos procesos terminen
    while (shared_data->half1_done == 0 || shared_data->half2_done == 0)
    {
        printf("Esperando a que ambos procesos terminen...\n");
        sleep(1);
    }
    
    
    printf("Guardando la imagen final en %s...\n", output_file);
    writeImage("output.bmp", &(shared_data->image));
    printf("Imagen guardada exitosamente.\n");

    // Desconectar de la memoria compartida
    shmdt(shared_data);
    return 0;
}