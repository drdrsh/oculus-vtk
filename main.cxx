#include <GL/glew.h>
#include <glm/vec3.hpp>

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkMarchingCubes.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkVoxelModeller.h>
#include <vtkSphereSource.h>
#include <vtkImageData.h>
#include <vtkNIFTIImageReader.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkSmoothPolyDataFilter.h>

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkThreshold.h>
#include <vtkDataSetAttributes.h>
#include <vtkImageWrapPad.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkLookupTable.h>
#include <vtkDecimatePro.h>
#include <vtkFillHolesFilter.h>

#include "OVRHelper.h"
#include "definitions.h"

//#define OUTPUT_FN "F:\\MAbdelraouf\\Projects\\CXX\\oculus - vtk\\src\\data\\H - 554_20041005_159571722609202.Lung.mask.img.gz"
#define OUTPUT_FN "C:\\data\\left_mask.nii.gz"


#include <stdio.h>
#include <stdlib.h>


#include <GLFW/glfw3.h>
#include "VTKModel.h"
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/mat4x4.hpp>

VTKModel *mainModel;

void checkForOpenglErrors(int line, const char * fn) {

	GLenum err(glGetError());

	while (err != GL_NO_ERROR) {
		std::string error;

		switch (err) {
		case GL_INVALID_OPERATION:				error = "INVALID_OPERATION";			 break;
		case GL_INVALID_ENUM:				    error = "INVALID_ENUM";					 break;
		case GL_INVALID_VALUE:					error = "INVALID_VALUE";				 break;
		case GL_OUT_OF_MEMORY:					error = "OUT_OF_MEMORY";				 break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}

		std::cerr << fn << ":" << line << "  GL_" << error.c_str() << std::endl;
		err = glGetError();
	}
}

static void error_callback(int error, const char *description) {
//	fputs(description, stderr);
	
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

	mainModel = new VTKModel(std::string(OUTPUT_FN), std::string("simple"));

	glm::vec3 center = mainModel->getCenter();
	double depth = mainModel->getExtentAlongAxis(2);

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
	GLERR;
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

int mxin(int argc, char * argv[]) {


	return 0;
	try {

		double isoValue;
		VTK_NEW(volume, vtkImageData);
		VTK_NEW(reader, vtkNIFTIImageReader);
		VTK_NEW(surface, vtkDiscreteMarchingCubes);
		VTK_NEW(smoother, vtkSmoothPolyDataFilter);
		VTK_NEW(decimate, vtkDecimatePro);
		

		reader->SetFileName(OUTPUT_FN);
		reader->Update();
		volume->DeepCopy(reader->GetOutput());

		surface->SetInputData(volume);
		surface->ComputeNormalsOn();
		surface->Update();

		


	} catch (std::exception e) {
		std::cerr << "C++ Runtime Exception caught:" << std::endl;
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
