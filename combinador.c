// combinador.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

#define SHM_KEY 12345
#define OUTPUT_FILE "output.bmp"

void save_bmp(const char *filename, BmpImage *image) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        exit(1);
    }

    // Escribir el encabezado BMP
    fwrite(&image->header, sizeof(BmpHeader), 1, file);

    // Escribir los píxeles
    fseek(file, image->header.offset, SEEK_SET);
    fwrite(image->pixels, image->header.image_size_bytes, 1, file);

    fclose(file);
    printf("Imagen guardada en %s\n", filename);
}

int main() {
    int shm_id;
    BmpImage *shm_ptr;

    // Adjuntar a la memoria compartida
    shm_id = shmget(SHM_KEY, 0, 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    shm_ptr = (BmpImage *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (BmpImage *)-1) {
        perror("shmat");
        exit(1);
    }

    // Aquí, asumiendo que las dos mitades ya están procesadas,
    // simplemente guardamos la imagen combinada.
    save_bmp(OUTPUT_FILE, shm_ptr);

    // Desvincular la memoria compartida
    shmdt(shm_ptr);

    return 0;
}

