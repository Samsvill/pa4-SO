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
#define BOX_SIZE 3
#define SHM_SIZE sizeof(BMP_Image)
//#define OUTPUT_FILE "output_desenfocado.bmp"
//#define INPUT_FILE "./testcases/airplane.bmp"


typedef struct {
    BMP_Image *image;
    int start_row;
    int end_row;
    BMP_Image *result_image;
} BlurArgs;

int blurFilter[BOX_SIZE][BOX_SIZE] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}
};

void *blur_section(void *args) {
    BlurArgs *bArgs = (BlurArgs *)args;
    BMP_Image *image = bArgs->image;
    int start_row = bArgs->start_row;
    int end_row = bArgs->end_row;
    BMP_Image *result_image = bArgs->result_image;
    

    for (int row = start_row; row < end_row; row++) {
        int width = image->header.width_px;
        for (int col = 0; col < width; col++) {
            int sum_red = 0, sum_green = 0, sum_blue = 0, sum_alpha = 0;
            for (int ki = 0; ki < BOX_SIZE; ki++) {
                for (int kj = 0; kj < BOX_SIZE; kj++) {
                    int ni = row + 1;//new row
                    int nj = col + 1;//new col
                    int height = image->header.height_px;
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        Pixel *p = &image->pixels[ni][nj];
                        sum_red += p->red * blurFilter[ki + 1][kj + 1];
                        sum_green += p->green * blurFilter[ki + 1][kj + 1];
                        sum_blue += p->blue * blurFilter[ki + 1][kj + 1];
                        sum_alpha += p->alpha * blurFilter[ki + 1][kj + 1];
                    }
                }
            }
            Pixel *p = &result_image->pixels[row][col];
            p->red = sum_red / 9;
            p->green = sum_green / 9;
            p->blue = sum_blue / 9;
            p->alpha = sum_alpha / 9;
        }
    }
    //free(bArgs);
    pthread_exit(NULL);
}

void save_bmp(const char *filename, BMP_Image *image) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(1);
    }


    fwrite(&image->header, sizeof(BMP_Header), 1, file);

   
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

BMP_Image *load_bmp(const char *filename, int shm_id) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    BMP_Image *image = (BMP_Image *)shmat(shm_id, NULL, 0);
    
    if (!image) {
        perror("shmat");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    
    fread(&image->header, sizeof(BMP_Header), 1, file);
    
    int width = image->header.width_px;
    int height = image->header.height_px;
    int padding = (4 - (width * sizeof(Pixel) % 4)) % 4;

    image->pixels = (Pixel *)malloc((width + padding / sizeof(Pixel)) * height * sizeof(Pixel));
    if (!image->pixels) {
        perror("malloc");
        free(image);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < height; i++) {
        fread(image->pixels + i * width, sizeof(Pixel), width, file);
        fseek(file, padding, SEEK_CUR);
    }
    readImage(file, image);
    fclose(file);
    return image;
}

int main(int argc, char *argv[]) {
    char *inputFile = argv[1];
    if (argc != 2) {
        printf("Uso: %s <número de hilos>\n", argv[0]);
        return 1;
    }
    int num_threads = atoi(argv[2]);

   /*BMP_Image *image = load_bmp(image);
    if (!image) {
        fprintf(stderr, "Error al cargar la imagen %s\n", INPUT_FILE);
        return 1;
    }*/
  // FILE *file = fopen("./testcases/test.bmp", "rb");
   int shm_id;
   //memoria compartida
   //BMP_Image *shm_ptr = load_bmp(INPUT_FILE);
    FILE *imageFile = fopen(inputFile, "rb");
    if (imageFile == NULL) {
        perror("Error al abrir el archivo de imagen");
        return 1;
    }
    BMP_Image *imageIn = load_bmp(imageFile);
    if (imageIn == NULL) {
        fprintf(stderr, "Error al cargar la imagen\n");
        fclose(imageFile);
        return 1;
    }
    fclose(imageFile);
    
    BMP_Image *shm_ptr = load_bmp(imageIn);

    if (shm_ptr == NULL) {
        fprintf(stderr, "Error al asignar memoria para la imagen de salida\n");
        freeImage(imageIn);
        return 1;
    }


    shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    printf("id memoria compartida: %i",shm_id);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    shm_ptr = (BMP_Image *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (BMP_Image *) -1) {
        perror("shmat");
        exit(1);
    }

    int height = shm_ptr->header.height_px;
    int rows_per_thread = height / num_threads;

    pthread_t threads[num_threads];
    BlurArgs blur_arg[num_threads];

    for (int i = 0; i < num_threads; i++) {
        blur_arg[i].image = shm_ptr;
        blur_arg[i].start_row = i * rows_per_thread;
        blur_arg[i].end_row = (i == num_threads - 1) ? height : (i + 1) * rows_per_thread;
        pthread_create(&threads[i], NULL, blur_section, &blur_arg[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Desenfoque completado.\n");

    
    save_bmp(OUTPUT_FILE, shm_ptr);


    //printBMPHeader(&shm_ptr->header);
    //printBMP_Image(shm_ptr);
    freeImage(shm_ptr);
    fclose(shm_ptr);
    //free(shm_ptr->pixels);
    //free(shm_ptr);
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
