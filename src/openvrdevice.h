/*
 * openvrdevice.h
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 */

#ifndef _OSG_OPENVRDEVICE_H_
#define _OSG_OPENVRDEVICE_H_

// Include the OpenVR SDK
#include <openvr.h>

#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Version>
#include <osg/FrameBufferObject>
#include <osgViewer/Renderer>
#include <osg/Program>
#include <osg/Shader>  


#if(OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))
    typedef osg::GLExtensions OSG_GLExtensions;
    typedef osg::GLExtensions OSG_Texture_Extensions;
#else
    typedef osg::FBOExtensions OSG_GLExtensions;
    typedef osg::Texture::Extensions OSG_Texture_Extensions;
#endif

class COSGRenderModel
{
public:
	COSGRenderModel(const std::string & sRenderModelName);
	~COSGRenderModel();

	bool BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }

private:
	osg::ref_ptr<osg::Vec3Array> vertices;
	osg::ref_ptr<osg::Vec3Array> normals;
	osg::ref_ptr<osg::Vec2Array> texcoords;
	osg::ref_ptr<osg::UShortArray> vertIndexes;
	osg::Texture2D* tex2D;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};

static bool g_bPrintf = true;

class OpenVRTextureBuffer : public osg::Referenced
{
public:
    OpenVRTextureBuffer(osg::ref_ptr<osg::State> state, int width, int height, int msaaSamples);
    void destroy(osg::GraphicsContext* gc);
    GLuint getTexture() { return m_Resolve_ColorTex; }
    int textureWidth() const { return m_width; }
    int textureHeight() const { return m_height; }
    int samples() const { return m_samples; }
    void onPreRender(osg::RenderInfo& renderInfo);
    void onPostRender(osg::RenderInfo& renderInfo);

protected:
    ~OpenVRTextureBuffer() {}

    friend class OpenVRMirrorTexture;
    GLuint m_Resolve_FBO; // MSAA FBO is copied to this FBO after render.
    GLuint m_Resolve_ColorTex; // color texture for above FBO.
    GLuint m_MSAA_FBO; // framebuffer for MSAA RTT
    GLuint m_MSAA_ColorTex; // color texture for MSAA RTT 
    GLuint m_MSAA_DepthTex; // depth texture for MSAA RTT
    GLint m_width; // width of texture in pixels
    GLint m_height; // height of texture in pixels
    int m_samples;  // sample width for MSAA

};

class OpenVRMirrorTexture : public osg::Referenced
{
public:
    OpenVRMirrorTexture(osg::ref_ptr<osg::State> state, GLint width, GLint height);
    void destroy(osg::GraphicsContext* gc);
    void blitTexture(osg::GraphicsContext* gc, OpenVRTextureBuffer* leftEye,  OpenVRTextureBuffer* rightEye);
protected:
    ~OpenVRMirrorTexture() {}

    GLuint m_mirrorFBO;
    GLuint m_mirrorTex;
    GLint m_width;
    GLint m_height;
};

class OpenVRInitialDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        osg::GraphicsOperation* graphicsOperation = renderInfo.getCurrentCamera()->getRenderer();
        osgViewer::Renderer* renderer = dynamic_cast<osgViewer::Renderer*>(graphicsOperation);
        if (renderer != nullptr)
        {
            // Disable normal OSG FBO camera setup because it will undo the MSAA FBO configuration.
            renderer->setCameraRequiresSetUp(false);
        }
    }
};

class OpenVRPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    OpenVRPreDrawCallback(osg::Camera* camera, OpenVRTextureBuffer* textureBuffer)
        : m_camera(camera)
        , m_textureBuffer(textureBuffer)
    {
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const;
protected:
    osg::Camera* m_camera;
    OpenVRTextureBuffer* m_textureBuffer;

};

class OpenVRPostDrawCallback : public osg::Camera::DrawCallback
{
public:
    OpenVRPostDrawCallback(osg::Camera* camera, OpenVRTextureBuffer* textureBuffer)
        : m_camera(camera)
        , m_textureBuffer(textureBuffer)
    {
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const;
protected:
    osg::Camera* m_camera;
    OpenVRTextureBuffer* m_textureBuffer;

};


class OpenVRDevice : public osg::Referenced
{

public:
    typedef enum Eye_
    {
        LEFT = 0,
        RIGHT = 1,
        COUNT = 2
    } Eye;

	// VR 环境模型
	osg::ref_ptr<osg::Node> sceneNode;
	osg::ref_ptr<osg::Node> controllerNode;
	osg::ref_ptr<osg::Node> renderModelNode;
	osg::ref_ptr<osg::Node> companionWindowNode;
	COSGRenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
	std::vector< COSGRenderModel * > m_vecRenderModels;
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];


