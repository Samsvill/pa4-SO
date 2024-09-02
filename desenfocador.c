#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

#define SHM_KEY 12345
#define MAX_IMAGE_SIZE 1024*1024*10
#define KERNEL_SIZE 3
#define OUTPUT_FILE "blurred_output.bmp"

typedef struct {
    BmpImage *image;
    int start_row;
    int num_rows;
} BlurArgs;

// Kernel de desenfoque
int blur_kernel[KERNEL_SIZE][KERNEL_SIZE] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}
};

void *blur_section(void *arg) {
    BlurArgs *args = (BlurArgs *)arg;
    Pixel *pixels = args->image->pixels;

    // Aplicar convolución del kernel de desenfoque
    for (int i = args->start_row; i < args->start_row + args->num_rows; i++) {
        for (int j = 1; j < args->image->header.width_px - 1; j++) {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            for (int ki = 0; ki < KERNEL_SIZE; ki++) {
                for (int kj = 0; kj < KERNEL_SIZE; kj++) {
                    Pixel *p = &pixels[(i + ki - 1) * args->image->header.width_px + (j + kj - 1)];
                    sum_r += p->red * blur_kernel[ki][kj];
                    sum_g += p->green * blur_kernel[ki][kj];
                    sum_b += p->blue * blur_kernel[ki][kj];
                }
            }
            Pixel *p = &pixels[i * args->image->header.width_px + j];
            p->red = sum_r / 9;
            p->green = sum_g / 9;
            p->blue = sum_b / 9;
        }
    }
    return NULL;
}

void save_bmp(const char *filename, BmpImage *image) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(1);
    }

    // Escribir el encabezado BMP
    fwrite(&image->header, sizeof(BmpHeader), 1, file);

    // Asegurarse de escribir los píxeles correctamente, manejando el padding
    int padding = (4 - (image->header.width_px * sizeof(Pixel)) % 4) % 4;
    for (int i = 0; i < image->header.height_px; i++) {
        fwrite(image->pixels + i * image->header.width_px, sizeof(Pixel), image->header.width_px, file);
        fwrite("\0\0\0", padding, 1, file);  // Agregar el padding necesario
    }

    fclose(file);
    printf("Imagen guardada en %s\n", filename);
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
    printf("id memoria compartida: %i",shm_id);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    shm_ptr = (BmpImage *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (BmpImage *) -1) {
        perror("shmat");
        exit(1);
    }

    //int half_height = shm_ptr->header.height_px / 2;
    int half_height = shm_ptr->header.height_px;

    pthread_t threads[num_threads];
    BlurArgs args[num_threads];
    int rows_per_thread = half_height / num_threads;

    for (int i = 0; i < num_threads; i++) {
        args[i].image = shm_ptr;
        args[i].start_row = i * rows_per_thread;
        args[i].num_rows = rows_per_thread;
        pthread_create(&threads[i], NULL, blur_section, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Desenfoque completado en la primera mitad.\n");

    // Guardar la imagen procesada para verificar
    save_bmp(OUTPUT_FILE, shm_ptr);

    return 0;
}

