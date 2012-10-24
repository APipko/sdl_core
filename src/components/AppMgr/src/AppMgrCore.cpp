/*
 * AppMgr.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: vsalo
 */

#include "AppMgr/AppMgrCore.h"
#include "JSONHandler/MobileRPCMessage.h"
#include "JSONHandler/MobileRPCRequest.h"
#include "JSONHandler/MobileRPCResponse.h"
#include "JSONHandler/MobileRPCNotification.h"
#include "JSONHandler/MobileRPCFactory.h"
#include "JSONHandler/RegisterAppInterface.h"
#include "JSONHandler/RegisterAppInterfaceResponse.h"
#include "JSONHandler/SubscribeButton.h"
#include "JSONHandler/SubscribeButtonResponse.h"
#include "JSONHandler/JSONHandler.h"
#include "JSONHandler/HMILevel.h"
#include "AppMgr/IApplication.h"
#include "AppMgr/Application.h"
#include "AppMgr/AppMgrRegistry.h"
#include "AppMgr/AppPolicy.h"
#include "AppMgr/RPCAppLinkObject.h"
#include "AppMgr/RPCBusObject.h"
#include "AppMgr/AppPolicy.h"
#include "AppMgr/RegistryItem.h"
#include "AppMgr/AppLinkInterface.h"
#include <sys/socket.h>
#include "AppMgr/AppMgrRegistry.h"
#include "LoggerHelper.hpp"
#include "JSONHandler/OnButtonEvent.h"
#include "JSONHandler/RPC2Marshaller.h"
#include "JSONHandler/RPC2Command.h"
#include "JSONHandler/RPC2Request.h"
#include "JSONHandler/RPC2Response.h"
#include "JSONHandler/RPC2Notification.h"
#include "JSONHandler/ButtonCapabilities.h"
#include "JSONHandler/GetCapabilitiesResponse.h"

namespace NsAppManager
{

log4cplus::Logger AppMgrCore::mLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("AppMgrCore"));
	
AppMgrCore& AppMgrCore::getInstance( )
{
	static AppMgrCore appMgr;
	return appMgr;
}

AppMgrCore::AppMgrCore()
	:mThreadRPCAppLinkObjectsIncoming(new System::ThreadArgImpl<AppMgrCore>(*this, &AppMgrCore::handleQueueRPCAppLinkObjectsIncoming, NULL))
	,mThreadRPCAppLinkObjectsOutgoing(new System::ThreadArgImpl<AppMgrCore>(*this, &AppMgrCore::handleQueueRPCAppLinkObjectsOutgoing, NULL))
	,mThreadRPCBusObjectsIncoming(new System::ThreadArgImpl<AppMgrCore>(*this, &AppMgrCore::handleQueueRPCBusObjectsIncoming, NULL))
	,mThreadRPCBusObjectsOutgoing(new System::ThreadArgImpl<AppMgrCore>(*this, &AppMgrCore::handleQueueRPCBusObjectsOutgoing, NULL))
	,m_bTerminate(false)
{
	LOG4CPLUS_INFO_EXT(mLogger, " AppMgrCore constructed!");
}

AppMgrCore::~AppMgrCore()
{
	if(!mThreadRPCAppLinkObjectsIncoming.Join())
		mThreadRPCAppLinkObjectsIncoming.Stop();
	
	if(!mThreadRPCAppLinkObjectsOutgoing.Join())
		mThreadRPCAppLinkObjectsOutgoing.Stop();
	
	if(!mThreadRPCBusObjectsIncoming.Join())
		mThreadRPCBusObjectsIncoming.Stop();
	
	if(!mThreadRPCBusObjectsOutgoing.Join())
		mThreadRPCBusObjectsOutgoing.Stop();

	LOG4CPLUS_INFO_EXT(mLogger, " AppMgrCore destructed!");
}

void AppMgrCore::pushMobileRPCMessage( MobileRPCMessage * message )
{
	LOG4CPLUS_INFO_EXT(mLogger, " Pushing mobile RPC message...");
	mMtxRPCAppLinkObjectsIncoming.Lock();
	
	mQueueRPCAppLinkObjectsIncoming.push((RPCAppLinkObject *)message);
	
	mMtxRPCAppLinkObjectsIncoming.Unlock();
	LOG4CPLUS_INFO_EXT(mLogger, " Pushed mobile RPC message");
}

