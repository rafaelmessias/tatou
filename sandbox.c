// GLEW must always come before SDL_opengl
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"


SDL_Window *Window;
GLuint Vbo, Vao;


GLchar* vertexSrc = \
    "#version 150\n\
    in vec3 in_Position;\
    void main(void) {\
        gl_Position = vec4(in_Position, 1.0);\
    }";

GLchar* fragSrc = \
    "#version 150\n\
    precision highp float;\
    out vec4 fragColor;\
    void main(void) {\
        fragColor = vec4(0.0, 1.0, 0.0, 1.0);\
    }";


void initSystem()
{
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    
    SDL_Init(SDL_INIT_VIDEO);
    Window = SDL_CreateWindow(
        "Sandbox", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        800,
        600, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext *context = SDL_GL_CreateContext(Window);
    SDL_GL_MakeCurrent(Window, context);
    // Without this, on OSX, we get a segfault...!
    glewExperimental = GL_TRUE; 
    glewInit();

    SDL_GL_SetSwapInterval(1);
    glClearColor(0, 0, 0, 1);

    // Setup VAO here
    glGenBuffers(1, &Vbo);
    glBindBuffer(GL_ARRAY_BUFFER, Vbo);    
	glGenVertexArrays(1, &Vao);
        glBindVertexArray(Vao);    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); 
        glEnableVertexAttribArray(0);
    glBindVertexArray(0);   

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)&vertexSrc, 0);
    glCompileShader(vertexShader);

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, (const GLchar**)&fragSrc, 0);
    glCompileShader(fragShader);

    GLuint shaderProg = glCreateProgram();
    glAttachShader(shaderProg, vertexShader);
    glAttachShader(shaderProg, fragShader);
    // This connects VAO position 0 (set above) to the shader variable in_Position
    glBindAttribLocation(shaderProg, 0, "in_Position");
    glLinkProgram(shaderProg);
    glUseProgram(shaderProg);
}

int main(int argc, char *argv[])
{
    initSystem();    

    float vertices[] =
    {
        -0.5, -0.5, 0.0,
         0.5, -0.5, 0.0,
         0.0,  0.5, 0.0
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    int quit = 0;
    do
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(Vao);    
            glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        SDL_GL_SwapWindow(Window);

        // Without event polling, in OSX the window does not open...!
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
                }
            }
        }        
    }
    while(!quit);
}
