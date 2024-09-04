#ifndef FILTER_H
#define FILTER_H

#include "bmp.h"

// Declaraciones de filtros predefinidos
extern int simplifiedSobelFilter[3][3];
extern int sobelFilter[3][3];
extern int simpleEdgeEnhanceFilter[3][3];
extern int blurFilter[3][3];
extern int sharpenFilter[3][3];

// Función para aplicar un filtro específico en un rango de filas
void *applyFilter(void *args);

// Estructura para pasar argumentos a los hilos
typedef struct {
    int startRow;
    int endRow;
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int (*filter)[3];
} ThreadArgs;


#endif // FILTER_H
