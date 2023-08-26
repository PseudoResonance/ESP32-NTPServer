#pragma once
#include <ETHClass.h>
#include <AsyncUDP.h>
#include <GPSManager.h>

#define NTP_PORT 123

class NTPServer {
	public:
		NTPServer(Stream& serial, GPSManager& gpsManager);
		~NTPServer();

   private:
		Stream* _serial;
		GPSManager* _gpsManager;
		AsyncUDP* _udp;
};