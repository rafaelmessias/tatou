#include <stdlib.h>
#include "model.h"
#include "pak.h"


float *allCoords = NULL;
int numOfVertices;
Primitive *allPrims = NULL;
int numPrim;


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
// Maybe the (min+max)/2 is better than the centroid?
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


// First attempt to generalize the loadTatou function
// TODO Sanity checks and error messages
// TODO Read/store original coordinates as what they actually are (int16_t), not floats
void loadModel(const char* pakName, int index)
{
    printf("Loading model: %s, %d\n", pakName, index);
    // fflush(stdout);
    // TODO For sure memory is leaking here.
    uint8_t *data = loadPak(pakName, index);

    // Not sure if this is really signed (or if it matters)
    int16_t flags = *(int16_t *)data;
	data += 2;

    // Bones?
    // if (flags & 2)
    // {
    //     printf("Model has bones.\n");
    //     return;
    // }

    // Not sure what is here, but I think there should be a bounding box in the start of the file somewhere
    data += 12;

    // No idea what this offset is
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

        const uint16_t numBones = *(uint16_t *)data;
        data += 2;
        
        uint16_t boneOffsets[numBones];
        memcpy(boneOffsets, data, sizeof(boneOffsets));
        data += sizeof(boneOffsets);

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
        data += accumBoneOffset;
    }

    numPrim = *(uint16_t *)data;
	data += 2;

    // printf("numprim: %d\n", numPrim);

    // Reuse the global
    if (allPrims != NULL)
    {
        // TODO Need to free all the malloc'd members too!
        free(allPrims);
    }
    allPrims = (Primitive *)malloc(numPrim * sizeof(Primitive));
	for (int i = 0; i < numPrim; ++i) 
    {
        Primitive prim;
		prim.type = *(data++);
		if (prim.type == 0) 
        {
            prim.numOfPointInPoly = 2;
			data++;
			prim.colorIndex = *data;
			data++;
			data++;

            prim.indices = (uint16_t*)malloc(2 * sizeof(uint16_t)); 

			prim.indices[0] = *(uint16_t*)data;
    		data+=2;

			prim.indices[1] = *(uint16_t*)data;
    		data+=2;

            // NOTE The +1 is because OBJ files are 1-indexed
			//printf("l %d %d\n", i, pointIndex1 / 6 + 1, pointIndex2 / 6 + 1);
		} 
        else if (prim.type == 1) 
        {
			prim.numOfPointInPoly = *data;
  			data++;

            // polyType 1 is dithered
  			uint8_t polyType = *data;
			data++;

  			prim.colorIndex = *data;
			data++;

            prim.indices = (uint16_t *)malloc(prim.numOfPointInPoly * sizeof(uint16_t));

			for(int j = 0; j < prim.numOfPointInPoly; j++)
			{
				uint16_t pointIndex = *(uint16_t *)data;
				data += 2;
                // NOTE The +1 is because OBJ files are 1-indexed
				//printf("%d ", pointIndex / 6 + 1);
                prim.indices[j] = pointIndex / 6;
			}
		} 
        else if (prim.type == 2) 
        {
            prim.numOfPointInPoly = 1;
            data++;
            prim.colorIndex = *data;
            data++;
            data++;
            // prim.discSize = *(uint16_t*)data;
            // data += 2;
            prim.indices = (uint16_t *)malloc(sizeof(uint16_t));
            *prim.indices = *(uint16_t*)data;
            data += 2;
		}
        else if (prim.type == 3)
        {
            prim.numOfPointInPoly = 1;
            data++;
            prim.colorIndex = *data;
            data++;
            data++;
            prim.discSize = *(uint16_t*)data;
            data += 2;
            prim.indices = (uint16_t *)malloc(sizeof(uint16_t));
            *prim.indices = *(uint16_t*)data;
            data += 2;
        }
        allPrims[i] = prim;
	}

    // If the code above fails, everything is a mess of incomplete data.
    // TODO Do everything above with temporary pointers, then memcpy everything if it succeeds.
    //   If it fails, we can still nicely fall back to the previous (current) model.

    mat4x4 M;
    mat4x4_identity(M);
    
    vec3 c;
    getCentroid(c);
    // printf("%f %f %f\n", c[0], c[1], c[2]);

    // TODO vectorize
    mat4x4_translate(M, -c[0], -c[1], -c[2]);

    applyMatrix(M, NULL);

    // HERE: Scale the model to fit the screen    
    // for (int i = 0; i < numOfVertices * 3; ++i) 
    // {
    //     allCoords[i] = allCoords[i] / 2000.0f;
    // }
}
