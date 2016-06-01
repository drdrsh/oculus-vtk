
#ifdef _WIN32
#include <windows.h>
#else
//TODO: Added needed files to implement a file edit hook
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "definitions.h"
#include "VTKModel.h"
#include "OVRHelper.h"

#define OUTPUT_FN "C:\\data\\left_mask.nii.gz"

VTKModel *mainModel;

static void error_callback(int error, const char *description) {
	cerr << description;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void renderOVRScene(glm::mat4 projection, glm::mat4 view) {
	glm::mat4 model = glm::scale(glm::vec3(1.0f));
	mainModel->render(model, projection, view);
}

static void render(GLFWwindow *window) {
	
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	OVRHelper::getInstance()->render(renderOVRScene);
}


void init(GLFWwindow *window) {

	OVRHelper::getInstance()->init();
	glm::ivec2 windowSize = OVRHelper::getInstance()->getViewportSize();
	glfwSetWindowSize(window, windowSize.x, windowSize.y);
	
	/*
	This load a 3D image and builds a 3D model from voxels of the value "1" 
	and uses shader programs starting with prefix "simple"
	*/
	mainModel = new VTKModel(std::string(OUTPUT_FN), std::string("simple"));

	glm::vec3 center = mainModel->getCenter();
	double depth = mainModel->getExtentAlongAxis(2);

	//Move the camera to be outside the object
	glm::vec3 camera = center;
	camera.z -= depth - 50.0f;

	OVRHelper::getInstance()->setLookAt(camera, center);

}

int main(void) {
	GLFWwindow *window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(100, 100, "OpenGL Boilerplate", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}


	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	glewExperimental = GL_TRUE;
	glewInit();

	init(window);

	while (!glfwWindowShouldClose(window)) {
		render(window);

		glfwSwapBuffers(window);
		glfwPollEvents();
		//glfwWaitEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
	exit(EXIT_SUCCESS);
}