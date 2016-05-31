#ifndef VTKMODE_H
#define VTKMODE_H

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/mat4x4.hpp>


#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include "VTKGLBuffer.h"
#include "definitions.h"

#include <thread>

class VTKModel {

public:


	VTKModel(std::string filename, std::string program);

	void render(glm::mat4 o, glm::mat4 proj, glm::mat4 view);

	void release();
	~VTKModel();




private:

	int		 m_numIndices;
	GLuint   m_program = -1;
	
	std::string m_programName;
	
	glm::mat4 m_matObj;
	glm::mat4 m_matProj;
	glm::mat4 m_matView;
	glm::mat4 m_matNorm;

	bool m_isInitialized;
	bool m_isThreadRunning;
	std::thread watcherThread;
	
	void allocateBuffers();
	void reloadProgram();
	void readProgram();
	void init();

	VTKGLBuffer* m_glBuffer = nullptr;
	
	std::string newFragmentProgram;
	std::string newVertexProgram;

	std::string getResourcePath(std::string path);
	GLuint createShader(GLenum type, const GLchar * src);
	GLint  ufLoc(const char * name) {
		GLint  r = glGetUniformLocation(m_program, name);
		return r;
	}

	vtkSmartPointer<vtkPolyData> m_triangles;


	std::thread m_watcherThread;

	void watchFileForNewProgram();

	vtkSmartPointer<vtkPolyData> VTKModel::getSquarePolyData();
	vtkSmartPointer<vtkPolyData> VTKModel::getSpherePolyData();
	vtkSmartPointer<vtkPolyData> VTKModel::getImagePolyData(std::string);


};
#endif