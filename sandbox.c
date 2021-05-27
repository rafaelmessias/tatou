// GLEW must always come before SDL_opengl
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"
#include <math.h>
#include "linmath.h"


SDL_Window *Window;
GLuint Vbo, Vao, ShaderProg, TransfLoc, IsPointLoc;


GLchar* vertexSrc = \
    "#version 150\n\
    in vec3 in_Position;\
    in vec3 in_Color;\
    out vec3 color;\
    uniform mat4 transf;\
    void main(void) {\
        color = in_Color;\
        gl_Position = transf * vec4(in_Position, 1.0);\
    }";

GLchar* fragSrc = \
    "#version 150\n\
    precision highp float;\
    in vec3 color;\
    out vec4 fragColor;\
    uniform bool isPoint;\
    void main(void) {\
        vec2 coord = gl_PointCoord - vec2(0.5);\
        if (isPoint && length(coord) > 0.5)\
            discard;\
        fragColor = vec4(color, 1.0);\
    }";


void initSystem()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    
    Window = SDL_CreateWindow(
        "Sandbox", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        800,
        800, 
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
        int stride = 6 * sizeof(GL_FLOAT);
        const void *colorOffset = (const void *)(3 * sizeof(GL_FLOAT));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0); 
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, colorOffset); 
        glEnableVertexAttribArray(1);
    glBindVertexArray(0);   

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

    ShaderProg = glCreateProgram();
    glAttachShader(ShaderProg, vertexShader);
    glAttachShader(ShaderProg, fragShader);
    // This connects VAO position 0 (set above) to the shader variable in_Position
    glBindAttribLocation(ShaderProg, 0, "in_Position");
    glBindAttribLocation(ShaderProg, 1, "in_Color");
    glLinkProgram(ShaderProg);
    glUseProgram(ShaderProg);

    TransfLoc = glGetUniformLocation(ShaderProg, "transf");
    IsPointLoc = glGetUniformLocation(ShaderProg, "isPoint");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // This is needed in order to handle gl_PointCoord in the fragment shader
    // NOTE This is always on in OpenGL 3.2+
    // glEnable(GL_POINT_SPRITE);

    // This is needed for alpha
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);
    
    // This is basically supersampling (all fragments are multisampled). Useful if you are discarding fragments.
    glEnable(GL_SAMPLE_SHADING);
    glMinSampleShading(1.0);

    // NOTE This never seem to work for me.
    // glEnable(GL_POINT_SMOOTH);
}

int main(int argc, char *argv[])
{
    initSystem();    

    float vertices[] =
    {
        -0.5, -0.5, 0.0,    1, 0, 0,
         0.5, -0.5, 0.0,    0, 1, 0,
         0.5,  0.5, 0.0,    0, 0, 1,
        -0.5,  0.5, 0.0,    0, 1, 1, 
         0.0,  0.0, 0.0,    1, 1, 0
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    mat4x4 M;
    mat4x4_identity(M);

    int quit = 0;
    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(TransfLoc, 1, GL_FALSE, (const GLfloat *)M);

        glBindVertexArray(Vao);
            glUniform1i(IsPointLoc, GL_FALSE);    
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glPointSize(200);
            glUniform1i(IsPointLoc, GL_TRUE);
            glDrawArrays(GL_POINTS, 4, 1);
            // glDrawArrays(GL_POINTS, 0, 1);
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
                mat4x4 M2;
                mat4x4_identity(M2);
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;

                    case SDLK_a:
                        // Multiply on the left, so that new operations are the *last* to be applied
                        mat4x4_rotate_Z(M2, M2, M_PI / 18);
                        mat4x4_mul(M, M2, M);
                        break;

                    case SDLK_LEFT:
                        mat4x4_rotate_Y(M2, M2, M_PI / 18);
                        mat4x4_mul(M, M2, M);
                        break;

                    case SDLK_UP:
                        mat4x4_rotate_X(M2, M2, M_PI / 18);
                        mat4x4_mul(M, M2, M);
                        break;
                }
            }
        }        
    }
    while(!quit);
}