void AppMgrCore::pushRPC2CommunicationMessage( RPC2Communication::RPC2Command * message )
{
	LOG4CPLUS_INFO_EXT(mLogger, " Returning a message from HMI...");
	mMtxRPCBusObjectsIncoming.Lock();
	
	mQueueRPCBusObjectsIncoming.push(message);
	
	mMtxRPCBusObjectsIncoming.Unlock();
	LOG4CPLUS_INFO_EXT(mLogger, " Returned a message from HMI");
}

void AppMgrCore::executeThreads()
{
	LOG4CPLUS_INFO_EXT(mLogger, " Threads are being started!");
	mThreadRPCAppLinkObjectsIncoming.Start(false);
	mThreadRPCAppLinkObjectsOutgoing.Start(false);
	mThreadRPCBusObjectsIncoming.Start(false);
	mThreadRPCBusObjectsOutgoing.Start(false);

	LOG4CPLUS_INFO_EXT(mLogger, " Threads have been started!");
	
/*	while(!m_bTerminate)
	{
	}*/

//	LOG4CPLUS_INFO_EXT(mLogger, " Threads are being stopped!");
}

void AppMgrCore::terminateThreads()
{
	m_bTerminate = true;
}

void AppMgrCore::handleMobileRPCMessage( MobileRPCMessage* msg )
{
	LOG4CPLUS_INFO_EXT(mLogger, " A mobile RPC message "<< msg->getFunctionName() <<" has been received!");
	MobileRPCFactory factory;
	switch(msg->getMessageType())
	{
		case MobileRPCMessage::REQUEST:
		{
			if(0 == msg->getFunctionName().compare("RegisterAppInterface"))
			{
				LOG4CPLUS_INFO_EXT(mLogger, " A RegisterAppInterface request has been invoked");
				RegisterAppInterface * object = (RegisterAppInterface*)msg;
				const RegistryItem* registeredApp =  registerApplication( object );
				MobileRPCMessage* response = queryInfoForRegistration( registeredApp );
				RegisterAppInterfaceResponse* msg = factory.createRegisterAppInterfaceResponse(*msg);
				
				sendMobileRPCResponse( msg );
			}
			else if(0 == msg->getFunctionName().compare("SubscribeButton"))
			{
				LOG4CPLUS_INFO_EXT(mLogger, " A SubscribeButton request has been invoked");
                SubscribeButton * object = (SubscribeButton*)msg;
			//	registerApplication( object );
				sendMobileRPCResponse( msg );
			}
			break;
		}
		case MobileRPCMessage::RESPONSE:
		case MobileRPCMessage::NOTIFICATION:
		{
			LOG4CPLUS_INFO_EXT(mLogger, " A "<< msg->getFunctionName() << " response or notification has been invoked");
			mJSONHandler->sendRPCMessage(msg);
			break;
		}
		case MobileRPCMessage::UNDEFINED:
		default:
			LOG4CPLUS_ERROR_EXT(mLogger, " An undefined RPC message "<< msg->getFunctionName() <<" has been received!");
			break;
	}
}

