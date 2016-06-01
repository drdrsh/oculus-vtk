
#ifdef _WIN32
#include <windows.h>
#else
//TODO: Added needed files to implement a file edit hook
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vtkVoxelModeller.h>
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkSphereSource.h>
#include <vtkNIFTIImageReader.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPolygon.h>
#include <vtkPoints.h>

#include <thread>
#include <fstream>
#include <algorithm>
#include <vector>
#include <regex>
#include <sstream>
#include <streambuf>


#include "definitions.h"
#include "VTKModel.h"


vtkSmartPointer<vtkPolyData> VTKModel::getSpherePolyData() {

	VTK_NEW(voxelModeller, vtkVoxelModeller);
	VTK_NEW(volume, vtkImageData);
	VTK_NEW(surface, vtkMarchingCubes);
	VTK_NEW(sphereSource, vtkSphereSource);

	sphereSource->SetPhiResolution(20);
	sphereSource->SetThetaResolution(20);
	sphereSource->Update();

	double bounds[6];
	sphereSource->GetOutput()->GetBounds(bounds);
	for (unsigned int i = 0; i < 6; i += 2)
	{
		double range = bounds[i + 1] - bounds[i];
		bounds[i] = bounds[i] - .1 * range;
		bounds[i + 1] = bounds[i + 1] + .1 * range;
	}

	voxelModeller->SetSampleDimensions(50, 50, 50);
	voxelModeller->SetModelBounds(bounds);
	voxelModeller->SetScalarTypeToFloat();
	voxelModeller->SetMaximumDistance(.1);

	voxelModeller->SetInputConnection(sphereSource->GetOutputPort());
	voxelModeller->Update();

	volume->DeepCopy(voxelModeller->GetOutput());

	surface->SetInputData(volume);

	surface->ComputeNormalsOn();
	surface->SetValue(0, .5);

	surface->Update();

	return surface->GetOutput();
}

vtkSmartPointer<vtkPolyData> VTKModel::getImagePolyData(std::string filename) {

	VTK_NEW(volume, vtkImageData);
	VTK_NEW(reader, vtkNIFTIImageReader);
	VTK_NEW(surface, vtkMarchingCubes);
	
	reader->SetFileName(filename.c_str());
	reader->Update();
	volume->DeepCopy(reader->GetOutput());
	surface->SetInputData(volume);

	surface->ComputeNormalsOn();
	surface->SetValue(0, 1);

	surface->Update();

	return surface->GetOutput();
}

vtkSmartPointer<vtkPolyData> VTKModel::getSquarePolyData() {

	VTK_NEW(points, vtkPoints);
	VTK_NEW(polygon, vtkPolygon);
	VTK_NEW(polygons, vtkCellArray);
	VTK_NEW(polygonPolyData, vtkPolyData);
	
	// Setup three points
	points->InsertNextPoint( 0.0,  0.5, 0.0);
	points->InsertNextPoint( 0.5, -0.5, 0.0);
	points->InsertNextPoint(-0.5, -0.5, 0.0);

	// Create the triangle
	polygon->GetPointIds()->SetNumberOfIds(3); //make a triangle
	polygon->GetPointIds()->SetId(0, 0);
	polygon->GetPointIds()->SetId(1, 1);
	polygon->GetPointIds()->SetId(2, 2);

	// Add the triangle to a list of polygons
	polygons->InsertNextCell(polygon);

	// Create a PolyData
	polygonPolyData->SetPoints(points);
	polygonPolyData->SetPolys(polygons);

	/*
	vtkSmartPointer<vtkFloatArray> colors = vtkFloatArray::New();
	colors->SetName("Colors");
	colors->SetNumberOfComponents(4);
	colors->SetNumberOfTuples(3);
	colors->SetTuple4(0, 1.0, 0.0, 0.0, 1.0);
	colors->SetTuple4(1, 0.0, 1.0, 0.0, 1.0);
	colors->SetTuple4(2, 0.0, 0.0, 1.0, 1.0);
	polygonPolyData->GetPointData()->AddArray(colors);
	*/

	return polygonPolyData;
}

VTKModel::VTKModel(std::string filename, std::string program) {

	VTK_NEW(volume, vtkImageData);
	VTK_NEW(reader, vtkNIFTIImageReader);
	VTK_NEW(surface, vtkMarchingCubes);

	m_programName = program;
	//m_triangles = getSquarePolyData();
	m_triangles = getImagePolyData(filename);
	//m_triangles = getSpherePolyData();
	m_isInitialized = false;
	m_isThreadRunning = false;
}


void VTKModel::allocateBuffers() {
	if (m_glBuffer) {
		delete m_glBuffer;
		m_glBuffer = nullptr;
	}
	m_glBuffer = new VTKGLBuffer(m_triangles, m_program);
}

