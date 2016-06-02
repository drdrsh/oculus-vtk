#ifndef OVRHELPER_H
#define OVRHELPER_H

#include <GL/glew.h>
#include <GL/GL.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>


#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#include "Extras/OVR_Math.h"

#include "definitions.h"
#include "OVRBuffers.h"

class OVRHelper {

public:

	static OVRHelper * getInstance();

	void init();
	void release();
	void render(void(*renderCallback)(glm::mat4, glm::mat4));
	void setLookAt(glm::vec3 pos, glm::vec3 point);
	glm::ivec2 getViewportSize();

	~OVRHelper();

private:

	OVRHelper() { 
		m_cameraPosition = glm::vec4(0.0f);
		m_lookAtPoint = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
	}

	static OVRHelper *m_instance;


	GLuint			m_uiFBOId;
	TextureBuffer*  m_pEyeRenderTexture[2] = { nullptr, nullptr };
	DepthBuffer*    m_pEyeDepthBuffer[2] = { nullptr, nullptr };

	ovrEyeRenderDesc m_eyeRenderDesc[2];

	ovrHmdDesc m_hmdDesc;

	glm::mat4		m_projection;
	glm::mat4		m_view;


	float			  m_cameraYaw = 3.141592f;
	glm::vec4         m_cameraPosition;
	glm::vec4		  m_lookAtPoint;
	ovrMirrorTexture  m_pMirrorTexture = nullptr;
	GLuint            m_uiMirrorFBO = 0;

	ovrSession		  m_HMD;
	ovrGraphicsLuid m_luid;


};
#endif