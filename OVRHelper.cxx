
#include <GL/glew.h>
#include <GL/GL.h>

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "definitions.h"
#include "Extras/OVR_Math.h"
#include "Kernel/OVR_System.h"
#include "OVR_CAPI_0_8_0.h"
#include "OVR_CAPI_GL.h"
#include "OVRHelper.h"

#include <glm/gtx/transform2.hpp>
#include <glm/gtx/projection.hpp>


#ifdef _WIN32
	#include <GL/wglew.h>
#else
	#include <GL/glxew.h>
#endif

OVRHelper * OVRHelper::getInstance() {

	if (!m_instance) {
		m_instance = new OVRHelper();
	}

	return m_instance;
}

OVRHelper::~OVRHelper() {
	if (!m_instance) {
		delete m_instance;
	}
}

void OVRHelper::render(void(*renderCallback)(glm::mat4, glm::mat4)) {
	
	if (!renderCallback) {
		return;
	}

	ovrVector3f ViewOffset[2] = {
		m_eyeRenderDesc[0].HmdToEyeViewOffset,
		m_eyeRenderDesc[1].HmdToEyeViewOffset
	};
	ovrPosef EyeRenderPose[2];

	double ftiming = ovr_GetPredictedDisplayTime(m_HMD, 0);
	// Keeping sensorSampleTime as close to ovr_GetTrackingState as possible - fed into the layer
	double sensorSampleTime = ovr_GetTimeInSeconds();
	ovrTrackingState hmdState = ovr_GetTrackingState(m_HMD, ftiming, ovrTrue);
	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, ViewOffset, EyeRenderPose);

	for (int eye = 0; eye < 2; ++eye)
	{
		// Increment to use next texture, just before writing
		m_pEyeRenderTexture[eye]->TextureSet->CurrentIndex = (m_pEyeRenderTexture[eye]->TextureSet->CurrentIndex + 1) % m_pEyeRenderTexture[eye]->TextureSet->TextureCount;

		// Switch to eye render target
		m_pEyeRenderTexture[eye]->SetAndClearRenderSurface(m_pEyeDepthBuffer[eye]);

		// Get view and projection matrices
		glm::mat4 rollPitchYaw = glm::eulerAngleY(m_cameraYaw);

		glm::quat eyeOrientation;
		eyeOrientation.x = EyeRenderPose[eye].Orientation.x;
		eyeOrientation.y = EyeRenderPose[eye].Orientation.y;
		eyeOrientation.z = EyeRenderPose[eye].Orientation.z;
		eyeOrientation.w = EyeRenderPose[eye].Orientation.w;

		float empty = 0.0f;

		glm::vec4 up = glm::vec4();
		up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f; up[3] = empty;

		glm::vec4 forward = glm::vec4();
		forward[0] = 0.0f; forward[1] = 0.0f; forward[2] = -1.0f; forward[3] = empty;

		glm::vec4 position = glm::vec4();
		position[0] = EyeRenderPose[eye].Position.x;
		position[1] = EyeRenderPose[eye].Position.y;
		position[2] = EyeRenderPose[eye].Position.z;
		position[3] = empty;


		glm::mat4 finalRollPitchYaw = rollPitchYaw * glm::mat4_cast(eyeOrientation);
		glm::vec4 finalUp = finalRollPitchYaw * up;
		glm::vec4 finalForward = finalRollPitchYaw * forward;
		glm::vec4 shiftedEyePos = m_cameraPosition + rollPitchYaw * position;

		//m_lastUp = finalUp;
		//m_lastLookAt = finalForward;

		m_view  = glm::mat4(1.0);  //loadIdentity
		m_view *= glm::lookAt(
			(glm::vec3)shiftedEyePos, 
			(glm::vec3)(shiftedEyePos + finalForward),
			(glm::vec3)finalUp
		);
		ovrMatrix4f m = ovrMatrix4f_Projection(m_hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_RightHanded);
		m_projection = glm::make_mat4(&m.M);
		
		renderCallback(m_projection, m_view);


		// Avoids an error when calling SetAndClearRenderSurface during next iteration.
		// Without this, during the next while loop iteration SetAndClearRenderSurface
		// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
		// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
		m_pEyeRenderTexture[eye]->UnsetRenderSurface();
	}

	// Do distortion rendering, Present and flush/sync

	// Set up positional data.
	ovrViewScaleDesc viewScaleDesc;
	viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	viewScaleDesc.HmdToEyeViewOffset[0] = ViewOffset[0];
	viewScaleDesc.HmdToEyeViewOffset[1] = ViewOffset[1];


	ovrLayerEyeFov ld;
	ld.Header.Type = ovrLayerType_EyeFov;
	ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

	for (int eye = 0; eye < 2; ++eye)
	{
		ld.ColorTexture[eye] = m_pEyeRenderTexture[eye]->TextureSet;
		ld.Viewport[eye] = Recti(m_pEyeRenderTexture[eye]->GetSize());
		ld.Fov[eye] = m_hmdDesc.DefaultEyeFov[eye];
		ld.RenderPose[eye] = EyeRenderPose[eye];
		ld.SensorSampleTime = sensorSampleTime;
	}

	ovrLayerHeader* layers = &ld.Header;
	ovrResult result = ovr_SubmitFrame(m_HMD, 0, &viewScaleDesc, &layers, 1);
	// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
	//VALIDATE(OVR_SUCCESS(result), "Failed to submit frame to HMD");

	//isVisible = (result == ovrSuccess);

	// Blit mirror texture to back buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_uiMirrorFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLint w = this->m_pMirrorTexture->OGL.Header.TextureSize.w;
	GLint h = this->m_pMirrorTexture->OGL.Header.TextureSize.h;
	glBlitFramebuffer(0, h, w, 0,
		0, 0, w, h,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);


}

