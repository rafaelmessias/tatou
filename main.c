#include <stdio.h>
#include "system.h"
#include "linmath.h"
#include "pak.h"
#include "model.h"


int ModelIndex = 12;


// TODO Make a "model" struct for this
void renderLoop()
{
    vec3 c;
    getCentroid(c);
    float r = getRadius(c);
    // printf("%f\n", r);

    float radPerSec = 2 * M_PI / 5;
    Uint32 ticks = SDL_GetTicks();
    mat4x4 M;

    int isDebugPrim = 0;
    int primHighlight = 0;

    int quit = 0;
    while (!quit)
    {
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
        applyMatrix(M, tmpVbo);

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
                (float) Palette[prim.colorIndex * 3] / 255.0f, 
                Palette[prim.colorIndex * 3 + 1] / 255.0f, 
                Palette[prim.colorIndex * 3 + 2] / 255.0f
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

            // TODO Make an Enum for primitive type
            // if (prim.type == 3)
            // {
                   // TODO Need to scale this correctly before using it 
            //     glPointSize(prim.discSize);
            // }

            GLenum mode;
            switch (prim.type)
            {
                case 0:
                    mode = GL_LINES;
                    break;

                case 1:
                    mode = GL_TRIANGLE_FAN;
                    break;

                case 2:
                    // TODO Actually a sphere, or disc
                    mode = GL_POINTS;
                    break;

                case 3:
                    mode = GL_POINTS;
                    break;

                default:
                    // For absolutely no reason at all
                    mode = GL_LINE_LOOP;
                    break;
            }
            
            // NOTE prim.indices is malloc'd, so 'sizeof(prim.indices)' doesn't work
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.numOfPointInPoly * 2, prim.indices, GL_STATIC_DRAW);            
            glDrawElements(mode, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);
        }

        if (isDebugPrim)
        {
            Primitive prim = allPrims[primHighlight]; 
            glUniform4f(ColorLocation, 1.0f, 1.0f, 1.0f, 0.5f);
            // glEnable(GL_POINT_SMOOTH);
            // glEnable(GL_BLEND);
            // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
            glPointSize(5.0f);
            glDepthFunc(GL_ALWAYS);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.numOfPointInPoly * 2, prim.indices, GL_STATIC_DRAW);        
            glDrawElements(GL_POINTS, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);
            glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);
            glDepthFunc(GL_LESS);
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
        while (SDL_PollEvent(&e) != 0) 
        {
            if (e.type == SDL_QUIT) 
            {
                quit = 1;
            }
            if (e.type == SDL_KEYDOWN)
            {
                Primitive p;
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;

                    case SDLK_RIGHT:
                        // TODO Must check boundaries here
                        loadModel("LISTBODY", ++ModelIndex);
                        primHighlight = 0;
                        break;

                    case SDLK_LEFT:
                        if (ModelIndex > 0)
                        {
                            loadModel("LISTBODY", --ModelIndex);
                            primHighlight = 0;
                        }                        
                        break;

                    case SDLK_s:
                        if (primHighlight < numPrim - 1)
                        {
                            primHighlight++;
                            printf("Highlighted primitive: %d\n", primHighlight);
                            // p = allPrims[primHighlight];
                            // printf("  Type: %d\n", p.type);
                            // printf("  Color: %d\n", p.colorIndex);
                            // printf("  Num. pts: %d\n", p.numOfPointInPoly);
                            // for (int i = 0; i < p.numOfPointInPoly; ++i)
                            // {
                            //     printf(" %d", p.indices[i]);
                            // }
                            // printf("\n");
                        }
                        break;

                    case SDLK_a:
                        if (primHighlight > 0)
                        {
                            primHighlight--;
                            printf("Highlighted primitive: %d\n", primHighlight);
                        }
                        break;

                    case SDLK_d:
                        isDebugPrim = !isDebugPrim;
                        break;
                }

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


void loadTatou() 
{    
    loadPalette(TATOU_PAL);
    loadModel("ITD_RESS", 0);
}


int main(int argv, char* argc[]) {
    initAll();    

    initRenderer();

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    loadPalette(DEFAULT_PAL);
    loadModel("LISTBOD2", ModelIndex);

    // loadTatou();

    renderLoop();
}
