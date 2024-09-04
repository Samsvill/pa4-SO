#include "bmp.h"

typedef struct {
	BMP_Image * imageIn;
	BMP_Image * imageOut;
	int (*filter)[3];
	int tid;
	int numThreads;
    float norm;
	int startRow;
	int endRow;
} FilterThreadArgs;

// Declaraciones de filtros predefinidos
extern int edgeEnhanceFilter[3][3];
extern int blurFilter[3][3];

// Función para aplicar un filtro específico en un rango de filas
void *applyFilter(void *args);
