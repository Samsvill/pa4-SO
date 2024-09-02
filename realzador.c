// realzador.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

#define SHM_KEY 12345
#define KERNEL_SIZE 3

typedef struct {
    BmpImage *image;
    int start_row;
    int num_rows;
} EdgeArgs;

int edge_kernel[KERNEL_SIZE][KERNEL_SIZE] = {
    {-1, -1, -1},
    {-1,  8, -1},
    {-1, -1, -1}
};

void *edge_section(void *arg) {
    EdgeArgs *args = (EdgeArgs *)arg;
    int width = args->image->header.width_px;
    int height = args->image->header.height_px;
    Pixel *pixels = args->image->pixels;

    // Aplicar convolución del kernel de detección de bordes
    for (int i = args->start_row; i < args->start_row + args->num_rows; i++) {
        for (int j = 1; j < width - 1; j++) {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            for (int ki = 0; ki < KERNEL_SIZE; ki++) {
                for (int kj = 0; kj < KERNEL_SIZE; kj++) {
                    Pixel *p = &pixels[(i + ki - 1) * width + (j + kj - 1)];
                    sum_r += p->red * edge_kernel[ki][kj];
                    sum_g += p->green * edge_kernel[ki][kj];
                    sum_b += p->blue * edge_kernel[ki][kj];
                }
            }
            Pixel *p = &pixels[i * width + j];
            p->red = sum_r;
            p->green = sum_g;
            p->blue = sum_b;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <número de hilos>\n", argv[0]);
        return 1;
    }
    int num_threads = atoi(argv[1]);

    int shm_id;
    BmpImage *shm_ptr;

    shm_id = shmget(SHM_KEY, MAX_IMAGE_SIZE, 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    shm_ptr = (BmpImage *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (BmpImage *) -1) {
        perror("shmat");
        exit(1);
    }

    int width = shm_ptr->header.width_px;
    int height = shm_ptr->header.height_px;
    int half_height = height / 2;

    pthread_t threads[num_threads];
    EdgeArgs args[num_threads];
    int rows_per_thread = half_height / num_threads;

    for (int i = 0; i < num_threads; i++) {
        args[i].image = shm_ptr;
        args[i].start_row = half_height + i * rows_per_thread;
        args[i].num_rows = rows_per_thread;
        pthread_create(&threads[i], NULL, edge_section, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Detección de bordes completada en la segunda mitad.\n");

    return 0;
}

