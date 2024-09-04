#ifndef FILTER_H
#define FILTER_H

#include "bmp.h"

// Declaraciones de filtros predefinidos
extern int edgeEnhanceFilter[3][3];
extern int blurFilter[3][3];

// Función para aplicar un filtro específico en un rango de filas
void *applyFilter(void *args);

// Estructura para pasar argumentos a los hilos
typedef struct {
    int startRow;
    int endRow;
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int filter[3][3];  // Permitir aplicar filtros diferentes
} ThreadArgs;

#endif // FILTER_H
