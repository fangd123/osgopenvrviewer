/*
 * viewerexample.cpp
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 */

#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include "openvrviewer.h"
#include "openvreventhandler.h"

class GraphicsWindowViewer : public osgViewer::Viewer
{
public:

    GraphicsWindowViewer(osg::ref_ptr<OpenVRDevice> openvr,osg::ArgumentParser& arguments, osgViewer::GraphicsWindow* graphicsWindow)
        : osgViewer::Viewer(arguments), _graphicsWindow(graphicsWindow)
    {
		openvrDevice = openvr;
		fake_position_x = 0;
		fake_position_y = 0;
		trigger = false;
    }

    virtual void eventTraversal()
    {
		// 添加VR事件响应，映射成鼠标事件
		// 按下trigger时，进行旋转
		// 按下trackpad时，进行缩放
		if (openvrDevice->controllerEventResult != -1)
		{
			osg::ref_ptr<osgGA::GUIEventAdapter> controllerBeforeEvent = new osgGA::GUIEventAdapter;
			osg::ref_ptr<osgGA::GUIEventAdapter> controllerEvent = new osgGA::GUIEventAdapter;
			osg::ref_ptr<osgGA::GUIEventAdapter> controllerAfterEvent = new osgGA::GUIEventAdapter;

			controllerBeforeEvent->setEventType(osgGA::GUIEventAdapter::PUSH);
			controllerAfterEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);

			switch (openvrDevice->controllerEventResult)
			{

			// Touchpad 按下
			case OpenVRDevice::Touchpad_Press:
			{
				double angle = VectorAngle(openvrDevice->m_touchpadTouchPosition);
				
				TouchpadLocation location = GetTouchpadLocation(angle);
				controllerEvent->setEventType(osgGA::GUIEventAdapter::DRAG);

				if(location == UP)
				{
					if(trigger)
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
					else
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
					
					controllerEvent->setY(fake_position_y);
				}
				else if (location == DOWN)
				{
					if (trigger)
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
					else
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
					controllerEvent->setY(-fake_position_y);
				}
				else if (location == LEFT)
				{
					if (trigger)
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
					else
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
					controllerEvent->setX(-fake_position_x);

				}
				else if (location == RIGHT)
				{
					if (trigger)
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
					else
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
					controllerEvent->setX(fake_position_x);
				}

				
				fake_position_x += 1;
				fake_position_y += 1;
			}
			break;
			// 按键弹起后，首先释放release消息
			// 然后复位所有信息
			case OpenVRDevice::Touchpad_Unpress:
			{
				controllerEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				controllerEvent->setButtonMask(0);
				controllerEvent->setX(fake_position_x);
				controllerEvent->setY(fake_position_y);
				fake_position_x = 0;
				fake_position_y = 0;
				openvrDevice->controllerEventResult = -1;
			}
			break;
			case OpenVRDevice::Trigger_Press:
			{
				if (trigger == false)
					trigger = true;
			}
			case OpenVRDevice::Trigger_Unpress:
			{
				trigger = false;
				controllerEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				controllerEvent->setButtonMask(0);
				controllerEvent->setX(fake_position_x);
				controllerEvent->setY(fake_position_y);
				fake_position_x = 0;
				fake_position_y = 0;
				openvrDevice->controllerEventResult = -1;
			}
			break;
			default:
			{
				controllerEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				printf("release actiated!");
				controllerEvent->setButtonMask(0);
			}
			}
			_graphicsWindow->getEventQueue()->addEvent(controllerEvent);
		}
		
        if (_graphicsWindow.valid() && _graphicsWindow->checkEvents())
        {

            osgGA::EventQueue::Events events;
            _graphicsWindow->getEventQueue()->copyEvents(events);
            osgGA::EventQueue::Events::iterator itr;
            for (itr = events.begin(); itr != events.end(); ++itr)
            {
                osgGA::GUIEventAdapter* event = dynamic_cast<osgGA::GUIEventAdapter*>(itr->get());
                if (event != nullptr && event->getEventType() == osgGA::GUIEventAdapter::CLOSE_WINDOW)
                {
                    // We have "peeked" at the event queue for the GraphicsWindow and 
                    // found a CLOSE_WINDOW event. This would normally mean that OSG 
                    // is about to release OpenGL contexts attached to the window.
                    // For some applications it is better to make the application
                    // terminate in a more "normal" way e.g. as it does when <Esc>
                    // key had been pressed.
                    // In this way we can safely perform any cleanup required after
                    // termination of the Viewer::run() loop, i.e. cleanup that would 
                    // otherwise malfunction if the earlier processing of the CLOSE_WINDOW
                    // event had already released required OpenGL contexts.
                    // So, here we "get in early" and remove the close event and append
                    // a quit application event.
                    events.erase(itr);
                    _graphicsWindow->getEventQueue()->setEvents(events);
                    _graphicsWindow->getEventQueue()->quitApplication();
                    break;
                }
            }
        }
        osgViewer::Viewer::eventTraversal();
    }
