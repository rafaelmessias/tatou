#include <stdint.h>
#include "linmath.h"


typedef struct {
    uint8_t type;
    uint8_t colorIndex;
    uint16_t* indices;
    uint8_t numOfPointInPoly;
    uint16_t discSize;
    // GLenum mode;
} Primitive;


// Global variables that store the current model
extern float *allCoords;
extern int numOfVertices;
extern Primitive *allPrims;
extern int numPrim;


void loadModel(const char* pakName, int index);
void applyMatrix(mat4x4 M, float* out);
void getCentroid(vec3 r);
float getRadius(vec3 centroid);
void dumpPrim(int);
void dumpModel();
