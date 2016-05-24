#include "VTKModel.h"
#include <thread>
#include <fstream>
#include <algorithm>
#include <vector>
#include <regex>
#include <map>
#include <sstream>
#include <streambuf>
#include <iostream>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkGenericCell.h>
#include <vtkTriangle.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkDecimatePro.h>
#include <vtkOBBTree.h>



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
#include <vtkPolygon.h>
#include <vtkFillHolesFilter.h>


#include "definitions.h"
#include <glm\glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	surface->SetValue(0, .5);

	surface->Update();

	return surface->GetOutput();
}

vtkSmartPointer<vtkPolyData> VTKModel::getSquarePolyData() {

	VTK_NEW(points, vtkPoints);
	VTK_NEW(polygon, vtkPolygon);
	VTK_NEW(polygons, vtkCellArray);
	VTK_NEW(polygonPolyData, vtkPolyData);
	
	// Setup four points
	points->InsertNextPoint(0.0, 0.0, 0.0);
	points->InsertNextPoint(1.0, 1.0, 0.0);
	points->InsertNextPoint(0.0, 1.0, 0.0);

	// Create the polygon
	polygon->GetPointIds()->SetNumberOfIds(3); //make a quad
	polygon->GetPointIds()->SetId(0, 0);
	polygon->GetPointIds()->SetId(1, 1);
	polygon->GetPointIds()->SetId(2, 2);

	// Add the polygon to a list of polygons
	polygons->InsertNextCell(polygon);

	// Create a PolyData
	polygonPolyData->SetPoints(points);
	polygonPolyData->SetPolys(polygons);

	vtkSmartPointer<vtkFloatArray> colors = vtkFloatArray::New();
	colors->SetName("Colors");
	colors->SetNumberOfComponents(4);
	colors->SetNumberOfTuples(3);
	colors->SetTuple4(0, 1.0, 0.0, 0.0, 1.0);
	colors->SetTuple4(1, 0.0, 1.0, 0.0, 1.0);
	colors->SetTuple4(2, 0.0, 0.0, 1.0, 1.0);

	polygonPolyData->GetPointData()->AddArray(colors);

	return polygonPolyData;
}

VTKModel::VTKModel(std::string filename) {

	VTK_NEW(volume, vtkImageData);
	VTK_NEW(reader, vtkNIFTIImageReader);
	VTK_NEW(surface, vtkMarchingCubes);
	VTK_NEW(smoother, vtkSmoothPolyDataFilter);
	VTK_NEW(decimate, vtkDecimatePro);

	m_triangles = getSquarePolyData();
	//m_triangles = getImagePolyData(filename);
	//m_triangles = getSpherePolyData();
	m_isInitialized = false;
}


void VTKModel::allocateBuffers() {
	//FIXME: Memory leak
	/*
	if (m_vertices && m_indices) {
	m_vertices->~VertexBuffer();
	m_indices->~IndexBuffer();
	}
	*/

	m_vBuffer = new VTKVertexBuffer(m_triangles);
	m_iBuffer = new VTKIndexBuffer(m_triangles);

}

void VTKModel::init() {

	m_isInitialized = true;
	if (!m_isThreadRunning) {
		//m_watcherThread = std::thread(VTKModel::watchFileForNewProgram);
		m_isThreadRunning = true;
	}

	readProgram();
	GLERR;

	reloadProgram();
	GLERR;

	allocateBuffers();

}



