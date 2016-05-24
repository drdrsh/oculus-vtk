#ifndef CVIS_DEFINITIONS_H
#define CVIS_DEFINITIONS_H
#include <vector>
#include <string>
#include <map>
#include <GL/glew.h>
#include <gl/Gl.h>
#include <gl/Glu.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

void checkForOpenglErrors(int, const char *);

#define GLERR  checkForOpenglErrors(__LINE__, __FILE__);

#define VTK_NEW(var, type)   vtkSmartPointer<type> var = vtkSmartPointer<type>::New();

struct VTKIndexBuffer {

	vtkSmartPointer<vtkPolyData> polyData;
	std::vector<GLuint> indices;
	GLuint id;


	void load() {
		indices.empty();
		vtkIdType npoints, *pointIds, triangleId = 0;
		for (polyData->GetPolys()->InitTraversal(); polyData->GetPolys()->GetNextCell(npoints, pointIds); triangleId++) {
			indices.push_back(pointIds[0]);
			indices.push_back(pointIds[1]);
			indices.push_back(pointIds[2]);
		}
	}

	void bind() {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	}

	void draw(GLenum mode) {
		glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, NULL);
	}

	void unbind() {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	VTKIndexBuffer(vtkSmartPointer<vtkPolyData> data) {

		/*
		glDrawElements doesn't support strides and vtkCellId Array is a simple 1D list with the following
		structure
		numOfComponents c1 c2 c3 numOfComponents c1 c2 c3
		so I cannot send the raw data to the rendering pipeline
		*/
		polyData = data;
		load();
		glGenBuffers(1, &id);
		GLERR;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
		GLERR;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
		GLERR;

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

	~VTKIndexBuffer() {
		if (id) {
			glDeleteBuffers(1, &id);
			id = 0;
		}
	}

};
struct VTKVertexBuffer {

	vtkSmartPointer<vtkPolyData> polyData;
	std::map<std::string, GLuint> buffers;
	std::map<GLuint, GLuint> VTK2GLTypeMap;

	void reload(std::string id) {

		GLuint bufferId = buffers[id];
		vtkDataArray * arr;
		if (id.compare("Points") == 0) {
			arr = polyData->GetPoints()->GetData();
		} else {
			arr = polyData->GetPointData()->GetArray(id.c_str());
		}

		unsigned long long sz = arr->GetDataTypeSize() * arr->GetSize();

		glBindBuffer(GL_ARRAY_BUFFER, bufferId);
		GLERR
		glBufferSubData(GL_ARRAY_BUFFER, 0, sz, arr->GetVoidPointer(0));
		GLERR
	}

	std::string getVariableName(vtkDataArray * arr) {
		std::string arrName = arr->GetName();
		arrName.insert(0, "vec");
		return arrName;
	}

	void pushArray(GLuint program, GLuint id, vtkDataArray * arr) {

		glBindBuffer(GL_ARRAY_BUFFER, id);
		std::string arrayName = getVariableName(arr);
		GLchar * p = (GLchar * )arrayName.data();
		GLint loc =		(program, p);
		if (loc == -1) {
			std::cerr << "Couldn't find shader variable " << arrayName << std::endl;
			return;
		}

		int numberOfComponents = arr->GetNumberOfComponents();
		GLuint glDatatype = VTK2GLTypeMap[arr->GetDataType()];
		glVertexAttribPointer(
			loc,
			numberOfComponents,
			glDatatype,
			GL_FALSE,
			0,
			0
		);
		GLERR;
	}

	void bind(GLuint program) {

		std::map<std::string, GLuint>::iterator it;
		for (it = buffers.begin(); it != buffers.end(); ++it) {
			std::string arrayName = it->first;
			GLuint id = it->second;
			vtkDataArray * arr;
			if (arrayName.compare("Points") == 0) {
				arr = polyData->GetPoints()->GetData();
			}
			else {
				arr = polyData->GetPointData()->GetArray(arrayName.c_str());
			}
			pushArray(program, id, arr);
		}

	}

	void unbind(GLuint program) {
		std::map<std::string, GLuint>::iterator it;
		for (it = buffers.begin(); it != buffers.end(); ++it) {
			std::string arrayName = it->first;
			GLuint id = it->second;
			vtkDataArray * arr;
			if (arrayName.compare("Points") == 0) {
				arr = polyData->GetPoints()->GetData();
			}
			else {
				arr = polyData->GetPointData()->GetArray(arrayName.c_str());
			}
			GLuint loc = glGetAttribLocation(program, getVariableName(arr).c_str());
			if (loc == -1) {
				continue;
			}
			glDisableVertexAttribArray(loc);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	VTKVertexBuffer(vtkSmartPointer<vtkPolyData> ptr) {

		VTK2GLTypeMap.insert_or_assign(VTK_FLOAT, GL_FLOAT);
		VTK2GLTypeMap.insert_or_assign(VTK_DOUBLE, GL_DOUBLE);
		VTK2GLTypeMap.insert_or_assign(VTK_INT, GL_INT);

		polyData = ptr;
		int arrCount = ptr->GetPointData()->GetNumberOfArrays();

		/*
		GLuint vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		*/

		for (int i = 0; i < arrCount; i++) {			
			GLuint buffer = 0;
			vtkDataArray* arr = ptr->GetPointData()->GetArray(i);
			char * arrName = arr->GetName();
			if (!arrName) {
				continue;
			}

			unsigned long long sz = arr->GetDataTypeSize() * arr->GetSize();
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sz, arr->GetVoidPointer(0), GL_DYNAMIC_DRAW);
			glVertexAttribPointer(
				i, 
				arr->GetNumberOfComponents(), 
				VTK2GLTypeMap[arr->GetDataType()],
				GL_FALSE, 
				0, 
				NULL
			);
			buffers.insert_or_assign(arrName, buffer);
		}

		unsigned long long sz =
			polyData->GetPoints()->GetData()->GetDataTypeSize()
			*
			polyData->GetPoints()->GetData()->GetNumberOfComponents()
			*
			polyData->GetPoints()->GetData()->GetNumberOfTuples();

		GLuint buffer = 0;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, sz, polyData->GetPoints()->GetVoidPointer(0), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(
			arrCount,
			polyData->GetPoints()->GetData()->GetNumberOfComponents(),
			VTK2GLTypeMap[polyData->GetPoints()->GetData()->GetDataType()],
			GL_FALSE,
			0,
			NULL
		);

		buffers.insert_or_assign("Points", buffer);
	}

	~VTKVertexBuffer() {
		std::map<std::string, GLuint>::iterator it;
		for (it = buffers.begin(); it != buffers.end(); ++it) {
			if (it->second) {
				glDeleteBuffers(1, &it->second);
				it->second = 0;
			}
		}
	}
};
 
#endif