private:
    osg::ref_ptr<osgViewer::GraphicsWindow> _graphicsWindow;
	osg::ref_ptr<OpenVRDevice> openvrDevice;

	double fake_position_x;
	double fake_position_y;
	bool trigger;
	typedef enum TouchpadLocation_
	{
		UP,
		DOWN,
		LEFT,
		RIGHT
	} TouchpadLocation;

	double VectorAngle(osg::Vec2f &v2)
	{
		osg::Vec2f v1(1, 0);
		double up = v1*v2;
		double down = v1.length() * v2.length();
		double cosr = up / down;
		double angle = acos(cosr) * 180 / osg::PI;
		if (v2.x() < 0 && v2.y() < 0)
		{
			angle = -angle;
		}
		else if (v2.x() > 0 && v2.y() < 0)
		{
			angle = -angle;
		}
		return angle;
	}
	TouchpadLocation GetTouchpadLocation(double angle)
	{
		if (angle > 45 && angle < 135)
		{
			printf("down");
			return DOWN;

		}
		//上  
		if (angle < -45 && angle > -135)
		{
			printf("up");
			return UP;

		}
		//左  
		if ((angle < 180 && angle > 135) || (angle < -135 && angle > -180))
		{
			printf("left");
			return LEFT;
		}
		//右  
		if ((angle > 0 && angle < 45) || (angle > -45 && angle < 0))
		{
			controllerEvent->setX(-fake_position_x);
			return RIGHT;
		}
	}

};

int main( int argc, char** argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc, argv);
    // read the scene from the list of file specified command line arguments.
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);

    // if not loaded assume no arguments passed in, try use default cow model instead.
    if (!loadedModel) { loadedModel = osgDB::readNodeFile("cow.osgt"); }

    // Still no loaded model, then exit
    if (!loadedModel)
    {
        osg::notify(osg::ALWAYS) << "No model could be loaded and didn't find cow.osgt, terminating.." << std::endl;
        return 0;
    }

    // Create Trackball manipulator
    osg::ref_ptr<osgGA::OrbitManipulator> cameraManipulator = new osgGA::OrbitManipulator;
	cameraManipulator->setAllowThrow(false);
	cameraManipulator->setAnimationTime(0);

    const osg::BoundingSphere& bs = loadedModel->getBound();

    if (bs.valid())
    {
        // Adjust view to object view
      /* This caused a problem with the head tracking on the Vive
        osg::Vec3 modelCenter = bs.center();
        osg::Vec3 eyePos = bs.center() + osg::Vec3(0, bs.radius(), 0);
        cameraManipulator->setHomePosition(eyePos, modelCenter, osg::Vec3(0, 0, 1));
      */
    }

    // Exit if we do not have an HMD present
    if (!OpenVRDevice::hmdPresent())
    {
        osg::notify(osg::FATAL) << "Error: No valid HMD present!" << std::endl;
        return 1;
    }

    // Open the HMD
    float nearClip = 0.01f;
    float farClip = 10000.0f;
    float worldUnitsPerMetre = 1.0f;
    int samples = 16;
    osg::ref_ptr<OpenVRDevice> openvrDevice = new OpenVRDevice(nearClip, farClip, worldUnitsPerMetre, samples);

    // Exit if we fail to initialize the HMD device
    if (!openvrDevice->hmdInitialized())
    {
        // The reason for failure was reported by the constructor.
        return 1;
    }

    // Get the suggested context traits
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = openvrDevice->graphicsContextTraits();
    traits->windowName = "OsgOpenVRViewerExample";

    // Create a graphic context based on our desired traits
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);

    if (!gc)
    {
        osg::notify(osg::NOTICE) << "Error, GraphicsWindow has not been created successfully" << std::endl;
        return 1;
    }

    if (gc.valid())
    {
        gc->setClearColor(osg::Vec4(0.2f, 0.2f, 0.4f, 1.0f));
        gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    GraphicsWindowViewer viewer(openvrDevice,arguments, dynamic_cast<osgViewer::GraphicsWindow*>(gc.get()));

    // Force single threaded to make sure that no other thread can use the GL context
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.getCamera()->setGraphicsContext(gc);
    viewer.getCamera()->setViewport(0, 0, traits->width, traits->height);

    // Disable automatic computation of near and far plane
    viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.setCameraManipulator(cameraManipulator);
	osg::ref_ptr<osg::ApplicationUsage> usage = new osg::ApplicationUsage;
    // Things to do when viewer is realized
    osg::ref_ptr<OpenVRRealizeOperation> openvrRealizeOperation = new OpenVRRealizeOperation(openvrDevice);
    viewer.setRealizeOperation(openvrRealizeOperation.get());

    osg::ref_ptr<OpenVRViewer> openvrViewer = new OpenVRViewer(&viewer, openvrDevice, openvrRealizeOperation);

    openvrViewer->addChild(loadedModel);
    viewer.setSceneData(openvrViewer);
    // Add statistics handler
    viewer.addEventHandler(new osgViewer::StatsHandler);

    viewer.addEventHandler(new OpenVREventHandler(openvrDevice));

    viewer.run();

    // Need to do this here to make it happen before destruction of the OSG Viewer, which destroys the OpenGL context.
    openvrDevice->shutdown(gc.get());

    return 0;
}
