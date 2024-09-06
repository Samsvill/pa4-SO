#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include "bmp.h"

#define SHM_KEY 1234  // Clave para la memoria compartida
#define PATH_NAME "test.bmp"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <ruta_imagen> <ruta_output> <hilos_filtro1> <hilos_filtro2>\n", argv[0]);
        return 1;
    }

    char *inputFile = argv[1];

    // Abrir el archivo BMP
    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL) {
        perror("No se pudo abrir la imagen");
        return 1;
    }

    // Leer la imagen desde el archivo
    BMP_Image *image = createBMPImage(imageFile);
    fclose(imageFile);

    // Calcular el tamaño de la memoria compartida
    int imageSize = sizeof(BMP_Image) + abs(image->norm_height) * image->header.width_px * sizeof(Pixel);
    int shm_size = sizeof(SharedData) + imageSize;  // Incluyendo SharedData y los píxeles

    // Crear o acceder a la memoria compartida
    printf("Creando/accediendo a la memoria compartida...\n");

    key_t key = ftok(inputFile, SHM_KEY);  // Genera una clave única a partir de un archivo
    if (key == -1) {
        perror("Error al generar la clave con ftok");
        exit(1);
    }
    int shmid = shmget(key, shm_size, 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("Error al crear/acceder a la memoria compartida");
        exit(1);
    }

    // Adjuntar la memoria compartida
    SharedData *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (SharedData *)-1) {
        perror("Error al adjuntar la memoria compartida");
        exit(1);
    }

    // Inicializar el mutex
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shared_data->mutex), &mutex_attr);

    // Inicializar las condiciones
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&(shared_data->cond_half1), &cond_attr);
    pthread_cond_init(&(shared_data->cond_half2), &cond_attr);

    // Inicializar los estados de procesamiento
    shared_data->half1_done = 0;
    shared_data->half2_done = 0;

    printf("Memoria compartida creada/accedida con ID %d\n", shmid);
    printf("Compartiendo imagen...\n");

    // Copiar la imagen en la memoria compartida
    // Primero, copiar el encabezado y el doble puntero (no los datos aún)
    memcpy(&(shared_data->image), image, sizeof(BMP_Image));

    // Copiar los píxeles en un bloque contiguo
    shared_data->pixels_data = (Pixel *)(shared_data + 1);  // Espacio después de SharedData
    memcpy(shared_data->pixels_data, image->pixels_data, image->norm_height * image->header.width_px * sizeof(Pixel));

    // Asignar las filas a los punteros del doble puntero
    shared_data->image.pixels = (Pixel **)malloc(image->norm_height * sizeof(Pixel *));
    for (int i = 0; i < image->norm_height; i++) {
        shared_data->image.pixels[i] = &shared_data->pixels_data[i * image->header.width_px];
    }

    printf("Imagen compartida\n");

    // Liberar la imagen original (ya está copiada en la memoria compartida)
    freeImage(image);

    // Crear un proceso hijo para lanzar el realzador
    pid_t pid_realzador = fork();
    if (pid_realzador == 0) {
        // Proceso hijo: lanzar el realzador
        char *args[] = {"./realzador", "half1", argv[3], NULL};  // Número de hilos argv[3]
        execvp(args[0], args);
        perror("Error al ejecutar el realzador");
        exit(1);
    }

    pid_t pid_desenfocador = fork();
    if (pid_desenfocador == 0) {
        // Proceso hijo: lanzar el desenfocador
        char *args[] = {"./desenfocador", "half2", argv[4], NULL};  // Número de hilos argv[4]
        execvp(args[0], args);
        perror("Error al ejecutar el desenfocador");
        exit(1);
    }

    // Esperar a que ambos procesos terminen
    printf("Esperando realzador\n");
    waitpid(pid_realzador, NULL, 0);
    printf("Realzador terminó, esperando desenfocador\n");
    waitpid(pid_desenfocador, NULL, 0);
    printf("Desenfocador terminó\n");

    // Después de que ambos hayan terminado, lanzar el combinador
    printf("Lanzando combinador\n");
    pid_t pid_combinador = fork();
    if (pid_combinador == 0) {
        printf("Dentro del hilo del combinador\n");
        char *args[] = {"./combinador", argv[2], NULL};  // Guardar el resultado en argv[2]
        execvp(args[0], args);
        perror("Error al ejecutar el combinador");
        exit(1);
    }

    // Esperar a que el combinador termine
    waitpid(pid_combinador, NULL, 0);
    printf("Combinador terminó\n");

    // Destruir el mutex y liberar recursos
    pthread_mutex_destroy(&(shared_data->mutex));
    pthread_cond_destroy(&(shared_data->cond_half1));
    pthread_cond_destroy(&(shared_data->cond_half2));

    // Desconectar y liberar la memoria compartida
    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);  // Eliminar la memoria compartida

    return 0;
}