void VTKModel::render(glm::mat4 obj, glm::mat4 proj, glm::mat4 view) {

	if (!m_isInitialized) {
		init();
	}

	if (newFragmentProgram.length() != 0 && newVertexProgram.length() != 0) {
		reloadProgram();
	}

	glm::mat4 norm = obj;

	glm::mat4 wvp = proj * view * obj;

	m_matObj = obj;
	m_matProj = proj;
	m_matView = view;
	m_matNorm = norm;


	char variableName[256];


	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);

	//glEnable(GL_MULTISAMPLE);
	glUseProgram(m_program);

	glUniformMatrix4fv(ufLoc("matWVP"), 1, GL_FALSE, glm::value_ptr(wvp));
	glUniformMatrix4fv(ufLoc("matProjection"), 1, GL_FALSE, glm::value_ptr(proj));
	glUniformMatrix4fv(ufLoc("matView"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(ufLoc("matObject"), 1, GL_FALSE, glm::value_ptr(obj));
	glUniformMatrix4fv(ufLoc("matNormal"), 1, GL_FALSE, glm::value_ptr(norm));

	m_iBuffer->bind();
	m_vBuffer->bind(m_program);

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

	m_iBuffer->draw(GL_TRIANGLES);

	m_vBuffer->unbind(m_program);
	m_iBuffer->unbind();

	glUseProgram(0);
	//glDisable(GL_MULTISAMPLE);

}



std::string VTKModel::getResourcePath(std::string p) {

#ifdef _DEBUG
	std::string vrPath = "C:\\OGLVTK\\";
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
	std::ifstream myfile(VTKModel::getResourcePath("shaders\\common.glsl"));
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
			log_file.open(VTKModel::getResourcePath("log\\error.txt"), std::ofstream::out | std::ofstream::app);
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
	log_file.open(VTKModel::getResourcePath("log\\error.txt"), std::ofstream::trunc);
	log_file.close();

	GLuint vertexShader = createShader(GL_VERTEX_SHADER, newVertexProgram.data());
	GLERR;
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, newFragmentProgram.data());
	GLERR;
	if (vertexShader == 0 || fragmentShader == 0) {
		newVertexProgram.clear();
		newFragmentProgram.clear();
		if (vertexShader != 0) {
			glDeleteShader(vertexShader);
			GLERR;
		}
		if (fragmentShader != 0) {
			glDeleteShader(fragmentShader);
			GLERR;
		}
		return;
	}

	GLuint newProgram = glCreateProgram();
	GLERR;

	glAttachShader(newProgram, vertexShader);
	GLERR;
	glAttachShader(newProgram, fragmentShader);
	GLERR;

	glLinkProgram(newProgram);
	GLERR;

	glDetachShader(newProgram, vertexShader);
	GLERR;
	glDetachShader(newProgram, fragmentShader);
	GLERR;

	glDeleteShader(vertexShader);
	GLERR;
	glDeleteShader(fragmentShader);
	GLERR;

	GLint r;
	glGetProgramiv(newProgram, GL_LINK_STATUS, &r);
	GLERR;
	if (!r) {
		GLchar msg[1024];
		glGetProgramInfoLog(newProgram, sizeof(msg), 0, msg);
		GLERR;
		std::ofstream log_file;
		log_file.open(VTKModel::getResourcePath("log\\error.txt"), std::ofstream::out | std::ofstream::app);
		log_file << "Linking program failed: " << msg;
		log_file << newVertexProgram;
		log_file << newFragmentProgram;
		log_file.close();
	}
	else {
		if (m_program != -1) {
			glDeleteProgram(m_program);
		}
		GLERR;
		m_program = newProgram;
	}
	newVertexProgram.clear();
	newFragmentProgram.clear();

}


void VTKModel::readProgram() {

	std::ifstream common_file(VTKModel::getResourcePath("shaders\\common.glsl"));
	std::string cp = std::string(
		std::istreambuf_iterator<char>(common_file),
		std::istreambuf_iterator<char>()
		);

	std::ifstream vertex_file(VTKModel::getResourcePath("shaders\\vertex.glsl"));
	std::string vp = std::string(
		std::istreambuf_iterator<char>(vertex_file),
		std::istreambuf_iterator<char>()
	);

	std::ifstream fragment_file(VTKModel::getResourcePath("shaders\\fragment.glsl"));
	std::string fp = std::string(
		std::istreambuf_iterator<char>(fragment_file),
		std::istreambuf_iterator<char>()
	);

	newVertexProgram = cp + vp;
	newFragmentProgram = cp + fp;

}

void VTKModel::release() {

	//TODO: deallocate gl buffer
	if (m_triangles) {
		m_triangles->Delete();
	}


	if (m_vBuffer) {
		delete m_vBuffer;
	}

	if (m_iBuffer) {
		delete m_iBuffer;
	}

	if (m_program) {
		glDeleteProgram(m_program);
		m_program = 0;
	}

	//FIXME: Release resources

}
