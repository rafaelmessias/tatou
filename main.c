#include <stdio.h>
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"

#include "linmath.h"
#include "pak.h"


#define X_RES 800
#define Y_RES 600

#define u8 unsigned char
#define u16 unsigned short int
#define s16 short int


#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SWAP16(X)    (X)
#define SWAP32(X)    (X)
#else
#define SWAP16(X)    SDL_Swap16(X)
#define SWAP32(X)    SDL_Swap32(X)
#endif


SDL_Window *Window;
GLuint Vbo, Vao, Ebo;
GLint ColorLocation;

typedef struct {
    u8 type;
    u8 colorIndex;
    u16* indices;
    u8 numOfPointInPoly;
    GLenum mode;
} Primitive;


// in_Position was bound to attribute index 0
// We output the ex_Color variable to the next shader in the chain (white)
const GLchar* vertexSrc = \
    "#version 150\n\
    in vec3 in_Position;\
    void main(void) {\
        gl_Position = vec4(in_Position, 1.0);\
    }";

const GLchar* fragSrc = \
    "#version 150\n\
    precision highp float;\
    uniform vec4 color;\
    out vec4 fragColor;\
    void main(void) {\
        fragColor = color;\
    }";


// u8* readFile(const char *filename) {
// 	FILE *f = fopen(filename, "rb");
// 	fseek(f, 0, SEEK_END);
// 	long fsize = ftell(f);
// 	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
// 	u8* data = (u8*)malloc(fsize);
// 	fread(data, 1, fsize, f);
// 	fclose(f);
// 	return data;
// }


