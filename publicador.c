// publicador.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

#define SHM_KEY 12345
#define MAX_IMAGE_SIZE 1024*1024*10  // Tamaño máximo de imagen de 10 MB

BmpImage* load_bmp(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return NULL;
    }

    BmpImage *image = (BmpImage *)malloc(sizeof(BmpImage));

    // Leer el encabezado de la imagen BMP
    fread(&image->header, sizeof(BmpHeader), 1, file);

    // Verificar que sea un archivo BMP válido
    if (image->header.type != 0x4D42) {
        printf("Archivo no es un BMP válido.\n");
        free(image);
        fclose(file);
        return NULL;
    }

    // Leer los píxeles
    image->pixels = (Pixel *)malloc(image->header.image_size_bytes);
    fseek(file, image->header.offset, SEEK_SET);
    fread(image->pixels, image->header.image_size_bytes, 1, file);

    fclose(file);
    return image;
}

int main() {
    int shm_id;
    void *shm_ptr;

    // Crear segmento de memoria compartida
    shm_id = shmget(SHM_KEY, MAX_IMAGE_SIZE, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    // Adjuntar memoria compartida
    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    while (1) {
        char file_path[256];
        printf("Ingrese la ruta de la imagen BMP: ");
        scanf("%s", file_path);

        BmpImage *image = load_bmp(file_path);
        if (image == NULL) {
            continue;
        }

        // Copiar la imagen a la memoria compartida
        memcpy(shm_ptr, image, sizeof(BmpImage) + image->header.image_size_bytes);
        free(image->pixels);
        free(image);

        printf("Imagen cargada en memoria compartida.\n");
    }

    return 0;
}

