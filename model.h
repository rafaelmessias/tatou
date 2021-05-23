#include <stdint.h>
#include "linmath.h"


typedef struct {
    uint8_t type;
    uint8_t colorIndex;
    uint8_t numOfPointInPoly;
    uint16_t* indices;
    uint16_t discSize;
} Primitive;


typedef enum {
    PRIM_LINE,
    PRIM_POLY,
    PRIM_POINT,
    PRIM_CIRCLE, // DISC...?
    PRIM_UNK_4,
    PRIM_UNK_5,
    PRIM_SQUARE,
    PRIM_UNK_6
} PrimType;

// Global variables that store the current model
extern float *allCoords;
extern int numOfVertices;
extern Primitive *allPrims;
extern int numPrim;


void loadModel(const char* pakName, int index);
void applyMatrix(mat4x4 M, float* in, int n, float* out);
void getCentroid(vec3 r);
float getRadius(vec3 centroid);
void dumpPrim(int);
void dumpModel();
void dumpBytes(uint8_t *ptr, int num, int rowLen);
