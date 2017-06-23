/*
 * openvreventhandler.cpp
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 */

#include "openvreventhandler.h"
#include "openvrdevice.h"

bool OpenVREventHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& ad)
{
    switch (ea.getEventType())
    {
        case osgGA::GUIEventAdapter::KEYUP:
        {
            switch (ea.getKey())
            {
                case osgGA::GUIEventAdapter::KEY_R:
                    m_openvrDevice->resetSensorOrientation();
                    break;
            }
        }
		case osgGA::GUIEventAdapter::DRAG:
		{
			printf("mouse position:%d ,%d\n", ea.getX(),ea.getY());
		}
    }

    return osgGA::GUIEventHandler::handle(ea, ad);
}
