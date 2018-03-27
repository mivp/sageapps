include ../../config.mk

SDK_PATH=include

COMPILER_FLAGS= -g -Wno-multichar -I$(SDK_PATH) -fno-rtti $(SAGE_CFLAGS) -O3 -I../../include $(QUANTA_CFLAGS) $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) $(GLUT_CFLAGS)

ifeq ($(MACHINE), Darwin)
   COMPILER_FLAGS  += -FGLUT -FOpenGL
   LIBS=-lpthread -L../../lib -lsail -framework GLUT -framework OpenGL -lobjc -lm 
else
   LIBS=-lpthread ${SUN_LIBS} -lm -ldl $(QUANTA_LDFLAGS) $(GLEW_LIB) -L../../lib -lquanta -lsail -lm -ldl -lpthread
endif

PROJ_EXE=decklinkcapture

default: $(PROJ_EXE)

install: default
	cp $(PROJ_EXE) ../../bin


decklinkcapture: decklinkcapture.o $(SDK_PATH)/DeckLinkAPIDispatch.o
	$(COMPILER) $(SAGE_LDFLAGS) -o decklinkcapture decklinkcapture.o $(SDK_PATH)/DeckLinkAPIDispatch.o $(LIBS)

decklinkcapture.o: decklinkcapture.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_DIR) -c decklinkcapture.cpp

clean:
	\rm -f *~ *.o $(PROJ_EXE)