void OVRHelper::init() {

	ovrResult result = ovr_Create(&this->m_HMD, &this->m_luid);

	m_hmdDesc = ovr_GetHmdDesc(m_HMD);

	// Setup Window and Graphics
	// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
	ovrSizei windowSize = { m_hmdDesc.Resolution.w / 2, m_hmdDesc.Resolution.h / 2 };

	RECT size = { 0, 0, windowSize.w, windowSize.h };

	// Make eye render buffers
	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(this->m_HMD, ovrEyeType(eye), m_hmdDesc.DefaultEyeFov[eye], 1);
		this->m_pEyeRenderTexture[eye] = new TextureBuffer(this->m_HMD, true, true, idealTextureSize, 1, NULL, 1);
		this->m_pEyeDepthBuffer[eye] = new DepthBuffer(this->m_pEyeRenderTexture[eye]->GetSize(), 0);

		if (!this->m_pEyeRenderTexture[eye]->TextureSet)
		{
			//VALIDATE(false, "Failed to create texture.");
			return;
		}
	}

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	result = ovr_CreateMirrorTextureGL(this->m_HMD, GL_SRGB8_ALPHA8, windowSize.w, windowSize.h, reinterpret_cast<ovrTexture**>(&this->m_pMirrorTexture));
	if (!OVR_SUCCESS(result)) {
		//VALIDATE(OVR_SUCCESS(result), "Failed to create mirror texture.");
		return;
	}

	// Configure the mirror read buffer
	glGenFramebuffers(1, &this->m_uiMirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_uiMirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->m_pMirrorTexture->OGL.TexId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	m_eyeRenderDesc[0] = ovr_GetRenderDesc(this->m_HMD, ovrEye_Left, m_hmdDesc.DefaultEyeFov[0]);
	m_eyeRenderDesc[1] = ovr_GetRenderDesc(this->m_HMD, ovrEye_Right, m_hmdDesc.DefaultEyeFov[1]);

	// Turn off vsync to let the compositor do its magic
#ifdef _WIN32
	wglSwapIntervalEXT(0);
#else
	glXSwapIntervalEXT(0);
#endif




	// enable the z buffer
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);


	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);

	glGenFramebuffers(1, &this->m_uiFBOId);

	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);



}