void VTKModel::init() {

	m_isInitialized = true;
	if (!m_isThreadRunning) {
		m_watcherThread = std::thread(&VTKModel::watchFileForNewProgram, this);
		m_isThreadRunning = true;
	}

	readProgram();
	
	reloadProgram();
	
	allocateBuffers();

}
void VTKModel::watchFileForNewProgram() {

#ifdef _WIN32
	//This will detect changes to shaders and reload them without restarting the application
	while (true) {
		DWORD dwWaitStatus;
		HANDLE notifyHandles[2];

		std::string pathToWatch = VTKModel::getResourcePath("shaders\\");

		notifyHandles[0] = FindFirstChangeNotification(pathToWatch.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

		dwWaitStatus = WaitForMultipleObjects(1, notifyHandles, FALSE, INFINITE);

		if (dwWaitStatus == WAIT_OBJECT_0) {
			Sleep(100);
			readProgram();
		}

	}
#endif


}

double VTKModel::getExtentAlongAxis(int axis) {
	double extent = 0.0;
	if (axis < 0 || axis > 2) {
		return extent;
	}
	double temp[6];
	m_triangles->GetBounds(temp);
	
	int idx = axis * 2;
	return temp[idx + 1] - temp[idx];
}

glm::vec3 VTKModel::getCenter() {

	glm::vec3 center = glm::vec3(0.0f);
	if (!m_triangles) {
		return center;
	}
	double temp[3];
	m_triangles->GetCenter(temp);
	center.x = temp[0];
	center.y = temp[1];
	center.z = temp[2];

	return center;
}

void VTKModel::render(glm::mat4 model, glm::mat4 proj, glm::mat4 view) {

	if (!m_isInitialized) {
		init();
	}

	if (newFragmentProgram.length() != 0 && newVertexProgram.length() != 0) {
		reloadProgram();
	}


	glm::mat4 norm = model;
	glm::mat4 mvp = proj * view * model;

	m_matModel= model;
	m_matProj = proj;
	m_matView = view;
	m_matNorm = norm;

	glUseProgram(m_program);

	glUniformMatrix4fv(ufLoc("matMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
	glUniformMatrix4fv(ufLoc("matProjection"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(ufLoc("matView"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(ufLoc("matModel"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(ufLoc("matNormal"), 1, GL_FALSE, glm::value_ptr(norm));

	/*
	glUniform4fv(glGetUniformLocation(program, "FrontMaterial.ambient"), 1, m_material.front.fAmbient);
	glUniform4fv(glGetUniformLocation(program, "FrontMaterial.emission"), 1, m_material.front.fEmission);
	glUniform4fv(glGetUniformLocation(program, "FrontMaterial.diffuse"), 1, m_material.front.fDiffuse);
	glUniform4fv(glGetUniformLocation(program, "FrontMaterial.specular"), 1, m_material.front.fSpecular);
	glUniform1f(glGetUniformLocation(program, "FrontMaterial.shininess"), m_material.front.fShininess);

	glUniform4fv(glGetUniformLocation(program, "BackMaterial.ambient"), 1, m_material.back.fAmbient);
	glUniform4fv(glGetUniformLocation(program, "BackMaterial.emission"), 1, m_material.back.fEmission);
	glUniform4fv(glGetUniformLocation(program, "BackMaterial.diffuse"), 1, m_material.back.fDiffuse);
	glUniform4fv(glGetUniformLocation(program, "BackMaterial.specular"), 1, m_material.back.fSpecular);
	glUniform1f(glGetUniformLocation(program, "BackMaterial.shininess"), m_material.back.fShininess);
	*/

	m_glBuffer->render();

	glUseProgram(0);

}



std::string VTKModel::getResourcePath(std::string p) {

#ifdef _DEBUG
	std::string vrPath = "F:\\MAbdelraouf\\Projects\\CXX\\oculus-vtk\\src\\";
#else
	if (vrPath.length() == 0) {
		TCHAR buff[MAX_PATH];
		memset(buff, 0, MAX_PATH);
		::GetModuleFileName(NULL, buff, sizeof(buff));
		CString VRDirectory = buff;
		VRDirectory = VRDirectory.Left(VRDirectory.ReverseFind(_T('\\')) + 1);
		VRDirectory.Append(_T("VR\\"));
		vrPath = VRDirectory.GetString();
	}
#endif
	std::string newPath = vrPath;
	newPath = newPath.append(p);
	return newPath;
}

VTKModel::~VTKModel() {
	release();
}

GLuint VTKModel::createShader(GLenum type, const GLchar * src)
{


	std::string output_text;

	//Correction for the line number in the error message
	int commonFileLines = 0;
	std::string line;

	std::ostringstream common_fn;
	common_fn << "shaders\\" << m_programName << "_common.glsl";
	std::ifstream myfile(VTKModel::getResourcePath(common_fn.str()));
	while (std::getline(myfile, line)) {
		commonFileLines++;
	}
	auto callback = [&](std::string const& m) {
		std::istringstream iss(m);
		int n;
		if (iss >> n) {
			if (n == 0) {
				output_text += m;
			}
			else {
				output_text += " (" + std::to_string(n - commonFileLines) + ")";
			}
		}
		else {
			output_text += m;
		}
	};



	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint r;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
	if (!r) {
		GLchar msg[1024];
		glGetShaderInfoLog(shader, sizeof(msg), 0, msg);
		std::string t;
		if (type == GL_VERTEX_SHADER) {
			t.append("VertexShader");
		}
		else {
			t.append("FragmentShader");
		}
		if (msg[0]) {
			std::ofstream log_file;
			std::ostringstream log_fn;
			log_fn << "log\\" << m_programName << "_error.txt";
			log_file.open(VTKModel::getResourcePath(log_fn.str()), std::ofstream::out | std::ofstream::app);
			std::string m = msg;
			std::regex re("\\((\\d+)\\) :");
			std::sregex_token_iterator
				begin(m.begin(), m.end(), re, { -1, 1 }),
				end;
			std::for_each(begin, end, callback);

			log_file << "Compiling " << t << " failed: " << output_text;
			log_file.close();
		}
		return 0;
	}

	return shader;
}


void VTKModel::reloadProgram() {

	std::ofstream log_file;

	std::ostringstream log_fn;
	log_fn << "log\\" << m_programName << "_error.txt";
	log_file.open(VTKModel::getResourcePath(log_fn.str()), std::ofstream::trunc);
	log_file.close();

	GLuint vertexShader = createShader(GL_VERTEX_SHADER, newVertexProgram.data());
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, newFragmentProgram.data());

	if (vertexShader == 0 || fragmentShader == 0) {
		newVertexProgram.clear();
		newFragmentProgram.clear();
		if (vertexShader != 0) {
			glDeleteShader(vertexShader);
		}
		if (fragmentShader != 0) {
			glDeleteShader(fragmentShader);
		}
		return;
	}

	GLuint newProgram = glCreateProgram();
	
	glAttachShader(newProgram, vertexShader);
	glAttachShader(newProgram, fragmentShader);
	
	glLinkProgram(newProgram);
	
	glDetachShader(newProgram, vertexShader);
	glDetachShader(newProgram, fragmentShader);
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	GLint r;
	glGetProgramiv(newProgram, GL_LINK_STATUS, &r);

	if (!r) {
		GLchar msg[1024];
		std::ostringstream log_fn;

		glGetProgramInfoLog(newProgram, sizeof(msg), 0, msg);

		log_fn << "log\\" << m_programName << "_error.txt";
		log_file.open(VTKModel::getResourcePath(log_fn.str()), std::ofstream::out | std::ofstream::app);
		log_file << "Linking program failed: " << msg;
		log_file << newVertexProgram;
		log_file << newFragmentProgram;
		log_file.close();
	}
	else {
		if (m_program != -1) {
			glDeleteProgram(m_program);
		}
		m_program = newProgram;
	}
	newVertexProgram.clear();
	newFragmentProgram.clear();

}


void VTKModel::readProgram() {

	std::ostringstream common_fn;
	common_fn << "shaders\\" << m_programName << "_common.glsl";
	std::ifstream common_file(VTKModel::getResourcePath(common_fn.str()));
	std::string cp = std::string(
		std::istreambuf_iterator<char>(common_file),
		std::istreambuf_iterator<char>()
	);

	std::ostringstream vertex_fn;
	vertex_fn << "shaders\\" << m_programName << "_vertex.glsl";
	std::ifstream vertex_file(VTKModel::getResourcePath(vertex_fn.str()));
	std::string vp = std::string(
		std::istreambuf_iterator<char>(vertex_file),
		std::istreambuf_iterator<char>()
	);
	if (vp.length() == 0) {
		throw std::exception((std::string("Empty Vertex shader detected, exiting - ") + vertex_fn.str()).c_str());
	}

	std::ostringstream fragment_fn;
	fragment_fn << "shaders\\" << m_programName << "_fragment.glsl";
	std::ifstream fragment_file(VTKModel::getResourcePath(fragment_fn.str()));
	std::string fp = std::string(
		std::istreambuf_iterator<char>(fragment_file),
		std::istreambuf_iterator<char>()
	);
	if (fp.length() == 0) {
		throw std::exception((std::string("Empty Fragment shader detected, exiting - ") + fragment_fn.str()).c_str());
	}

	newVertexProgram = cp + vp;
	newFragmentProgram = cp + fp;

}

void VTKModel::release() {

	//TODO: deallocate gl buffer
	if (m_triangles) {
		m_triangles->Delete();
	}

	if (m_program) {
		glDeleteProgram(m_program);
		m_program = 0;
	}

	if (m_glBuffer) {
		m_glBuffer->release();
		delete m_glBuffer;
	}

}
