#ifndef OVRHELPER_H
#define OVRHELPER_H

#include <GL/glew.h>
#include <GL/GL.h>

#include "Kernel/OVR_System.h"
#include "OVR_CAPI_0_8_0.h"
#include "OVR_CAPI_GL.h"

#include "Extras/OVR_Math.h"

class OVRHelper {

public:

	static OVRHelper * getInstance();

	void init();
	void render(void(*renderCallback)(glm::mat4, glm::mat4));
	
	~OVRHelper();

private:

	OVRHelper();

	static OVRHelper *m_instance;


	GLuint			m_uiFBOId;
	TextureBuffer*  m_pEyeRenderTexture[2] = { nullptr, nullptr };
	DepthBuffer*    m_pEyeDepthBuffer[2] = { nullptr, nullptr };

	ovrEyeRenderDesc m_eyeRenderDesc[2];

	ovrHmdDesc m_hmdDesc;

	glm::mat4		m_projection;
	glm::mat4		m_view;


	float			m_cameraYaw = 3.141592f;
	glm::vec4       m_cameraPosition;
	ovrGLTexture*   m_pMirrorTexture = nullptr;
	GLuint          m_uiMirrorFBO = 0;

	ovrHmd			m_HMD;
	ovrGraphicsLuid m_luid;


};
#endif