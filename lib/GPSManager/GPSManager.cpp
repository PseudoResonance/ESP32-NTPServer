#include <GPSManager.h>

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

GPSManager::GPSManager(Stream& serial, int ppsPin) {
	_serial = &serial;
	_ppsPin = ppsPin;
	_lastUpdate = 0;
	_gps = new TinyGPSPlus();
	pinMode(_ppsPin, INPUT_PULLDOWN);
	attachInterrupt(_ppsPin, ppsInterrupt, RISING);
}

GPSManager::~GPSManager() {
	detachInterrupt(_ppsPin);
}

void GPSManager::loop() {
	while (_serial->available()) {
		if (_gps->encode(_serial->read()) && _gps->time.isUpdated() && _gps->date.isUpdated() && lastFix() < 100 && millis() - _lastUpdate > 900) {
			TinyGPSTime time = _gps->time;
			TinyGPSDate date = _gps->date;
			setTime(time.hour(), time.minute(), time.second(), date.day(), date.month(), date.year());
			_lastUpdate = millis();
		}
	}
}

boolean GPSManager::validFix() {
	return _gps->time.isValid() && _gps->date.isValid();
}

uint32_t GPSManager::lastFix() {
	return max(_gps->time.age(), _gps->date.age());
}

void IRAM_ATTR ppsInterrupt() {
	portENTER_CRITICAL_ISR(&mux);
	syncToPPS();
	portEXIT_CRITICAL_ISR(&mux);
}