#include <stdlib.h>
#include "model.h"
#include "system.h"


// TODO Make a "Model" struct
// TODO Make allCoords a vertex buffer of vec3's
// NOTE Remember: global pointers must be heap-allocated (that's why we do it here)
void loadCube()
{
    // Setup a palette with known colors for the first 6 primitives (ignore the rest)
    uint8_t tmpPal[] =
    {
          0, 0,   255,  // Front, blue
          0, 255,   0,  // Left, green
          0, 255, 255,  // Back, cyan
        255,   0,   0,  // Right, red
        255,   0, 255,  // Top, magenta
        255, 255,   0,  // Bottom, yellow
    };
    memcpy(Palette, tmpPal, sizeof(tmpPal));

    numOfVertices = 8;
    
    if (allCoords != NULL)
        free(allCoords);
    allCoords = (float *)malloc(numOfVertices * sizeof(float) * 3);

    numPrim = 6;

    if (allPrims != NULL)
    {
        free(allPrims);
        for (int i = 0; i < numPrim; ++i)
            free(allPrims[i].indices);
    }
    allPrims = (Primitive *)malloc(numPrim * sizeof(Primitive));

    // This is just to make it easier to write the coordinates
    // NOTE Remember that OpenGL's default winding order is counter-clockwise
    float coordTmp[] = 
    {
        // Front
        -1, -1,  1,   1, -1,   1,   1,  1,   1,   -1,  1,   1,
        // Back
        -1, -1, -1,   1, -1,  -1,   1,  1,  -1,   -1,  1,  -1,
    };
    memcpy(allCoords, coordTmp, sizeof(coordTmp));
    
    // This is just to make it easier to write the indices
    uint16_t indicesTmp[] =
    {
        0, 1, 2, 3, // Front
        0, 3, 7, 4, // Left
        4, 7, 6, 5, // Back
        1, 5, 6, 2, // Right
        3, 2, 6, 7, // Top
        0, 4, 5, 1  // Bottom
    };

    for (int i = 0; i < numPrim; ++i)
    {
        allPrims[i].type = PRIM_POLY;
        allPrims[i].colorIndex = i;
        allPrims[i].numOfPointInPoly = 4;
        allPrims[i].indices = (uint16_t *)malloc(allPrims[i].numOfPointInPoly * sizeof(uint16_t));
        int offset = i * allPrims[i].numOfPointInPoly;
        memcpy(allPrims[i].indices, indicesTmp + offset, allPrims[i].numOfPointInPoly * sizeof(uint16_t));
    }
    
    // 50% downscaling just for aesthetics
    mat4x4 M;
    mat4x4_identity(M);
    mat4x4_scale(M, M, 0.5f);
    applyMatrix(M, NULL);

}