//////////////////////////////////////////////////////////////////////////
// NSClient++ Base Service
// 
// Copyright (c) 2004 MySolutions NORDIC (http://www.medin.name)
//
// Date: 2004-03-13
// Author: Michael Medin (michael@medin.name)
//
// Part of this file is based on work by Bruno Vais (bvais@usa.net)
//
// This software is provided "AS IS", without a warranty of any kind.
// You are free to use/modify this code but leave this header intact.
//
//////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <winsvc.h>
#include "NSClient++.h"
#include "Settings.h"
#include <charEx.h>

NSClient mainClient;	// Global core instance.

//////////////////////////////////////////////////////////////////////////
// Startup code

/**
 * Application startup point
 *
 * @param argc Argument count
 * @param argv[] Argument array
 * @param envp[] Environment array
 * @return exit status
 */
int main(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	if ( (argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/')) ) {
		if ( _stricmp( "install", argv[1]+1 ) == 0 ) {
			try {
				serviceControll::Install(SZSERVICENAME, SZSERVICEDISPLAYNAME, SZDEPENDENCIES);
			} catch (const serviceControll::SCException& e) {
				LOG_MESSAGE_STD("Service installation failed: " + e.error_);
				return -1;
			}
			LOG_MESSAGE("Service installed!");
		} else if ( _stricmp( "uninstall", argv[1]+1 ) == 0 ) {
			try {
				serviceControll::Uninstall(SZSERVICENAME);
			} catch (const serviceControll::SCException& e) {
				LOG_MESSAGE_STD("Service deinstallation failed; " + e.error_);
				return -1;
			}
		} else if ( _stricmp( "start", argv[1]+1 ) == 0 ) {
			serviceControll::Start(SZSERVICENAME);
		} else if ( _stricmp( "stop", argv[1]+1 ) == 0 ) {
			serviceControll::Stop(SZSERVICENAME);
		} else if ( _stricmp( "about", argv[1]+1 ) == 0 ) {
			LOG_MESSAGE(SZAPPNAME " (C) Michael Medin");
			LOG_MESSAGE("Version " SZVERSION);
		} else if ( _stricmp( "version", argv[1]+1 ) == 0 ) {
			LOG_MESSAGE(SZAPPNAME " Version: " SZVERSION);
		} else if ( _stricmp( "test", argv[1]+1 ) == 0 ) {
			mainClient.InitiateService();
			LOG_MESSAGE("Enter command to inject or exit to terminate...");
			std::string s = "";
			while (s != "exit") {
				std::cin >> s;
				mainClient.inject(s);
			}
			mainClient.TerminateService();
			return 0;
		} else {
			LOG_MESSAGE("Usage: -version, -about, -install, -uninstall, -start, -stop");
		}
		return nRetCode;
	}
	mainClient.StartServiceCtrlDispatcher();
	return nRetCode;
}

//////////////////////////////////////////////////////////////////////////
// Service functions

/**
 * Service control handler startup point.
 * When the program is started as a service this will be the entry point.
 */
void NSClientT::InitiateService(void) {
	char* buffer = new char[1024];
	GetModuleFileName(NULL, buffer, 1023);
	std::string path = buffer;
	std::string::size_type pos = path.rfind('\\');
	basePath = path.substr(0, pos) + "\\";
	LOG_DEBUG_STD("Base path is: " + basePath);
	delete [] buffer;
	Settings::getInstance()->setFile(basePath + "NSC.ini");

	SettingsT::sectionList list = Settings::getInstance()->getSection("modules");
	for (SettingsT::sectionList::iterator it = list.begin(); it != list.end(); it++) {
		try {
			LOG_DEBUG_STD("Loading: " + basePath + "modules\\" + (*it));
			loadPlugin(basePath + "modules\\" + (*it));
		} catch(const NSPluginException& e) {
			LOG_ERROR_STD("Exception raised: " + e.error_ + " in module: " + e.file_);
		}
	}
	socketThread.createThread();
}
/**
 * Service control handler termination point.
 * When the program is stopped as a service this will be the "exit point".
 */
void NSClientT::TerminateService(void) {
	if (!socketThread.exitThread())
		LOG_ERROR("Could not exit socket listener thread");
	try {
		mainClient.unloadPlugins();
	} catch(NSPluginException *e) {
		LOG_ERROR_STD("Exception raised: " + e->error_ + " in module: " + e->file_);
	}
	Settings::destroyInstance();
}

/**
 * Forward this to the main service dispatcher helper class
 * @param dwArgc 
 * @param *lpszArgv 
 */
void WINAPI NSClientT::service_main_dispatch(DWORD dwArgc, LPTSTR *lpszArgv) {
	mainClient.service_main(dwArgc, lpszArgv);
}
/**
 * Forward this to the main service dispatcher helper class
 * @param dwCtrlCode 
 */
void WINAPI NSClientT::service_ctrl_dispatch(DWORD dwCtrlCode) {
	mainClient.service_ctrl(dwCtrlCode);
}

//////////////////////////////////////////////////////////////////////////
// Member functions


/**
 * Load a list of plug-ins
 * @param plugins A list with plug-ins (DLL files) to load
 */
void NSClientT::loadPlugins(std::list<std::string> plugins) {
	std::list<std::string>::const_iterator it;
	for (it = plugins.begin(); it != plugins.end(); ++it) {
		loadPlugin(*it);
	}
}
/**
 * Unload all plug-ins (in reversed order)
 */
void NSClientT::unloadPlugins() {
	messageHandlers_.clear();
	commandHandlers_.clear();
	pluginList::reverse_iterator it;
	for (it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
		LOG_DEBUG_STD("---\n\n\nUnloading: " + (*it)->getName());
		(*it)->unload();
		delete (*it);
	}
	plugins_.clear();
}
/**
 * Load a single plug-in using a DLL filename
 * @param file The DLL file
 */
void NSClientT::loadPlugin(std::string file) {
	addPlugin(NSCPlugin::loadPlugin(file));
}
/**
 * Load and add a plugin to various internal structures
 * @param *plugin The plug-ininstance to load. The pointer is managed by the 
 */
void NSClientT::addPlugin(NSCPlugin *plugin) {
	plugin->load();
	LOG_DEBUG_STD("Loading: " + plugin->getName());
	// @todo Catch here and unload if we fail perhaps ?
	plugins_.push_back(plugin);
	if (plugin->hasCommandHandler())
		commandHandlers_.push_back(plugin);
	if (plugin->hasMessageHandler())
		messageHandlers_.push_back(plugin);
}
/**
 * Inject a command into the plug-in stack.
 *
 * @param buffer A command string to inject. This should be a unparsed command string such as command&arg1&arg2&arg...
 * @return The result, empty string if no result
 */
std::string NSClientT::inject(std::string buffer) {
	std::list<std::string> args = charEx::split(buffer.c_str(), '&');
	if (args.size() < 2) {
		LOG_MESSAGE("Insufficient arguments!");
	}
	std::string command = args.front(); args.pop_front();
	LOG_MESSAGE_STD("Injecting: " + command);
	std::string ret = execute(NSClientT::getPassword(), command, args);
	LOG_MESSAGE_STD("Injected Result: " + ret);
	return ret;

}
/**
 * Helper function to return the current password (perhaps this should be static ?)
 *
 * @return The current password
 */
std::string NSClientT::getPassword() {
	return Settings::getInstance()->getString("generic", "password", "");
}
/**
 * Execute a command.
 *
 * @param password The password
 * @param cmd The command
 * @param args Arguments as a list<string>
 * @return The result if any, empty string if no result.
 *
 * @todo Make an int return value to set critical/warning/ok/unknown status.
 * ie. pair<int,string>
 */
std::string NSClientT::execute(std::string password, std::string cmd, std::list<std::string> args) {
	static unsigned int bufferSize = 0;
	if (bufferSize == 0)
		bufferSize = static_cast<unsigned int>(Settings::getInstance()->getInt("main", "bufferSize", 4096));

	if (password != getPassword())
		return "INVALID PASSWORD";

	// Pack the argument as a char** buffer
	std::string ret;
	int len = args.size();
	char **arguments = new char*[len];
	std::list<std::string>::iterator it = args.begin();
	for (int i=0;it!=args.end();++it,i++) {
		int alen = (*it).size();
		arguments[i] = new char[alen+2];
		strncpy(arguments[i], (*it).c_str(), alen+1);
	}
	// Allocate return buffer
	char* returnbuffer = new char[bufferSize+1];
	pluginList::const_iterator plit;
	for (plit = commandHandlers_.begin(); plit != commandHandlers_.end(); ++plit) {
		try {
			int c = (*plit)->handleCommand(cmd.c_str(), len, arguments, returnbuffer, bufferSize);
			if (c == NSCAPI::handled) {					// module handled the message "we are done..."
				ret = returnbuffer;
				break;
			} else if (c == NSCAPI::isfalse) {			// Module ignored the message
			} else if (c == NSCAPI::invalidBufferLen) {	// Buffer is to small
				LOG_ERROR("Return buffer to small, need to increase it in the ini file.");
			} else {									// Something else went wrong...
				LOG_ERROR("Unknown error from handleCommand");
			}
		} catch(const NSPluginException& e) {
			LOG_ERROR_STD("Exception raised: " + e.error_ + " in module: " + e.file_);
		}
	}
	// delete buffers
	delete [] returnbuffer;
	for (int i=0;i<len;i++) {
		delete [] arguments[i];
	}
	delete [] arguments;
	return ret;
}
/**
 * Report a message to all logging enabled modules.
 *
 * @param msgType Message type 
 * @param file Filename generally __FILE__
 * @param line  Line number, generaly __LINE__
 * @param message The message as a human readable string.
 */
void NSClientT::reportMessage(int msgType, const char* file, const int line, std::string message) {
	MutexLock lock(messageMutex);
	pluginList::const_iterator plit;
	for (plit = messageHandlers_.begin(); plit != messageHandlers_.end(); ++plit) {
		try {
			(*plit)->handleMessage(msgType, file, line, message.c_str());
		} catch(const NSPluginException& e) {
			// Here we are pretty much fucked! (as logging this might cause a loop :)
			throw "This shouldn't have happened...";
		}
	}
}

int NSAPIGetSettingsString(const char* section, const char* key, const char* defaultValue, char* buffer, unsigned int bufLen) {
	return NSCHelper::wrapReturnString(buffer, bufLen, Settings::getInstance()->getString(section, key, defaultValue));
}
int NSAPIGetSettingsInt(const char* section, const char* key, int defaultValue) {
	return Settings::getInstance()->getInt(section, key, defaultValue);
}

int NSAPIGetApplicationName(char*buffer, unsigned int bufLen) {
	return NSCHelper::wrapReturnString(buffer, bufLen, SZAPPNAME);
}
int NSAPIGetApplicationVersionStr(char*buffer, unsigned int bufLen) {
	return NSCHelper::wrapReturnString(buffer, bufLen, SZVERSION);
}
void NSAPIMessage(int msgType, const char* file, const int line, const char* message) {
	mainClient.reportMessage(msgType, file, line, message);
}
void NSAPIStopServer(void) {
	serviceControll::Stop(SZSERVICENAME);
}
int NSAPIInject(const char* command, char* buffer, unsigned int bufLen) {
	return NSCHelper::wrapReturnString(buffer, bufLen, mainClient.inject(command));
}

LPVOID NSAPILoader(char*buffer) {
	if (stricmp(buffer, "NSAPIGetApplicationName") == 0)
		return &NSAPIGetApplicationName;
	if (stricmp(buffer, "NSAPIGetApplicationVersionStr") == 0)
		return &NSAPIGetApplicationVersionStr;
	if (stricmp(buffer, "NSAPIGetSettingsString") == 0)
		return &NSAPIGetSettingsString;
	if (stricmp(buffer, "NSAPIGetSettingsInt") == 0)
		return &NSAPIGetSettingsInt;
	if (stricmp(buffer, "NSAPIMessage") == 0)
		return &NSAPIMessage;
	if (stricmp(buffer, "NSAPIStopServer") == 0)
		return &NSAPIStopServer;
	if (stricmp(buffer, "NSAPIInject") == 0)
		return &NSAPIInject;
	return NULL;
}