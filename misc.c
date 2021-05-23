#include <stdlib.h>
#include <math.h>
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


// TODO Maybe a function to initialize an empty model (since this code is repeated a lot)
//   Parameters: number of vertices, number of primitives
// TODO This is so complicated for a single primitive, with a single vertex...
void loadCircle()
{
    // Red
    Palette[0] = 255;

    numOfVertices = 20;
    if (allCoords != NULL)
        free(allCoords);
    allCoords = (float *)malloc(numOfVertices * sizeof(float) * 3);
    
    numPrim = 1;
    if (allPrims != NULL)
    {
        free(allPrims);
        for (int i = 0; i < numPrim; ++i)
            free(allPrims[i].indices);
    }
    allPrims = (Primitive *)malloc(numPrim * sizeof(Primitive));

    // For lazyness, I'll just assume that the undefined allCoords are all 0

    // This is just done once, but I'll soon have more than one, so...
    for (int i = 0; i < numPrim; ++i)
    {
        allPrims[i].type = PRIM_POLY;
        allPrims[i].colorIndex = i;
        // Although the circle will be a triangle fan, it only has one vertex in the actual model (the center)
        //   The rest are artficial points that will be generated on the fly (and without rotation)
        allPrims[i].numOfPointInPoly = numOfVertices;
        allPrims[i].indices = (uint16_t *)malloc(numOfVertices * sizeof(uint16_t));
        // NOTE We don't need the center point; the triangle fan works perfectly
        for (int j = 0; j < numOfVertices; ++j)
        {
            // Skip the first position (the center)
            int offset = j * 3;
            double angleRad = 2 * M_PI * (float)j / (float)numOfVertices;
            // X
            allCoords[offset] = (float)cos(angleRad);
            // Y
            allCoords[offset + 1] = (float)sin(angleRad);
            // Z
            allCoords[offset + 2] = 0.0f;
            // Draw the points in order
            allPrims[i].indices[j] = j;
        }
    }

    // 50% downscaling just for aesthetics
    mat4x4 M;
    mat4x4_identity(M);
    mat4x4_scale(M, M, 0.5f);
    applyMatrix(M, NULL);
}