void AppMgrCore::handleBusRPCMessageIncoming( RPC2Communication::RPC2Command* msg )
{
	LOG4CPLUS_INFO_EXT(mLogger, " A RPC2 bus message "<< msg->getMethod() <<" has been invoking...");

	switch(msg->getCommandType())
	{
		case MobileRPCMessage::REQUEST:
		{
			if(RPC2Communication::RPC2Marshaller::METHOD_GET_CAPABILITIES_REQUEST == msg->getMethod())
			{
				LOG4CPLUS_INFO_EXT(mLogger, " A GetCapabilitiesResponse request has been invoked");
				RPC2Communication::GetCapabilitiesResponse * object = (RPC2Communication::GetCapabilitiesResponse*)msg;
				setButtonCapabilities( object );
			}
	/*		else if(RPC2Communication::RPC2Marshaller:: == msg->getMethod().compare("SubscribeButton"))
			{
				LOG4CPLUS_INFO_EXT(mLogger, " A SubscribeButton request has been invoked");
				//	SubscribeButton * object = (SubscribeButton*)msg;
				//	registerApplication( object );
				sendMobileRPCResponse( msg );
			} */
			break;
		}
		case MobileRPCMessage::RESPONSE:
		case MobileRPCMessage::NOTIFICATION:
		{
			LOG4CPLUS_INFO_EXT(mLogger, " A "<< msg->getMethod() << " response or notification has been invoked");
		//	mJSONHandler->sendRPCMessage(msg);
			break;
		}
		case MobileRPCMessage::UNDEFINED:
		default:
			LOG4CPLUS_ERROR_EXT(mLogger, " An undefined RPC message "<< msg->getMethod() <<" has been received!");
			break;
	}
//	AppLinkInterface::getInstance().sendRPCCommand( msg );
	LOG4CPLUS_INFO_EXT(mLogger, " A RPC2 bus message "<< msg->getMethod() <<" has been invoked!");
}

void AppMgrCore::enqueueOutgoingMobileRPCMessage( MobileRPCMessage * message )
{
	LOG4CPLUS_INFO_EXT(mLogger, " A " << message->getFunctionName() << " outgoing mobile RPC message send has been invoked");
	
	mMtxRPCAppLinkObjectsOutgoing.Lock();
	
	mQueueRPCAppLinkObjectsOutgoing.push((RPCAppLinkObject *)message);
	
	mMtxRPCAppLinkObjectsOutgoing.Unlock();
	
	LOG4CPLUS_INFO_EXT(mLogger, " A " << message->getFunctionName() << " outgoing mobile RPC message has been sent");
}

void AppMgrCore::enqueueOutgoingBusRPCMessage( RPC2Communication::RPC2Command * message )
{
	LOG4CPLUS_INFO_EXT(mLogger, " A " << message->getMethod() << " outgoing mobile RPC message send has been invoked");
	
	mMtxRPCBusObjectsOutgoing.Lock();
	
	mQueueRPCBusObjectsOutgoing.push(message);
	
	mMtxRPCBusObjectsOutgoing.Unlock();

	LOG4CPLUS_INFO_EXT(mLogger, " A " << message->getMethod() << " outgoing mobile RPC message has been sent");
}

const RegistryItem* AppMgrCore::registerApplication( RegisterAppInterface* object )
{
	LOG4CPLUS_INFO_EXT(mLogger, " Registering an application " << object->getAppName() << "!");
	const std::string& appName = object->getAppName();
	const std::string& ngnMediaScreenAppName = object->getNgnMediaScreenAppName();
	const std::vector<std::string>& vrSynonyms = object->getVrSynonyms();
	bool usesVehicleData = object->getUsesVehicleData();
	bool isMediaApplication = object->getIsMediaApplication();
	const Language& languageDesired = object->getLanguageDesired();
	const std::string& autoActivateID = object->getAutoActivateID();
	const SyncMsgVersion& syncMsgVersion = object->getSyncMsgVersion();

	Application* application = new Application( appName );
	application->setAutoActivateID(autoActivateID);
	application->setIsMediaApplication(isMediaApplication);
	application->setLanguageDesired(languageDesired);
	application->setNgnMediaScreenAppName(ngnMediaScreenAppName);
	application->setSyncMsgVersion(syncMsgVersion);
	application->setUsesVehicleData(usesVehicleData);
	application->setVrSynonyms(vrSynonyms);

	application->setApplicationHMIStatusLevel(HMILevel::NONE);

//    RPC2Communication::

	return AppMgrRegistry::getInstance().registerApplication( application );
}

MobileRPCMessage* AppMgrCore::queryInfoForRegistration( const RegistryItem* registryItem )
{
	LOG4CPLUS_INFO_EXT(mLogger, " Querying info for registration of an application " << registryItem->getApplication()->getName() << "!");

	

	LOG4CPLUS_INFO_EXT(mLogger, " Queried info for registration of an application " << registryItem->getApplication()->getName() << "!");
}