void initAll() {
    int isInit = SDL_Init(SDL_INIT_VIDEO);
    if (isInit < 0) {
        printf("Couldn't initialize SDL Video: %s\n", SDL_GetError());
        exit(isInit);
    }
    
    Window = SDL_CreateWindow(
        "SDL Tutorial", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        X_RES,
        Y_RES, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (Window == NULL) {
        printf("Couldn't create a window with SDL: %s\n", SDL_GetError());
        exit(-1);
    }

    //Use OpenGL 3.1 core
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_GLContext *context = SDL_GL_CreateContext(Window);
    if (context == NULL) {
        printf("Couldn't create a GL context: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_GL_MakeCurrent(Window, context);
    
    //Initialize GLEW
    glewExperimental = GL_TRUE; 
    GLenum isGlewInit = glewInit();
    if( isGlewInit != GLEW_OK ) {
        printf( "Couldn't initialize GLEW: %s\n", glewGetErrorString(isGlewInit));
        exit(-1);
    }
}


// TODO Just for fun, make these computations vectorized...?
// Maybe the (min+max)/2 is better than the centroid?
void getCentroid(vec3 r, float* allCoords, int numOfVertices) {
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


void applyMatrix(mat4x4 M, float* allCoords, float* out, int numOfVertices)
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


float getRadius(float* allCoords, int numOfVertices, vec3 centroid) {
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


// TODO Make a "model" struct for this
void renderLoop(float *allCoords, int numOfVertices, Primitive *allPrims, int numPrim, u8 *palette)
{
    mat4x4 M;
    mat4x4_identity(M);
    
    vec3 c;
    getCentroid(c, allCoords, numOfVertices);
    // printf("%f %f %f\n", c[0], c[1], c[2]);

    float r = getRadius(allCoords, numOfVertices, c);
    // printf("%f\n", r);

    // TODO vectorize
    mat4x4_translate(M, -c[0], -c[1], -c[2]);

    applyMatrix(M, allCoords, NULL, numOfVertices);

    // HERE: Scale the model to fit the screen    
    // for (int i = 0; i < numOfVertices * 3; ++i) 
    // {
    //     allCoords[i] = allCoords[i] / 2000.0f;
    // }

    float radPerSec = 2 * M_PI / 5;
    Uint32 ticks = SDL_GetTicks();

    int quit = 0;
    while (!quit)
    {
        // mat4x4 M;
        mat4x4_identity(M);

        // Last: Correct aspect ratio by shrinking the X coordinate.
        // OpenGL always draws from -1 to 1 in the three axes. Since the screen
        //   is longer in the X (horizontal) direction, things are stretched in
        //   this axis. Thus, we shrink them accordingly.
        // We do this by setting up a scaling matrix for the X axis only.
        M[0][0] = (float) Y_RES / (float) X_RES;

        Uint32 newTicks = SDL_GetTicks();
        
        // Third: Rotate.
        // How many seconds ellapsed since the first frame, times the desired
        //   angular speed.
        // This will increase unbounded, but it's a rotation so it doesn't
        //   matter (trigonometry fixes it).
        // If I checked the time ellapsed since the last frame I'd have to keep
        //   track of the current angle and add to it, because the model is 
        //   reset for each frame.
        mat4x4_rotate_Y(M, M, ((newTicks - ticks) / 1000.0f) * radPerSec);
        mat4x4_rotate_Z(M, M, M_PI);
        // mat4x4_rotate_X(M, M, M_PI / 2);
        
        // Second: Scale to something less than the unit sphere, so that the
        //   entire model fits into the screen given any rotation.
        mat4x4_scale(M, M, 0.95f / r);
        // M[3][3] = 1.0;

        // First: translate to origin
        // Why doesn't this work...?
        // mat4x4_translate(M, -c[0], -c[1], -c[2]);
        
        float tmpVbo[numOfVertices * 3];
        applyMatrix(M, allCoords, tmpVbo, numOfVertices);

        // for (int i = 0; i < numOfVertices; ++i) {
        //     int off = i * 3;
        //     printf("%f %f %f\n", tmpVbo[off], tmpVbo[off+1], tmpVbo[off+2]);
        // }
        // exit(0);

        // getCentroid(c, tmpVbo, numOfVertices);
        // printf("%f %f %f\n", c[0], c[1], c[2]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Send the vertices to the GPU
        glBufferData(GL_ARRAY_BUFFER, sizeof(tmpVbo), tmpVbo, GL_STATIC_DRAW);

        for (int i = 0; i < numPrim; ++i) 
        {
            Primitive prim = allPrims[i];
            // float g = (float) rand() / (float) RAND_MAX;
            vec3 color = {
                (float) palette[prim.colorIndex * 3] / 255.0f, 
                palette[prim.colorIndex * 3 + 1] / 255.0f, 
                palette[prim.colorIndex * 3 + 2] / 255.0f
            };
            glUniform4f(ColorLocation, color[0], color[1], color[2], 1.0f);
            // printf("%f %f %f\n", color[0], color[1], color[2]);
            // printf("%d\n", prim.numOfPointInPoly);
            // for (int j = 0; j < prim.numOfPointInPoly; ++j) {
            //     int off = prim.indices[j];
            //     vec3 v = {tmpVbo[off], tmpVbo[off+1], tmpVbo[off+2]};
            //     printf("  %d %f %f %f\n", off, v[0], v[1], v[2]);
            // }

            // This only works on Windows (didn't test Linux)
            // glDrawElements(prim.mode, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);

            // It turns out that OSX cannot use CPU-based indices with glDrawElements,
            //   so they need to be moved to the GPU.
            
            // TODO This is really inefficient, but I don't know how to fix it.

            // NOTE prim.indices is malloc'd, so 'sizeof(prim.indices)' doesn't work
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.numOfPointInPoly * 2, prim.indices, GL_STATIC_DRAW);            
            glDrawElements(prim.mode, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);
        }

        /* Sleep */
        // SDL_Delay(1000);

        // Since I am using VSYNC, this will "wait" as long as necessary
        SDL_GL_SwapWindow(Window);

        // SDL_Delay(1000.0f/60.0f);

        // Read all events from the queue before the next frame.
        // IMPORTANT: On OSX, nothing happens (i.e. no window is shown) until
        //   the event loop starts! (I guess some events need to be polled?)
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }
    }
}


// Offsets from Pak files always come in intervals of 6 bytes, because they
//   point to the beginning of a vertex, which is 6 bytes long. I divide them
//   by 2 because I need them in intervals of 3, since there are three
//   consecutive floats per vertex in my buffer.
// void getVertex(float *vertexBuffer, u16 offset, vec3 out)
// {    
//     out[0] = vertexBuffer[offset/2];
//     out[1] = vertexBuffer[offset/2+1];
//     out[2] = vertexBuffer[offset/2+2];
// }


// First attempt to generalize the loadTatou function
// TODO Sanity checks and error messages
// TODO Read/store original coordinates as what they actually are (s16), not floats
void loadModel(const char* pakName, int index)
{
    u8 *data = loadPak(pakName, index);

    u8 *palette = loadPak("ITD_RESS", 3);
    // palette += 2;

    // Not sure if this is really signed (or if it matters)
    s16 flags = *(s16 *)data;
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
	u16 offset = *(u16 *)data;
	data += 2;

    data += offset;
	u16 numOfVertices = *(u16 *)data;
	data += 2;
    // printf("%d\n", numOfVertices);
    
    // Read three coordinates (signed 16 bits each) per point
    // TODO Use memcpy, or some custom specialized function
    float allCoords[numOfVertices * 3];
	for (int i = 0; i < numOfVertices * 3; ++i)
	{
        // Even though allCoords is float (4 bytes), the actual coordinates are stored with two bytes
		allCoords[i] = *(s16 *)data;
		data += 2;

        // if (i > 0 && i % 3 == 0)
        //     printf("\n");
        // printf("%f", allCoords[i]);
	}

    if (flags & 2)
    {
        // printf("Model has bones.\n");

        u16 numBones = *(u16 *)data;
        data += 2;
        
        u16 boneOffsets[numBones];
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
            u8 *boneData = data + boneOffsets[i];
            if(flags & 8)
            {
                printf("  flag 3 set\n");
                accumBoneOffset += 24;
            }
            else
            {
                // for (int j = 0; j < 8; ++j)
                //     printf("%d ", *(u16 *)(boneData + j * 2));
                // printf("\n");
                u16 firstVertexOffset = *(u16 *)boneData / 2;
                // vec3 firstVertex; 
                // getVertex(allCoords, firstVertexOffset, firstVertex);
                // printf("%f %f %f\n", firstVertex[0], firstVertex[1], firstVertex[2]);

                u16 numSubVertex = *(u16 *)(boneData + 2);
                                
                u16 refVertexOffset = *(u16 *)(boneData + 4) / 2;
                // vec3 refVertex; 
                // getVertex(allCoords, *(u16 *)(boneData + 4), refVertex);
                // printf("%f %f %f\n", refVertex[0], refVertex[1], refVertex[2]);
                vec3 refVertex = 
                { 
                    allCoords[refVertexOffset], 
                    allCoords[refVertexOffset + 1],
                    allCoords[refVertexOffset + 2]
                };

                u16 unknownFlag = *(u16 *)(boneData + 6);
                u16 type = *(u16 *)(boneData + 8);
                // Three rotation angles (I think...?)                
                u16 *rotVec3 = (u16 *)(boneData + 10);
                
                printf("bone offset: %d, type: %d\n", boneOffsets[i], type);
                // printf("  %d, %d, %d\n", firstVertex, numSubVertex, refVertex);

                // Translate 'numSubVertex' vertices, starting from 'firstVertex',
                //   following 'refVertex'.
                u16 curVertexOffset = firstVertexOffset;                 
                for (int j = 0; j < numSubVertex; ++j)
                {
                    allCoords[curVertexOffset] += refVertex[0];
                    allCoords[curVertexOffset + 1] += refVertex[1];
                    allCoords[curVertexOffset + 2] += refVertex[2];
                    curVertexOffset += 3;
                }
                
                accumBoneOffset += 16;
            }
            // exit(0);
        }
        data += accumBoneOffset;

        // exit(0);
    }

    u16 numPrim = *(u16 *)data;
	data += 2;

    // printf("numprim: %d\n", numPrim);

    Primitive allPrims[numPrim];
	for (int i = 0; i < numPrim; ++i) {
        Primitive prim;
		prim.type = *(data++);
		if (prim.type == 0) {
            // printf("type 0");
            prim.mode = GL_LINES;
            prim.numOfPointInPoly = 2;
			data++;
			prim.colorIndex = *data;
			data++;
			data++;

            // u16 indices[2];
            prim.indices = (u16*)malloc(2 * sizeof(u16)); 

			prim.indices[0] = *(u16*)data / 6;
    		data+=2;

			prim.indices[1] = *(u16*)data / 6;
    		data+=2;

            // prim.indices = indices;

            // glDrawElements expects indices to vertices, but the RES file 
            // provides them as byte offsets (6 bytes per vertex)
            //u16 indices[] = {pointIndex1 / 6, pointIndex2 / 6};

            // glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, prim.indices);

            // NOTE The +1 is because OBJ files are 1-indexed
			//printf("l %d %d\n", i, pointIndex1 / 6 + 1, pointIndex2 / 6 + 1);
            // glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);
		} else if (prim.type == 1) {
            // printf("type 1\n");
            prim.mode = GL_TRIANGLE_FAN;
			prim.numOfPointInPoly = *data;
  			data++;

            // polyType 1 is dithered
  			u8 polyType = *data;
			data++;

  			prim.colorIndex = *data;
			data++;

            // u16 indices[prim.numOfPointInPoly];
            prim.indices = (u16 *)malloc(prim.numOfPointInPoly * sizeof(u16));

			// printf("%d %d %d\n", prim.numOfPointInPoly, polyType, prim.colorIndex);
  			
			//printf("f %d, ", numOfPointInPoly);
			for(int j = 0; j < prim.numOfPointInPoly; j++)
			{
				u16 pointIndex = *(u16 *)data;
				//printf("  %d\n", pointIndex);
				data += 2;
				if ((pointIndex % 6) != 0) {
					//printf("not divisible by 6");
					exit(0);
				}
                // NOTE The +1 is because OBJ files are 1-indexed
				//printf("%d ", pointIndex / 6 + 1);
                prim.indices[j] = pointIndex / 6;
			}
            // prim.indices = indices;

            // glDrawElements(GL_TRIANGLE_FAN, numOfPointInPoly, GL_UNSIGNED_SHORT, indices);
            // glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);
		} else {
			printf("Unknown primitive type: %d.\n", prim.type);
			exit(-1);
		}
        // glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);
        allPrims[i] = prim;        
	}

    renderLoop(allCoords, numOfVertices, allPrims, numPrim, palette);
    exit(0);
}


void loadTatou() 
{    
    // u8 *data = readFile("ITD_RESS.PAK_0.dat");
    u8 *data = loadPak("ITD_RESS", 0);
	data += 14;

    // u8 *palette = readFile("ITD_RESS.PAK_2.dat");
    u8 *palette = loadPak("ITD_RESS", 2);
    palette += 2;
    
    // for (int i = 0; i < 256; ++i) {
    //     int offset = i * 3;
    //     printf("%d %d %d\n", palette[offset], palette[offset+1], palette[offset+2]);
    // }
    // exit(0);
	
    // No idea what this offset is
	u16 offset = *(u16 *)data;
	data += 2;

	data += offset;
	u16 numOfVertices = *(u16 *)data;
	data += 2;

    float allCoords[numOfVertices * 3];
    // Read three coordinates (signed 16 bits each) per point
	for (int i = 0; i < numOfVertices * 3; ++i)
	{
        s16 c = *(s16 *)data;
		allCoords[i] = c;
		data+=2;
	}

	u16 numPrim = *(u16 *)data;
	data += 2;
	//printf("numPrim %d", numPrim);

    Primitive allPrims[numPrim];
	for (int i = 0; i < numPrim; ++i) {
        Primitive prim;
		prim.type = *(data++);
		if (prim.type == 0) {
            prim.mode = GL_LINES;
            prim.numOfPointInPoly = 2;
			data++;
			prim.colorIndex = *data;
			data++;
			data++;

            // u16 indices[2];
            prim.indices = (u16*)malloc(2 * sizeof(u16)); 

			prim.indices[0] = *(u16*)data / 6;
    		data+=2;

			prim.indices[1] = *(u16*)data / 6;
    		data+=2;

            // prim.indices = indices;

            // glDrawElements expects indices to vertices, but the RES file 
            // provides them as byte offsets (6 bytes per vertex)
            //u16 indices[] = {pointIndex1 / 6, pointIndex2 / 6};

            // glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, prim.indices);

            // NOTE The +1 is because OBJ files are 1-indexed
			//printf("l %d %d\n", i, pointIndex1 / 6 + 1, pointIndex2 / 6 + 1);
            // glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);
		} else if (prim.type == 1) {
            prim.mode = GL_TRIANGLE_FAN;
			prim.numOfPointInPoly = *data;
  			data++;

  			u8 polyType = *data;
			data++;

  			prim.colorIndex = *data;
			data++;

            // u16 indices[prim.numOfPointInPoly];
            prim.indices = (u16*)malloc(prim.numOfPointInPoly * sizeof(u16));

			// printf("%d %d %d\n", numOfPointInPoly, polyType, polyColor);
  			
			//printf("f %d, ", numOfPointInPoly);
			for(int j = 0; j < prim.numOfPointInPoly; j++)
			{
				u16 pointIndex = *(u16*)data;
				//printf("  %d\n", pointIndex);
				data += 2;
				if ((pointIndex % 6) != 0) {
					//printf("not divisible by 6");
					exit(0);
				}
                // NOTE The +1 is because OBJ files are 1-indexed
				//printf("%d ", pointIndex / 6 + 1);
                prim.indices[j] = pointIndex / 6;
			}
            // prim.indices = indices;

            // glDrawElements(GL_TRIANGLE_FAN, numOfPointInPoly, GL_UNSIGNED_SHORT, indices);
            // glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);
		} else {
			printf("Unknown primitive type: %d.\n", prim.type);
			exit(-1);
		}
        // glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, prim.indices);
        allPrims[i] = prim;
	}

    renderLoop(allCoords, numOfVertices, allPrims, numPrim, palette);
    exit(0);
}


int main(int argv, char* argc[]) {
    initAll();    

    static GLfloat vertices[] = { 
		-1.0, -1.0, 0.0,
        -1.0,  1.0, 0.0,
         1.0,  1.0, 0.0,
         1.0, -1.0, 0.0
	};
    int numVertices = 12;
    
    // TODO The Vbo and Vao here do not need to be global... maybe model-specific?

    // Generate a vertex buffer in GPU memory, make it active, then copy the vertices to it
    glGenBuffers(1, &Vbo);
    glBindBuffer(GL_ARRAY_BUFFER, Vbo);

    // Generate a vertex array, make it active, and specify it
	glGenVertexArrays(1, &Vao);
	glBindVertexArray(Vao);
    // A vertex array describes a vertex buffer: here it says "position 0, with 3 floats per vertex"
    // The position refers to how it will be accessed by the shaders later
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // Enabling is different than binding it (i.e. making it active). Only one
    //   VAO can be bound at any time, but multiple can be enabled (and used at
    //   the same time by shaders).
    glEnableVertexAttribArray(0);

    // NOTE As soon as this Ebo is bound, glDrawElements will expect it to be used, so make sure it works.
    glGenBuffers(1, &Ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Ebo);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)&vertexSrc, 0);
    glCompileShader(vertexShader);

    int IsCompiled_VS, maxLength;
    char *vertexInfoLog;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &IsCompiled_VS);
    if(IsCompiled_VS == GL_FALSE)
    {
       glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

       /* The maxLength includes the NULL character */
       vertexInfoLog = (char *)malloc(maxLength);

       glGetShaderInfoLog(vertexShader, maxLength, &maxLength, vertexInfoLog);

       /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
       printf("%s", vertexInfoLog);
       /* In this simple program, we'll just leave */
       free(vertexInfoLog);
       return -1;
    }

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, (const GLchar**)&fragSrc, 0);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &IsCompiled_VS);
    if(IsCompiled_VS == GL_FALSE)
    {
       glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &maxLength);

       /* The maxLength includes the NULL character */
       vertexInfoLog = (char *)malloc(maxLength);

       glGetShaderInfoLog(fragShader, maxLength, &maxLength, vertexInfoLog);

       /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
       printf("%s", vertexInfoLog);
       /* In this simple program, we'll just leave */
       free(vertexInfoLog);
       return -1;
    }

    GLuint shaderProg = glCreateProgram();
    glAttachShader(shaderProg, vertexShader);
    glAttachShader(shaderProg, fragShader);
    // This connects VAO position 0 (set above) to the shader variable in_Position
    glBindAttribLocation(shaderProg, 0, "in_Position");
    glLinkProgram(shaderProg);
    ColorLocation = glGetUniformLocation(shaderProg, "color");
    glUseProgram(shaderProg);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    SDL_GL_SetSwapInterval(1);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // loadTatou();
    loadModel("LISTBODY", 1);

    exit(0);

    // The code below is just a quad, for testing when something goes wrong.

    //Main loop flag
    int quit = 0;

    //Event handler
    SDL_Event e;

    //While application is running
    while( !quit )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4x4 M;
        mat4x4_identity(M);

        // Correct aspect ratio
        M[0][0] = (float) Y_RES / (float) X_RES;
        // M[0][0] = (float) rand() / (float) RAND_MAX;

        float tmpVbo[12];
        applyMatrix(M, vertices, tmpVbo, 12);

        glBufferData(GL_ARRAY_BUFFER, sizeof(tmpVbo), tmpVbo, GL_STATIC_DRAW);

        float r = (float) rand() / RAND_MAX;
        printf("%f ", r);
        glUniform4f(ColorLocation, r, 0.0f, 0.0f, 1.0f);

        // Feed the shaders with 4 points, starting from 0
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        /* Swap our buffers to make our changes visible */
        SDL_GL_SwapWindow(Window);

        //Handle events on queue
        while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
                quit = 1;
            }
        }

        /* Sleep */
        // SDL_Delay(1.0f / 30.0f);
    }


    // NOTE The indices point to vertices, not coordinates! What exactly is a vertex is defined in the VAO.
    // u16 indices[] = {0,1,2,3};
    // glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, indices);
    // glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, indices);

    /* Swap our buffers to make our changes visible */
    // SDL_GL_SwapWindow(Window);

    /* Sleep for 2 seconds */
    // SDL_Delay(2000);


    // float vertices2[numVertices];
    // for (int i = 0; i < numVertices; ++i) {
    //     vertices2[i] = 0.9 * vertices[i];
    // }
    //printf("%f \n", vertices2[0]);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    // glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, indices);

    /* Swap our buffers to make our changes visible */
    // SDL_GL_SwapWindow(Window);

    /* Sleep for 2 seconds */
    // SDL_Delay(2000);


    /* Swap our buffers to make our changes visible */
    // SDL_GL_SwapWindow(Window);

    /* Sleep for 2 seconds */
    // SDL_Delay(2000);

}
