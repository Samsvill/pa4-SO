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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <ruta_imagen>\n", argv[0]);
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
    int shm_size = sizeof(pthread_mutex_t) + imageSize;

    // Crear o acceder a la memoria compartida
    printf("Creando/accediendo a la memoria compartida...\n");
    printf("Argumentos: SHM_KEY=%d, tamaño=%d\n", SHM_KEY, shm_size);
    int shmid = shmget(SHM_KEY, shm_size, 0666 | IPC_CREAT);
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

    // Inicializar el mutex si es necesario
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);  // El mutex será compartido entre procesos
    pthread_mutex_init(&(shared_data->mutex), &mutex_attr);

    // Bloquear el mutex antes de acceder a la memoria compartida
    pthread_mutex_lock(&(shared_data->mutex));

    // Copiar la imagen en la memoria compartida (encabezado y píxeles)
    memcpy(&(shared_data->image), image, sizeof(BMP_Image));
    memcpy(shared_data->pixels, image->pixels, image->norm_height * image->header.width_px * sizeof(Pixel));

    // Desbloquear el mutex después de escribir en la memoria compartida
    pthread_mutex_unlock(&(shared_data->mutex));

    // Liberar la imagen original (ya está copiada en la memoria compartida)
    freeImage(image);

    // Crear un proceso hijo para lanzar el realzador
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error al hacer fork");
        exit(1);
    }

    if (pid == 0) {
        // Código del proceso hijo: ejecutar el realzador
        char *args[] = {"./realzador", "output_test.bmp", "4", NULL};  // 4 hilos como ejemplo
        execvp(args[0], args);
        perror("Error al ejecutar el realzador");
        exit(1);
    } else {
        // Código del proceso padre: esperar a que el realzador termine
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Realzador finalizó con estado %d\n", WEXITSTATUS(status));
        } else {
            printf("Realzador no terminó correctamente\n");
        }

        // Destruir el mutex
        pthread_mutex_destroy(&(shared_data->mutex));

        // Desconectar y liberar la memoria compartida
        shmdt(shared_data);
        shmctl(shmid, IPC_RMID, NULL);  // Eliminar la memoria compartida
    }

    return 0;
}
