#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"


#define X_RES 960
#define Y_RES 720

#define X_INT 320
#define Y_INT 240

#define DEFAULT_PAL 0
#define TATOU_PAL 1


extern GLuint Vao, Vbo, Fbo;
extern SDL_Window *Window;
extern GLint ColorLocation, IsPointLoc;
extern uint8_t Palette[256 * 3];
extern GLchar *vertexSrc, *fragSrc;


void initAll();
void initRenderer();
void loadPalette(int paletteId);
