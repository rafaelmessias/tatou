#include <stdlib.h>
#include "model.h"
#include "pak.h"


// TODO Globals should be capitalized correctly
float *allCoords = NULL;
int numOfVertices;
Primitive *allPrims = NULL;
int numPrim;
uint16_t *boneOffsets;
uint16_t numBones;
int16_t bbox[6];
vec3 bbCenter;


void dumpPrim(int primIndex)
{
    Primitive p = allPrims[primIndex];
    printf("Primitive #%d:\n", primIndex);
    printf("  Type: %d\n", p.type);
    printf("  Color: %d\n", p.colorIndex);
    printf("  Num. pts: %d\n", p.numOfPointInPoly);
    for (int i = 0; i < p.numOfPointInPoly; ++i)
    {
        printf(" %d", p.indices[i]);
    }
    printf("\n");
}


void dumpModel()
{
    printf("Current model:\n");
    printf("  Num. vertices: %d\n", numOfVertices);
    printf("  Num. bones: %d\n", numBones);
    printf("  Num. prims: %d\n", numPrim);
}


void dumpBytes(uint8_t *ptr, int num)
{
    for (int i = 0; i < num; ++i)
    {
        printf("%d ", *ptr++);
    }
    printf("\n");
}


void applyMatrix(mat4x4 M, float* out)
{
    if (out == NULL)
    {
        out = allCoords;
    }
    for (int i = 0; i < numOfVertices; ++i) {
        int offset = i * 3;
        vec4 v = {allCoords[offset], allCoords[offset+1], allCoords[offset+2], 1};
        vec4 r;
    //     vec3_scale(r, v, 1.0 / 1000.0);
        mat4x4_mul_vec4(r, M, v);
        out[offset] = r[0];
        out[offset+1] = r[1];
        out[offset+2] = r[2];
    }
}


// TODO Just for fun, make these computations vectorized...?
// NOTE Not using this anymore.
void getCentroid(vec3 r) {
    r[0] = 0, r[1] = 0, r[2] = 0;
    for (int i = 0; i < numOfVertices; ++i) {
        int offset = i * 3;
        r[0] += allCoords[offset];
        r[1] += allCoords[offset + 1];
        r[2] += allCoords[offset + 2];
    }
    r[0] /= numOfVertices;
    r[1] /= numOfVertices;
    r[2] /= numOfVertices;
}


float getRadius(vec3 centroid) {
    float r = 0.0f;
    for (int i = 0; i < numOfVertices; ++i) {
        int offset = i * 3;
        vec3 v = {
            allCoords[offset] - centroid[0], 
            allCoords[offset+1] - centroid[1], 
            allCoords[offset+2] - centroid[2]
        };
        // No need for sqrt here
        float vr = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
        if (vr > r)
            r = vr;
    }
    return sqrt(r);
}


// This is given in the file, but I'm not sure if it's always correct.
// NOTE Not using this for now.
// TODO Currently not following the original file format.
void getBoundingBox(float bbox[6])
{
    // Initialize output with the first vertex
    // Output format: min_x, min_y, min_z, max_x, max_y, max_z
    memcpy(bbox, allCoords, 3 * sizeof(float));
    memcpy(bbox+3, allCoords, 3 * sizeof(float));
    // Skip the first vertex
    for (int i = 1; i < numOfVertices; ++i)
    {
        int off = i * 3;
        for (int j = 0; j < 3; ++j)
        {
            bbox[j] = allCoords[off+j] < bbox[j] ? allCoords[off+j] : bbox[j];
            bbox[j+3] = allCoords[off+j] > bbox[j+3] ? allCoords[off+j] : bbox[j+3];
        }
    }
}


