#ifndef VTKGLBUFFER_H
#define VTKGLBUFFER_H

#include <GL/glew.h>
#include <GL/GL.h>

#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <vector>
#include <string>
#include <sstream>
#include <map>


class VTKGLBuffer {

public:

	VTKGLBuffer(vtkSmartPointer<vtkPolyData> ptr, GLuint progam);
	~VTKGLBuffer();

	void reload(std::string id);
	void render();
	void release();

private:

	std::string getVariableName(vtkDataArray * arr);


	void loadIndexBuffer();
	void loadVertexBuffers();

	GLuint vao = -1;
	GLuint program = -1;
	GLuint indexBuffer = -1;
	GLuint indexCount = -1;

	vtkSmartPointer<vtkPolyData> polyData;
	std::map<std::string, GLuint> buffers;
	std::map<GLuint, GLuint> VTK2GLTypeMap;


};
#endif