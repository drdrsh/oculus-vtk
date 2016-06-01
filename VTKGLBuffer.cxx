#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/GLU.h>

#include "Kernel/OVR_System.h"
#include "OVR_CAPI_0_8_0.h"
#include "OVR_CAPI_GL.h"

#include <vtkDataArray.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include "VTKGLBuffer.h"

void VTKGLBuffer::reload(std::string id) {

	GLuint bufferId = buffers[id];
	vtkDataArray * arr;
	if (id.compare("Points") == 0) {
		arr = polyData->GetPoints()->GetData();
	}
	else {
		arr = polyData->GetPointData()->GetArray(id.c_str());
	}

	unsigned long long sz = arr->GetDataTypeSize() * arr->GetSize();

	glBindBuffer(GL_ARRAY_BUFFER, bufferId);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sz, arr->GetVoidPointer(0));
}

std::string VTKGLBuffer::getVariableName(vtkDataArray * arr) {
	std::string arrName = arr->GetName();
	arrName.insert(0, "vec");
	return arrName;
}



void VTKGLBuffer::loadIndexBuffer() {

	std::vector<GLuint> indices;
	vtkIdType npoints, *pointIds, triangleId = 0;
	for (polyData->GetPolys()->InitTraversal(); polyData->GetPolys()->GetNextCell(npoints, pointIds); triangleId++) {
		indices.push_back(pointIds[0]);
		indices.push_back(pointIds[1]);
		indices.push_back(pointIds[2]);
	}
	indexCount = indices.size();

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/*
	void * p = (void *)indices.data();
	unsigned int * pf = reinterpret_cast<unsigned int *>(p);
	std::cout << "Indices" << std::endl;
	for (int i = 0; i < indices.size(); i++) {
	unsigned int x = pf[i];
	std::cout << x << std::endl;
	} 
	*/

}


void VTKGLBuffer::render() {
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void VTKGLBuffer::loadVertexBuffers() {

	int arrCount = polyData->GetPointData()->GetNumberOfArrays();
	for (int i = 0; i < arrCount + 1; i++) {
		GLuint buffer = 0;
		vtkDataArray* arr;
		char * arrName;
		if (i == 0) {
			//First build the points array (vertex data)
			arr = polyData->GetPoints()->GetData();
			arrName = "Points";
		}
		else {
			arr = polyData->GetPointData()->GetArray(i - 1);
			arrName = arr->GetName();
		}

		if (!arrName || strlen(arrName) == 0) {
			continue;
		}

		unsigned long long sz =
			arr->GetDataTypeSize()
			*
			arr->GetNumberOfComponents()
			*
			arr->GetNumberOfTuples();

		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, sz, arr->GetVoidPointer(0), GL_STATIC_DRAW);

		std::string arrayName = getVariableName(arr);
		GLchar * p = (GLchar *)arrayName.data();
		GLint loc = glGetAttribLocation(program, p);
		glEnableVertexAttribArray(loc);
		if (loc == -1) {
			std::ostringstream error;
			error << "Couldn't find shader variable " << arrayName << ", Could be missing or optimized out" << std::endl;
			throw std::exception(error.str().c_str());
		}

		glVertexAttribPointer(
			loc,
			arr->GetNumberOfComponents(),
			VTK2GLTypeMap[arr->GetDataType()],
			GL_FALSE,
			0,
			NULL
		);
		buffers.insert_or_assign(arrName, buffer);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		/*
		void * pnt = (void *)arr->GetVoidPointer(0);
		float * pf = reinterpret_cast<float *>(pnt);
		std::cout << "Array " << arrName << std::endl;
		for (int i = 0; i < sz / arr->GetDataTypeSize(); i++) {
		float x = pf[i];
		std::cout << x << std::endl;
		}
		*/

	}
}

VTKGLBuffer::VTKGLBuffer(vtkSmartPointer<vtkPolyData> ptr, GLuint p) {

	program = p;
	polyData = ptr;

	VTK2GLTypeMap.insert_or_assign(VTK_FLOAT, GL_FLOAT);
	VTK2GLTypeMap.insert_or_assign(VTK_DOUBLE, GL_DOUBLE);
	VTK2GLTypeMap.insert_or_assign(VTK_INT, GL_INT);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	loadIndexBuffer();
	loadVertexBuffers();

	glBindVertexArray(0);
}

void VTKGLBuffer::release() {
	std::map<std::string, GLuint>::iterator it;
	for (it = buffers.begin(); it != buffers.end(); ++it) {
		if (it->second) {
			glDeleteBuffers(1, &it->second);
			it->second = 0;
		}
	}
	glDeleteBuffers(1, &indexBuffer);
	glDeleteVertexArrays(1, &vao);
}

VTKGLBuffer::~VTKGLBuffer() {
	release();
}