// First attempt to generalize the loadTatou function
// TODO Sanity checks and error messages
// TODO Read/store original coordinates as what they actually are (int16_t), not floats
void loadModel(const char* pakName, int index)
{
    printf("Loading model: %s, %d\n", pakName, index);

    // TODO For sure memory is leaking here.
    uint8_t *data = loadPak(pakName, index);

    // Not sure if this is really signed (or if it matters)
    int16_t flags = *(int16_t *)data;
    // dumpBytes(data, 2);
	data += 2;

    // Bounding Box
    memcpy(bbox, data, 6 * sizeof(int16_t));
    data += 12;

    // Compute center point from bounding box, forced to float
    // X
    bbCenter[0] = (bbox[0] + bbox[1]) / 2.0f;
    // Y
    bbCenter[1] = (bbox[2] + bbox[3]) / 2.0f;
    // Z
    bbCenter[2] = (bbox[4] + bbox[5]) / 2.0f;

    // No idea what this offset is; seems to be zero many times?
	uint16_t offset = *(uint16_t *)data;
	data += 2;

    data += offset;
	numOfVertices = *(uint16_t *)data;
	data += 2;
    // printf("%d\n", numOfVertices);
    
    // We'll reuse the global vertex buffer
    if (allCoords != NULL)
        free(allCoords);
    // Read three coordinates (signed 16 bits each) per point
    // TODO Use memcpy, or some custom specialized function
    allCoords = (float *)malloc(numOfVertices * sizeof(float) * 3);
	for (int i = 0; i < numOfVertices * 3; ++i)
	{
        // Even though allCoords is float (4 bytes), the actual coordinates are stored with two bytes
		allCoords[i] = *(int16_t *)data;
		data += 2;

        // if (i > 0 && i % 3 == 0)
        //     printf("\n");
        // printf("%f", allCoords[i]);
	}

    if (flags & 2)
    {
        printf("Model has bones.\n");

        numBones = *(uint16_t *)data;
        data += 2;

        int sizeOfBoneOffsets = numBones * sizeof(uint16_t);
        
        boneOffsets = (uint16_t *)malloc(sizeOfBoneOffsets);
        memcpy(boneOffsets, data, sizeOfBoneOffsets);
        data += sizeOfBoneOffsets;

        // TODO This routine for reading bones is really messy right now.

        // The 'data' pointer must remain fixed throughout the loop, we only
        //   add the accumulated offset at the end.
        int accumBoneOffset = 0;
        // To be honest I don't really see the point of the offsets... we might
        //   as well just loop over all bones in the file order.
        for (int i = 0; i < numBones; ++i)
        {
            // boneData must be a byte type to simplify pointer arithmetic
            // uint8_t *boneData = data + boneOffsets[i];
            uint8_t *boneData = data;
            if(flags & 8)
            {
                printf("  flag 3 set\n");
                accumBoneOffset += 24;
            }
            else
            {
                // for (int j = 0; j < 8; ++j)
                //     printf("%d ", *(uint16_t *)(boneData + j * 2));
                // printf("\n");
                uint16_t firstVertexOffset = *(uint16_t *)boneData / 2;
                // vec3 firstVertex; 
                // getVertex(allCoords, firstVertexOffset, firstVertex);
                // printf("%f %f %f\n", firstVertex[0], firstVertex[1], firstVertex[2]);

                uint16_t numSubVertex = *(uint16_t *)(boneData + 2);
                                
                uint16_t refVertexOffset = *(uint16_t *)(boneData + 4) / 2;
                // vec3 refVertex; 
                // getVertex(allCoords, *(uint16_t *)(boneData + 4), refVertex);
                // printf("%f %f %f\n", refVertex[0], refVertex[1], refVertex[2]);
                vec3 refVertex = 
                { 
                    allCoords[refVertexOffset], 
                    allCoords[refVertexOffset + 1],
                    allCoords[refVertexOffset + 2]
                };

                uint16_t unknownFlag = *(uint16_t *)(boneData + 6);
                uint16_t type = *(uint16_t *)(boneData + 8);
                // Three rotation angles (I think...?)                
                uint16_t *rotVec3 = (uint16_t *)(boneData + 10);
                
                // printf("bone offset: %d, type: %d\n", boneOffsets[i], type);
                // printf("  rot: %d %d %d\n", rotVec3[0], rotVec3[1], rotVec3[2]);
                // printf("  %d, %d, %d\n", firstVertex, numSubVertex, refVertex);

                // Translate 'numSubVertex' vertices, starting from 'firstVertex',
                //   following 'refVertex'.
                uint16_t curVertexOffset = firstVertexOffset;                 
                for (int j = 0; j < numSubVertex; ++j)
                {
                    allCoords[curVertexOffset] += refVertex[0];
                    allCoords[curVertexOffset + 1] += refVertex[1];
                    allCoords[curVertexOffset + 2] += refVertex[2];
                    curVertexOffset += 3;
                }
                
                // accumBoneOffset += 16;
                data += 16;
            }
        }
        // data += accumBoneOffset;
    }

    numPrim = *(uint16_t *)data;
	data += 2;

    // Reuse the global
    if (allPrims != NULL)
    {
        // TODO Need to free all the malloc'd members too!
        free(allPrims);
    }
    
    allPrims = (Primitive *)malloc(numPrim * sizeof(Primitive));

    dumpModel();
	int abortPrims = 0;
    for (int i = 0; i < numPrim; ++i) 
    {
        Primitive prim;

        // The first byte is always the primitive type
		prim.type = *(data++);
		
        switch (prim.type)
        {
            // Line
            case 0:
                prim.numOfPointInPoly = 2;
			
                // ?
                data++;
                
                prim.colorIndex = *data;
                data++;

                // ?
                data++;

                break;
            
            // Polygon, with variable number of vertices
            case 1:
                prim.numOfPointInPoly = *data;
                data++;

                // NOTE polyType 1 is dithered
                uint8_t polyType = *data;
                data++;

                prim.colorIndex = *data;
                data++;

                break;
            
            // Treat unknown primitives as points, for debugging purposes
            case 6:
            case 7:
                printf("Unsupported primitive type: %d\n", prim.type);
            // Point (1 pixel..?)            
            case 2:            
                prim.numOfPointInPoly = 1;
                
                // ?
                data++;

                prim.colorIndex = *data;
                data++;

                // ?
                data++;
                
                break;

            // Sphere
            case 3:
                prim.numOfPointInPoly = 1;

                // ?
                data++;

                prim.colorIndex = *data;
                data++;
                
                // ?
                data++;

                prim.discSize = *(uint16_t*)data;
                data += 2;

                break;

            default:
                printf("Unsupported primitive type: %d\n", prim.type);
                // This doesn't happen often, but I've found at least primitive types 6 and 7,
                //   which are unknown to me at this point. If that happens, stop reading the
                //   model but do not abort the program.
                abortPrims = 1;
        }

        if (abortPrims)
        {
            // This will limit the model to the supported primitives read so far, which is not
            //   optimal but allows the program to keep working.
            numPrim = i;
            break;
        }

        prim.indices = (uint16_t *)malloc(prim.numOfPointInPoly * sizeof(uint16_t));

        for(int j = 0; j < prim.numOfPointInPoly; j++)
        {
            uint16_t pointIndex = *(uint16_t *)data;
            data += 2;
            
            // The indices here must point to vertices, not coordinates (or bytes),
            //   because they are passed directly to glDrawElements.
            prim.indices[j] = pointIndex / 6;
        }

        allPrims[i] = prim;
	}

    // If the code above fails, everything is a mess of incomplete data.
    // TODO Do everything above with temporary pointers, then memcpy everything if it succeeds.
    //   If it fails, we can still nicely fall back to the previous (current) model.

    mat4x4 M;
    mat4x4_identity(M);
    
    // vec3 c;
    // getCentroid(c);
    // printf("%f %f %f\n", c[0], c[1], c[2]);

    // TODO vectorize
    mat4x4_translate(M, -bbCenter[0], -bbCenter[1], -bbCenter[2]);

    applyMatrix(M, NULL);

    // HERE: Scale the model to fit the screen    
    // for (int i = 0; i < numOfVertices * 3; ++i) 
    // {
    //     allCoords[i] = allCoords[i] / 2000.0f;
    // }
}
