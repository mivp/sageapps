#include "GLInclude.h"
#include "GLUtils.h"
#include "Camera.h"
#include "Renderer.h"
#include "VideoDecode.h"

#include "libsage.h"

#include <sstream>
#include <iostream>
#include <vector>
using std::stringstream;
using std::string;
using std::cout;
using std::endl;

using namespace viewer360;

GLFWwindow* window;
string title;

//timing related variables
float last_time=0, current_time =0;
//delta time
float dt = 0;

#define WIDTH 1920//1024
#define HEIGHT 768

Camera* camera = NULL;
Renderer* renderer = NULL;
VideoDecode* decoder = NULL;
sail *sageInf;

//camera / mouse
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstmouse = true;
bool usemouse = false;

void doMovement();

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //cout << key << endl;
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key >= 0 && key < 1024)
    {
        if(action == GLFW_PRESS)
            keys[key] = true;
        else if(action == GLFW_RELEASE)
            keys[key] = false;
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    //cout << "button: " << button << " action: " << action << endl;
    if (button == 0 && action == 1)
        usemouse = true;

    else if (button == 0 && action == 0)
        usemouse = false;
}


static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    //camera->processMouseScroll(yoffset);
}

static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {

    if(firstmouse) {
        camera->move_camera = false;
        camera->Move2D((int)xpos, (int)ypos);
        firstmouse = false;
        return;
    }

    camera->move_camera = usemouse;
    camera->Move2D((int)xpos, (int)ypos);
}

static void window_size_callback(GLFWwindow* window, int width, int height) {

}

void frameCallback(const AVFrame* frame, void* user) {
    cout << "get a frame width " << frame->width << " height: " << frame->height << endl;
    if(renderer)
        renderer->update(frame->data[0], frame->linesize[0],
                        frame->data[1], frame->linesize[1],
                        frame->data[2], frame->linesize[2]);
}

void init_resources() {

    camera = new Camera();
    camera->SetPosition(glm::vec3(0, 0, 0));
    camera->SetLookAt(glm::vec3(0, -2, 0));
    camera->SetViewport(0, 0, WIDTH, HEIGHT);
    camera->SetClipping(0.1, 1000);
    camera->camera_scale = 0.01;
    camera->camera_up = glm::vec3(0, 0, 1);
    camera->Update();

    decoder = new VideoDecode(frameCallback, NULL);
    decoder->load("testdata/ayutthaya_4k.mp4");

    renderer = new Renderer(true, decoder->getWidth(), decoder->getHeight());

    sageInf = createSAIL("viewer360", WIDTH, HEIGHT, PIXFMT_888, NULL);
}

void free_resources()
{
    if(camera)
        delete camera;
    if(renderer)
        delete renderer;
    deleteSAIL(sageInf);
}

void doMovement() {
    // Camera controls
    if(keys[GLFW_KEY_W])
        camera->Move(CAM_FORWARD);
    if(keys[GLFW_KEY_S])
        camera->Move(CAM_BACK);
    if(keys[GLFW_KEY_A])
        camera->Move(CAM_LEFT);
    if(keys[GLFW_KEY_D])
        camera->Move(CAM_RIGHT);
    if(keys[GLFW_KEY_Q])
        camera->Move(CAM_DOWN);
    if(keys[GLFW_KEY_E])
        camera->Move(CAM_UP);

    if(keys[GLFW_KEY_I]) {
        cout << "pos: " << camera->camera_position[0] << ", " << camera->camera_position[1] << ", " << camera->camera_position[2] << endl;
        cout << "direction: " << camera->camera_direction[0] << ", " << camera->camera_direction[1] << ", " << camera->camera_direction[2] << endl;
        cout << "lookat: " << camera->camera_look_at[0] << ", " << camera->camera_look_at[1] << ", " << camera->camera_look_at[2] << endl;
        cout << "up: " << camera->camera_up[0] << ", " << camera->camera_up[1] << ", " << camera->camera_up[2] << endl;
        keys[GLFW_KEY_I] = false;
    }
    if(keys[GLFW_KEY_T]) {
        keys[GLFW_KEY_T] = false;
    }
    if(keys[GLFW_KEY_N]) {
        keys[GLFW_KEY_N] = false;
    }
    if(keys[GLFW_KEY_M]) {
        keys[GLFW_KEY_M] = false;
    }
    if(keys[GLFW_KEY_P]) {
        keys[GLFW_KEY_P] = false;
    }
    if(keys[GLFW_KEY_O]) {
        keys[GLFW_KEY_O] = false;
    }
    if(keys[GLFW_KEY_R]) {
        if(decoder)
            decoder->rewind();
        keys[GLFW_KEY_R] = false;
    }
    camera->Update();
}

void mainLoop()
{
    const int samples = 50;
    float time[samples];
    int index = 0;

    do{
        //timing related calcualtion
        last_time = current_time;
        current_time = glfwGetTime();
        dt = current_time-last_time;

        glfwPollEvents();
        doMovement();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render terrain
        float *MV, *P;
        MV = (float*)glm::value_ptr(camera->MV);
        P = (float*)glm::value_ptr(camera->projection);
        // render

        bool hasFrame = false;
        decoder->readFrame(hasFrame);
        renderer->render(MV, P);

        GLubyte *rgbBuffer = nextBuffer(sageInf);
        glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer);
        swapBuffer(sageInf);

        glfwSwapBuffers(window);

        // Update FPS
        time[index] = glfwGetTime();
        index = (index + 1) % samples;

        if( index == 0 ) {
          float sum = 0.0f;
          for( int i = 0; i < samples-1 ; i++ )
            sum += time[i + 1] - time[i];
          float fps = samples / sum;

          stringstream strm;
          strm << title;
          strm.precision(4);
          strm << " (fps: " << fps << ")";
          glfwSetWindowTitle(window, strm.str().c_str());
        }

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );
}

int main(int argc, char* argv[]) {

	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

    glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( WIDTH, HEIGHT, "OpenGL window with GLFW", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_position_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// print GL info
	GLUtils::dumpGLInfo();

	// init resources
	init_resources();

	// Enter the main loop
	mainLoop();

	free_resources();

	// Close window and terminate GLFW
	glfwTerminate();

	// Exit program
	return EXIT_SUCCESS;
}
