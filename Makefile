#OBJS specifies which files to compile as part of the project
OBJS = main.c pak.c unpack.c

#CC specifies which compiler we're using
CC = gcc

#INCLUDE_PATHS specifies the additional include paths we'll need
#INCLUDE_PATHS = -I../linmath.h

#LIBRARY_PATHS specifies the additional library paths we'll need
#LIBRARY_PATHS = -LC:\mingw_dev_lib\lib

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
#COMPILER_FLAGS = -w -Wl,-subsystem,windows
COMPILER_FLAGS = $(shell sdl2-config --cflags)

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = $(shell sdl2-config --libs) -lz

ifeq ($(OS),Windows_NT)
    LINKER_FLAGS += -lglew32 -lopengl32
else ifeq ($(shell uname),Darwin)
    LINKER_FLAGS += -lglew -framework OpenGL
endif

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = tatou

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

clean:
	rm $(OBJ_NAME)
