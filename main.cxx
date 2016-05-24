#include "definitions.h"
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

#include "definitions.h"

#define OUTPUT_FN   "C:\\Users\\Mostafa\\Desktop\\Spring_2016\\MIATT\\HW_Build\\Project\\Slices\\left-volume.nii"

#include <stdio.h>
#include <stdlib.h>


#include <GLFW/glfw3.h>
#include <VTKModel.h>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/projection.hpp>

VTKModel *mainModel;

#define GLSL(src) #src

void checkForOpenglErrors(int line, const char * fn) {

	GLenum err(glGetError());

	while (err != GL_NO_ERROR) {
		std::string error;

		switch (err) {
		case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
		case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
		case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
		case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
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

static void render(GLFWwindow *window) {
	
	glm::mat4 matProjection = glm::mat4(1.0);  //loadIdentity
	matProjection *= glm::perspective(90.0f, 640.0f/480.0f, -500.0f, 500.0f);

	glm::mat4 matView = glm::mat4(1.0);  //loadIdentity
	matView *= glm::lookAt(
		glm::vec3(0.0f, 0.0f,  0.0f), //Eye
		glm::vec3(0.0f, 0.0f, -1.0f),//Point
		glm::vec3(0.0f, 1.0f,  0.0f)  //Up
	);

	glm::mat4 matObj = glm::mat4(1.0);

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLERR;
	//glLoadIdentity();
	//GLERR;

	mainModel->render(matObj, matProjection, matView);

	GLERR;
}


void init() {
	mainModel = new VTKModel(std::string("C:\\Users\\Mostafa\\Desktop\\Spring_2016\\MIATT\\Data\\project\\right_mask.nii.gz"));
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
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window = glfwCreateWindow(640, 480, "OpenGL Boilerplate", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}


	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	glewExperimental = GL_TRUE;
	glewInit();

	init();
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
