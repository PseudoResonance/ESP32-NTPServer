#pragma once
#include <MicroTime.h>
#include <TinyGPS++.h>

class GPSManager {
   public:
	GPSManager(Stream& serial, int ppsPin);
	~GPSManager();

	void loop();
	boolean validFix();
	uint32_t lastFix();

   private:
	TinyGPSPlus* _gps;
	Stream* _serial;
	int _ppsPin;
	uint32_t _lastUpdate;
};

void IRAM_ATTR ppsInterrupt();