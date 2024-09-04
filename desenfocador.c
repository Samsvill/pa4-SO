#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

#define SHM_KEY 12345
//#define MAX_IMAGE_SIZE 1024*1024*10
#define KERNEL_SIZE 3
#define SHM_SIZE sizeof(BmpImage)
#define OUTPUT_FILE "output_desenfocado.bmp"
#define INPUT_FILE "./testcases/test.bmp"


typedef struct {
    BmpImage *image;
    BmpImage *result_image;
    int start_row;
    int end_row;
} BlurArgs;


int blur_kernel[KERNEL_SIZE][KERNEL_SIZE] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}
};

void *blur_section(void *arg) {
    BlurArgs *args = (BlurArgs *)arg;
    BmpImage *image = args->image;
    BmpImage *result_image = args->result_image;
    int start_row = args->start_row;
    int end_row = args->end_row;
    
    int width = image->header.width_px;
    int height = image->header.height_px;

    for (int i = start_row; i < end_row; i++) {
        for (int j = 1; j < width - 1; j++) {
            int sum_red = 0, sum_green = 0, sum_blue = 0, sum_alpha = 0;
            for (int ki = 0; ki < KERNEL_SIZE; ki++) {
                for (int kj = 0; kj < KERNEL_SIZE; kj++) {
                    int ni = i + ki - 1;
                    int nj = j + kj - 1;
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        Pixel *p = &image->pixels[ni * width + nj];
                        sum_red += p->red * blur_kernel[ki][kj];
                        sum_green += p->green * blur_kernel[ki][kj];
                        sum_blue += p->blue * blur_kernel[ki][kj];
                        sum_alpha += p->alpha * blur_kernel[ki][kj];
                    }
                }
            }
            Pixel *p = &result_image->pixels[i * width + j];
            p->red = sum_red / 9;
            p->green = sum_green / 9;
            p->blue = sum_blue / 9;
            p->alpha = sum_alpha / 9;
        }
    }
    pthread_exit(NULL);
}

void save_bmp(const char *filename, BmpImage *image) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(1);
    }


    fwrite(&image->header, sizeof(BmpHeader), 1, file);

   
    int padding = (4 - (image->header.width_px * sizeof(Pixel) % 4)) % 4;
    for (int i = 0; i < image->header.height_px; i++) {
        fwrite(image->pixels + i * image->header.width_px, sizeof(Pixel), image->header.width_px, file);
        if (padding > 0) {
            unsigned char pad[4] = {0}; 
            fwrite(pad, padding, 1, file);
        }
    }

    fclose(file);
    printf("Imagen guardada en %s\n", filename);
}

BmpImage *load_bmp(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return NULL;
    }

    BmpImage *image = (BmpImage *)malloc(sizeof(BmpImage));
    if (!image) {
        perror("malloc");
        fclose(file);
        return NULL;
    }

    
    fread(&image->header, sizeof(BmpHeader), 1, file);

    
    int width = image->header.width_px;
    int height = image->header.height_px;
    int padding = (4 - (width * sizeof(Pixel) % 4)) % 4;

    image->pixels = (Pixel *)malloc((width + padding / sizeof(Pixel)) * height * sizeof(Pixel));
    if (!image->pixels) {
        perror("malloc");
        free(image);
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < height; i++) {
        fread(image->pixels + i * width, sizeof(Pixel), width, file);
        fseek(file, padding, SEEK_CUR);
    }

    fclose(file);
    return image;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <número de hilos>\n", argv[0]);
        return 1;
    }
    int num_threads = atoi(argv[1]);

   /*BmpImage *image = load_bmp(image);
    if (!image) {
        fprintf(stderr, "Error al cargar la imagen %s\n", INPUT_FILE);
        return 1;
    }*/
  // FILE *file = fopen("./testcases/test.bmp", "rb");
   int shm_id;
   BmpImage *shm_ptr = load_bmp(INPUT_FILE);

    shm_id = shmget(SHM_KEY, SHM_SIZE, 0666);
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

    int height = shm_ptr->header.height_px;
    int rows_per_thread = height / num_threads;

    pthread_t threads[num_threads];
    BlurArgs args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        args[i].image = shm_ptr;
        args[i].start_row = i * rows_per_thread;
        args[i].end_row = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;
        pthread_create(&threads[i], NULL, blur_section, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Desenfoque completado.\n");

    
    save_bmp(OUTPUT_FILE, shm_ptr);


    
    //free(shm_ptr->pixels);
    //free(shm_ptr);
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
