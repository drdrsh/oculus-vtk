#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#else
#include <GL/glxew.h>
#endif
#include <GL/GL.h>

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/vec2.hpp>

#include "Extras/OVR_Math.h"
#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"

#include "OVRHelper.h"
#include "definitions.h"

OVRHelper * OVRHelper::m_instance;

OVRHelper * OVRHelper::getInstance() {

	if (!m_instance) {
		m_instance = new OVRHelper();
	}

	return m_instance;
}


void OVRHelper::setLookAt(glm::vec3 pos, glm::vec3 point) {
	m_cameraPosition = glm::vec4(pos, 1.0f);
	m_lookAtPoint = glm::vec4(point, 1.0f);
}

void OVRHelper::release() {

	for (int eye = 0; eye < 2; ++eye) {
		delete this->m_pEyeRenderTexture[eye];
		delete this->m_pEyeDepthBuffer[eye];
	}
	ovr_Destroy(this->m_HMD);
	ovr_Shutdown();

	if (!m_instance) {
		delete m_instance;
	}
}

OVRHelper::~OVRHelper() {
	release();
}



glm::ivec2 OVRHelper::getViewportSize() {

	glm::ivec2 size;
	size.x = m_hmdDesc.Resolution.w / 2;
	size.y = m_hmdDesc.Resolution.h / 2;
	return size;
}

void OVRHelper::render(void(*renderCallback)(glm::mat4, glm::mat4)) {
	
	if (!renderCallback) {
		return;
	}

	ovrVector3f ViewOffset[2] = {
		m_eyeRenderDesc[0].HmdToEyeOffset,
		m_eyeRenderDesc[1].HmdToEyeOffset
	};
	ovrPosef EyeRenderPose[2];

	double ftiming = ovr_GetPredictedDisplayTime(m_HMD, 0);
	// Keeping sensorSampleTime as close to ovr_GetTrackingState as possible - fed into the layer
	double sensorSampleTime = ovr_GetTimeInSeconds();
	ovrTrackingState hmdState = ovr_GetTrackingState(m_HMD, ftiming, ovrTrue);
	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, ViewOffset, EyeRenderPose);

	for (int eye = 0; eye < 2; ++eye)
	{
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
		up.x = 0.0f; 
		up.y = 1.0f; 
		up.z = 0.0f; 
		up.w = 0.0f;   //This is a direction, we set w to 0.0f
		
		glm::vec4 forward = glm::vec4();
		forward.x =  0.0f;
		forward.y =  0.0f;
		forward.z = -1.0f;
		forward.w =  0.0f;     //This is a direction, we set w to 0.0f


		glm::vec4 position = glm::vec4();
		position.x = EyeRenderPose[eye].Position.x;
		position.y = EyeRenderPose[eye].Position.y;
		position.z = EyeRenderPose[eye].Position.z;
		position.w = 1.0f;				 //This is a position, we set w to 1.0f


		glm::mat4 finalRollPitchYaw = rollPitchYaw * glm::mat4_cast(eyeOrientation);
		glm::vec4 finalUp = finalRollPitchYaw * up;
		glm::vec4 finalForward = finalRollPitchYaw * forward;
		glm::vec4 shiftedEyePos = m_cameraPosition + rollPitchYaw * position;

		m_view = glm::lookAt(
			(glm::vec3)m_cameraPosition,
			(glm::vec3)m_lookAtPoint,
			(glm::vec3)up
		);

		m_view = glm::lookAt(
			(glm::vec3)shiftedEyePos, 
			(glm::vec3)(shiftedEyePos + finalForward),
			(glm::vec3)finalUp
		);

		//Possible unnecessary performance hit, converting from ovrMatrix4f to glm::mat4
		ovrMatrix4f m = ovrMatrix4f_Projection(m_hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
		m_projection = glm::make_mat4((float*)&m.M);
		m_projection = glm::transpose(m_projection);

		renderCallback(m_projection, m_view);


		// Avoids an error when calling SetAndClearRenderSurface during next iteration.
		// Without this, during the next while loop iteration SetAndClearRenderSurface
		// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
		// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
		m_pEyeRenderTexture[eye]->UnsetRenderSurface();

		// Commit changes to the textures so they get picked up frame
		m_pEyeRenderTexture[eye]->Commit();
	}

	// Do distortion rendering, Present and flush/sync

	ovrLayerEyeFov ld;
	ld.Header.Type = ovrLayerType_EyeFov;
	ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

	for (int eye = 0; eye < 2; ++eye)
	{
		ld.ColorTexture[eye] = m_pEyeRenderTexture[eye]->TextureChain;
		ld.Viewport[eye] = OVR::Recti(m_pEyeRenderTexture[eye]->GetSize());
		ld.Fov[eye] = m_hmdDesc.DefaultEyeFov[eye];
		ld.RenderPose[eye] = EyeRenderPose[eye];
		ld.SensorSampleTime = sensorSampleTime;
	}

	ovrLayerHeader* layers = &ld.Header;
	ovrResult result = ovr_SubmitFrame(m_HMD, 0, nullptr, &layers, 1);
	// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
	VALIDATE(OVR_SUCCESS(result), "Failed to submit frame to HMD");

	ovrSessionStatus sessionStatus;
	ovr_GetSessionStatus(m_HMD, &sessionStatus);
	if (sessionStatus.ShouldQuit) {
		exit(-1);
	}
	if (sessionStatus.ShouldRecenter) {
		ovr_RecenterTrackingOrigin(m_HMD);
	}

	// Blit mirror texture to back buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_uiMirrorFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glm::ivec2 viewportSize = getViewportSize();
	GLint w = viewportSize.x;
	GLint h = viewportSize.y;
	glBlitFramebuffer(
		0, h, w, 0,
		0, 0, w, h,
		GL_COLOR_BUFFER_BIT, GL_NEAREST
	);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);


}

void OVRHelper::init() {

	ovrResult result;
	
	result = ovr_Initialize(nullptr);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize HMD");


	result = ovr_Create(&this->m_HMD, &this->m_luid);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize HMD");

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

		if (!this->m_pEyeRenderTexture[eye]->TextureChain)
		{
			VALIDATE(false, "Failed to create texture.");
			return;
		}
	}

	ovrMirrorTextureDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = windowSize.w;
	desc.Height = windowSize.h;
	desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	result = ovr_CreateMirrorTextureGL(this->m_HMD, &desc, &m_pMirrorTexture);
	if (!OVR_SUCCESS(result)) {
		VALIDATE(OVR_SUCCESS(result), "Failed to create mirror texture.");
		return;
	}

	GLuint texId;
	ovr_GetMirrorTextureBufferGL(m_HMD, m_pMirrorTexture, &texId);

	// Configure the mirror read buffer
	glGenFramebuffers(1, &this->m_uiMirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_uiMirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
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

	glGenFramebuffers(1, &this->m_uiFBOId);
}