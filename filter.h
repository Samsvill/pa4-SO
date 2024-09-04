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


void apply(BMP_Image * imageIn, BMP_Image * imageOut);

void applyParallel(BMP_Image * imageIn, BMP_Image * imageOut, int boxFilter[3][3], int numThreads);

void *filterThreadWorker(void * args);
