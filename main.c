#include <stdio.h>
#include "system.h"
#include "linmath.h"
#include "pak.h"
#include "model.h"
#include "misc.h"


int ModelIndex = 263;
int primHighlight = 0;
int isDebugPrim = 0;


// TODO Make a "model" struct for this
void renderLoop()
{
    float radPerSec = 2 * M_PI / 5;
    Uint32 ticks = SDL_GetTicks();
    mat4x4 M;

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

        // The negative here is because the models are upside down, at first,
        //   then Z-rotated 180 deg.
        // TODO The renderer shouldn't know this; the model should fix itself
        mat4x4_rotate_X(M, M, -M_PI / 8);
        
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
        
        // Second: Scale to something less than the unit sphere, so that the
        //   entire model fits into the screen given any rotation.
        // mat4x4_scale(M, M, 0.95f / r);
        // M[3][3] = 1.0;

        // First: translate to origin
        // Why doesn't this work...?
        // mat4x4_translate(M, -c[0], -c[1], -c[2]);
        
        float tmpVbo[numOfVertices * 3];
        applyMatrix(M, allCoords, numOfVertices, tmpVbo);

        // for (int i = 0; i < numOfVertices; ++i) {
        //     int off = i * 3;
        //     printf("%f %f %f\n", tmpVbo[off], tmpVbo[off+1], tmpVbo[off+2]);
        // }
        // exit(0);

        // getCentroid(c, tmpVbo, numOfVertices);
        // printf("%f %f %f\n", c[0], c[1], c[2]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Send the vertices to the GPU
        // NOTE One important thing to do when programming with OpenGL is to avoid ambiguity, in the name of sanity (e.g. for easier debugging)
        //   Ex.: I might not have to bind this buffer everytime, but by doing this I know for sure I'm using the right buffer
        glBindBuffer(GL_ARRAY_BUFFER, Vbo);
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

            GLenum mode;
            switch (prim.type)
            {
                case PRIM_LINE:
                    mode = GL_LINES;
                    break;

                case PRIM_POLY:
                    mode = GL_TRIANGLE_FAN;
                    break;

                case PRIM_CIRCLE:
                    // TODO Actually a sphere, or disc
                    glPointSize(10.0f);
                    mode = GL_POINTS;
                    break;

                case PRIM_POINT:
                    // Just to make sure the size is correct
                    glPointSize(1.0f);
                    mode = GL_POINTS;
                    break;

                default:
                    // For absolutely no reason at all
                    mode = GL_LINE_LOOP;
                    break;
            }

            // Debug
            // glPointSize(1.0f);
            // glDrawElements(GL_POINTS, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);

            // TODO Apparently, the Vao must be bound before the Ebo data is buffered
            glBindVertexArray(Vao);

            // It turns out that OSX cannot use CPU-based indices with glDrawElements,
            //   so they need to be moved to the GPU, to an element buffer.
            
            // NOTE prim.indices is malloc'd, so 'sizeof(prim.indices)' doesn't work
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.numOfPointInPoly * 2, prim.indices, GL_STATIC_DRAW);

            glDrawElements(mode, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);

            // NOTE Although this doesn't matter much, it's a good practice to unbind in order to avoid inadvertently
            //   changing something in the VAO in some other part of the code. However, this also means that you
            //   cannot *assume* that it's bound; whenever you need to use it, you must explicitly bind it.
            glBindVertexArray(0);
        }

        // TODO Move this to inside the previous loop (with an if), to avoid re-buffering the elements
        if (isDebugPrim)
        {
            Primitive prim = allPrims[primHighlight]; 
            glUniform4f(ColorLocation, 1.0f, 1.0f, 1.0f, 0.5f);
            glPointSize(2.0f);
            glDepthFunc(GL_ALWAYS);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.numOfPointInPoly * 2, prim.indices, GL_STATIC_DRAW);        
            glDrawElements(GL_POINTS, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);
            glDrawElements(GL_LINE_LOOP, prim.numOfPointInPoly, GL_UNSIGNED_SHORT, 0);
            glDepthFunc(GL_LESS);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, Fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        // glDrawBuffer(GL_BACK);
        glBlitFramebuffer(
            0, 0, X_INT, Y_INT,
            0, 0, X_RES, Y_RES,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST);

        // glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        // GLubyte pixels[320*200*4];
        // glReadPixels(0, 0, 320, 200, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
        // dumpBytes(pixels, 320*200*4, 4 * 12);
        // exit(0);

        // glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Since I am using VSYNC, this will "wait" as long as necessary
        SDL_GL_SwapWindow(Window);

        glBindFramebuffer(GL_FRAMEBUFFER, Fbo);

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
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;

                    case SDLK_RIGHT:
                        // TODO Must check boundaries here
                        loadModel("LISTBODY", ++ModelIndex);
                        primHighlight = 0;
                        // dumpModel();
                        break;

                    case SDLK_LEFT:
                        if (ModelIndex > 0)
                        {
                            loadModel("LISTBODY", --ModelIndex);
                            primHighlight = 0;                            
                            // dumpModel();
                        }                        
                        break;

                    case SDLK_s:
                        if (isDebugPrim && primHighlight < numPrim - 1)
                        {
                            primHighlight++;
                            printf("Highlighted primitive: %d\n", primHighlight);
                            dumpPrim(primHighlight);
                        }
                        break;

                    case SDLK_a:
                        if (isDebugPrim && primHighlight > 0)
                        {
                            primHighlight--;
                            printf("Highlighted primitive: %d\n", primHighlight);
                            dumpPrim(primHighlight);
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
