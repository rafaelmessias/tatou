#include "system.h"
#include "pak.h"


SDL_Window *Window;
GLuint Vbo, Vao, Ebo;
GLint ColorLocation;
uint8_t Palette[256 * 3];


// in_Position was bound to attribute index 0
// We output the ex_Color variable to the next shader in the chain (white)
GLchar* vertexSrc = \
    "#version 150\n\
    in vec3 in_Position;\
    void main(void) {\
        gl_Position = vec4(in_Position, 1.0);\
    }";

GLchar* fragSrc = \
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
    
    //Use OpenGL 3.1 core
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    // glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

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

    SDL_GLContext *context = SDL_GL_CreateContext(Window);
    if (context == NULL) {
        printf("Couldn't create a GL context: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_GL_MakeCurrent(Window, context);

    // SDL_SetWindowFullscreen(Window, SDL_WINDOW_FULLSCREEN);
    
    //Initialize GLEW
    glewExperimental = GL_TRUE; 
    GLenum isGlewInit = glewInit();
    if( isGlewInit != GLEW_OK ) {
        printf( "Couldn't initialize GLEW: %s\n", glewGetErrorString(isGlewInit));
        exit(-1);
    }

    // Auto-flush stdout
    setvbuf(stdout, NULL, _IONBF, 0);
}


void initRenderer()
{    
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
       exit(-1);
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
       exit(-1);
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
    glEnable(GL_POINT_SMOOTH);
    // glEnable(GL_CULL_FACE);
    SDL_GL_SetSwapInterval(1);
}


// Maybe use an enum?
void loadPalette(int paletteId) {
    uint8_t *tmpPal;
    if (paletteId == TATOU_PAL)
    {
        tmpPal = loadPak("ITD_RESS", 2);
        tmpPal += 2;
    }
    else
    {
        tmpPal = loadPak("ITD_RESS", 3);
    }
    // The global Palette is a fixed-size array, but loadPak returns a heap
    //   pointer, so we have to handle that
    memcpy(Palette, tmpPal, sizeof(Palette));
    free(tmpPal);
}