void AppMgrCore::registerApplicationOnHMI( const std::string& name )
{
//	RPCBusObject* object = new RPCBusObject( 1, RPCBusObject::REQUEST, "AppLinkCore.OnAppRegister" );
//	object->setParameter("name", name);
}

void AppMgrCore::setButtonCapabilities( RPC2Communication::GetCapabilitiesResponse* msg )
{
	mButtonCapabilities = msg->getCapabilities();
}

Capabilities AppMgrCore::getButtonCapabilities() const
{
	return mButtonCapabilities;
}

void AppMgrCore::sendMobileRPCResponse( MobileRPCMessage* msg )
{
	LOG4CPLUS_INFO_EXT(mLogger, " Sending mobile RPC response to "<< msg->getFunctionName() <<"!");
	MobileRPCFactory factory;
	RegisterAppInterfaceResponse* response = factory.createRegisterAppInterfaceResponse( *msg );
	enqueueOutgoingMobileRPCMessage( response );
}

void* AppMgrCore::handleQueueRPCBusObjectsIncoming( void* )
{
	while(true)
	{
		std::size_t size = mQueueRPCBusObjectsIncoming.size();
		if( size > 0 )
		{
			mMtxRPCBusObjectsIncoming.Lock();
			RPC2Communication::RPC2Command* msg = mQueueRPCBusObjectsIncoming.front();
			mQueueRPCBusObjectsIncoming.pop();
			mMtxRPCBusObjectsIncoming.Unlock();
			if(!msg)
			{
				LOG4CPLUS_ERROR_EXT(mLogger, " Erroneous null-message has been received!");
				continue;
			}
			
			handleBusRPCMessageIncoming( msg );
		}
	}
}

void* AppMgrCore::handleQueueRPCAppLinkObjectsOutgoing( void* )
{
	while(true)
	{
		std::size_t size = mQueueRPCAppLinkObjectsOutgoing.size();
		if( size > 0 )
		{
			mMtxRPCAppLinkObjectsOutgoing.Lock();
			MobileRPCMessage* msg = mQueueRPCAppLinkObjectsOutgoing.front();
			mQueueRPCAppLinkObjectsOutgoing.pop();
			mMtxRPCAppLinkObjectsOutgoing.Unlock();
			if(!msg)
			{
				LOG4CPLUS_ERROR_EXT(mLogger, " Erroneous null-message has been received!");
				continue;
			}
			
			handleMobileRPCMessage( msg );
		}
	}
}

void* AppMgrCore::handleQueueRPCAppLinkObjectsIncoming( void* )
{
	while(true)
	{
		std::size_t size = mQueueRPCAppLinkObjectsIncoming.size();
		if( size > 0 )
		{
			mMtxRPCAppLinkObjectsIncoming.Lock();
			MobileRPCMessage* msg = mQueueRPCAppLinkObjectsIncoming.front();
			mQueueRPCAppLinkObjectsIncoming.pop();
			mMtxRPCAppLinkObjectsIncoming.Unlock();
			if(!msg)
			{
				LOG4CPLUS_ERROR_EXT(mLogger, " Erroneous null-message has been received!");
				continue;
			}
			
			handleMobileRPCMessage( msg );
		}
	}
}

void* AppMgrCore::handleQueueRPCBusObjectsOutgoing( void* )
{
	while(true)
	{
		std::size_t size = mQueueRPCBusObjectsOutgoing.size();
		if( size > 0 )
		{
			mMtxRPCBusObjectsOutgoing.Lock();
			RPC2Communication::RPC2Command* msg = mQueueRPCBusObjectsOutgoing.front();
			mQueueRPCBusObjectsOutgoing.pop();
			mMtxRPCBusObjectsOutgoing.Unlock();
			if(!msg)
			{
				LOG4CPLUS_ERROR_EXT(mLogger, " Erroneous null-message has been received!");
				continue;
			}
			
			handleBusRPCMessageIncoming( msg );
		}
	}
}

void AppMgrCore::setJsonHandler(JSONHandler* handler)
{
	mJSONHandler = handler;
}

JSONHandler* AppMgrCore::getJsonHandler( ) const
{
	return mJSONHandler;
}

};
