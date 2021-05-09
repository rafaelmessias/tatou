#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"


#define X_RES 800
#define Y_RES 600


extern SDL_Window *Window;
extern GLuint Vbo, Vao, Ebo;
extern GLint ColorLocation;
extern uint8_t Palette[256 * 3];
extern GLchar *vertexSrc, *fragSrc;


void initAll();
void initRenderer();