	bool CreateAllShaders(osg::ref_ptr<osg::StateSet> sceneStateSet, 
		osg::ref_ptr<osg::StateSet> controllerStateSet,
		osg::ref_ptr<osg::StateSet> renderModelStateSet, 
		osg::ref_ptr<osg::StateSet> companionWindowStateSet);
	bool CreateRenderModelShaders(osg::ref_ptr<osg::StateSet> renderModelStateSet);
	COSGRenderModel* FindOrLoadRenderModel(const char *pchRenderModelName);
	std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL);
	void SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);
	void SetupRenderModels();
	bool BInitEnv();
    OpenVRDevice(float nearClip, float farClip, const float worldUnitsPerMetre = 1.0f, const int samples = 0);
    void createRenderBuffers(osg::ref_ptr<osg::State> state);
    void init();
    void shutdown(osg::GraphicsContext* gc);

	void ProcessVREvent(const vr::VREvent_t& event);
	void HandleInput();

    static bool hmdPresent();
    bool hmdInitialized() const;

    osg::Matrix projectionMatrixCenter() const;
    osg::Matrix projectionMatrixLeft() const;
    osg::Matrix projectionMatrixRight() const;

    osg::Matrix projectionOffsetMatrixLeft() const;
    osg::Matrix projectionOffsetMatrixRight() const;

    osg::Matrix viewMatrixLeft() const;
    osg::Matrix viewMatrixRight() const;

    float nearClip() const { return m_nearClip;    }
    float farClip() const { return m_farClip; }

    void resetSensorOrientation() const;
    void updatePose();
	void RenderModelRendering();

    osg::Vec3 position() const { return m_position; }
    osg::Quat orientation() const { return m_orientation;  }

    osg::Camera* createRTTCamera(OpenVRDevice::Eye eye, osg::Transform::ReferenceFrame referenceFrame, const osg::Vec4& clearColor, osg::GraphicsContext* gc = 0) const;

    bool submitFrame();
    void blitMirrorTexture(osg::GraphicsContext* gc);

    osg::GraphicsContext::Traits* graphicsContextTraits() const;
protected:
    ~OpenVRDevice(); // Since we inherit from osg::Referenced we must make destructor protected

    void calculateEyeAdjustment();
    void calculateProjectionMatrices();

    void trySetProcessAsHighPriority() const;

    vr::IVRSystem* m_vrSystem;
    vr::IVRRenderModels* m_vrRenderModels;
    const float m_worldUnitsPerMetre;

    osg::ref_ptr<OpenVRTextureBuffer> m_textureBuffer[2];
    osg::ref_ptr<OpenVRMirrorTexture> m_mirrorTexture;

    osg::Matrixf m_leftEyeProjectionMatrix;
    osg::Matrixf m_rightEyeProjectionMatrix;
    osg::Vec3f m_leftEyeAdjust;
    osg::Vec3f m_rightEyeAdjust;

    osg::Vec3 m_position;
    osg::Quat m_orientation;

    float m_nearClip;
    float m_farClip;
    int m_samples;
private:


    std::string GetDeviceProperty(vr::TrackedDeviceProperty prop);
    OpenVRDevice(const OpenVRDevice&); // Do not allow copy
    OpenVRDevice& operator=(const OpenVRDevice&); // Do not allow assignment operator.
};


class OpenVRRealizeOperation : public osg::GraphicsOperation
{
public:
    explicit OpenVRRealizeOperation(osg::ref_ptr<OpenVRDevice> device) :
        osg::GraphicsOperation("OpenVRRealizeOperation", false), m_device(device), m_realized(false) {}
    virtual void operator () (osg::GraphicsContext* gc);
    bool realized() const { return m_realized; }
protected:
    OpenThreads::Mutex  _mutex;
    osg::observer_ptr<OpenVRDevice> m_device;
    bool m_realized;
};


class OpenVRSwapCallback : public osg::GraphicsContext::SwapCallback
{
public:
    explicit OpenVRSwapCallback(osg::ref_ptr<OpenVRDevice> device) : m_device(device), m_frameIndex(0) {}
    void swapBuffersImplementation(osg::GraphicsContext* gc);
    int frameIndex() const { return m_frameIndex; }
private:
    osg::observer_ptr<OpenVRDevice> m_device;
    int m_frameIndex;
};



#endif /* _OSG_OPENVRDEVICE_H_ */