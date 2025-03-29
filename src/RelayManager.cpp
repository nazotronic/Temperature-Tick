/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

RelayManager::RelayManager() {
	makeDefault();
}


void RelayManager::makeDefault() {
	/* --- classes & structures --- */
	system = NULL;

	/* --- settings --- */
	invert_flag = DEFAULT_RELAY_INVERT_FLAG;
	mode = DEFAULT_RELAY_MODE;

	therm_sensor_index = DEFAULT_RELAY_THERM_SENSOR_INDEX;
	therm_set_t = DEFAULT_RELAY_THERM_T;
	therm_delta = DEFAULT_RELAY_THERM_DELTA;
	therm_mode = DEFAULT_RELAY_THERM_MODE;
	therm_error_relay_flag = DEFAULT_RELAY_THERM_ERROR_RELE_FLAG;

	/* --- variables --- */
	observers.clear();
	relay_flag = false;
}

void RelayManager::begin() {
	pinMode(RELAY_PORT, OUTPUT);
	setRelayFlag(false);
}

void RelayManager::tick() {
	SensorsManager* sensors = system->getSensorsManager();
	relayTick();

	if (getMode() == RELAY_MODE_SIMPLE) {

	}

	else if (getMode() == RELAY_MODE_THERM) {
		if (!getThermStatus()) {
			float t = sensors->getDS18B20T(getThermSensor());

			if (getThermMode() == RELAY_THERM_MODE_HEATING) {
				if (t >= getThermSetT()) {
					setRelayFlag(false);
				}

				else if (t <= getThermSetT() - getThermDelta()) {
					setRelayFlag(true);
				}
			}

			else if (getThermMode() == RELAY_THERM_MODE_COOLING) {
				if (t >= getThermSetT() + getThermDelta()) {
					setRelayFlag(true);
				}

				else if (t <= getThermSetT()) {
					setRelayFlag(false);
				}
			}
		}

		else {
			setRelayFlag(getThermErrorRelayFlag());
		}
	}
}

void RelayManager::addElementCodes(DynamicArray<String>* array) {
	if (array == NULL) {
		return;
	}

	array->add(String("/relay/data/relay_flag"));
	array->add(String("/relay/settings/relay_flag"));
}


void RelayManager::addObserver(IObserver* observer) {
	if (observer == NULL) {
		return;
	}

	observers.add(observer);
}

bool RelayManager::handleEvent(const char* code, void* data, uint8_t type) {
	if (!strcmp(code, "/relay/settings/relay_flag")) {
		setRelayFlag(POINTER_TO_TYPE(data, type));
		return true;
	}

	if (strstr(code, "/relay/data") != NULL) {
		return true;
	}

	return false;
}


void RelayManager::writeSettings(char* buffer) {
	setParameter(buffer, "RSif", getInvertFlag());
	setParameter(buffer, "RSm", getMode());

	setParameter(buffer, "RSTsi", getThermSensor());
	setParameter(buffer, "RSTst", getThermSetT());
	setParameter(buffer, "RSTd", getThermDelta());
	setParameter(buffer, "RSTm", getThermMode());
	setParameter(buffer, "RSTerf", getThermErrorRelayFlag());
}

void RelayManager::readSettings(char* buffer) {
	getParameter(buffer, "RSif", &invert_flag);
	getParameter(buffer, "RSm", &mode);

	getParameter(buffer, "RSTsi", &therm_sensor_index);
	getParameter(buffer, "RSTst", &therm_set_t);
	getParameter(buffer, "RSTd", &therm_delta);
	getParameter(buffer, "RSTm", &therm_mode);
	getParameter(buffer, "RSTerf", &therm_error_relay_flag);

	setInvertFlag(invert_flag);
	setMode(mode);

	setThermSensor(therm_sensor_index);
	setThermSetT(therm_set_t);
	setThermDelta(therm_delta);
	setThermMode(therm_mode);
	setThermErrorRelayFlag(therm_error_relay_flag);
}

void RelayManager::setSystemManager(SystemManager* system) {
	this->system = system;
}

void RelayManager::setRelayFlag(bool relay_flag, bool sync_flag) {
	if (getRelayFlag() != relay_flag || sync_flag) {Serial.print("set ");Serial.println(relay_flag);
		this->relay_flag = relay_flag;

		relayTick();
		notifyObservers(String("/relay/data/relay_flag"), &relay_flag, TYPE_BOOL);
	}
}


void RelayManager::setInvertFlag(bool invert_flag) {
	this->invert_flag = invert_flag;
	relayTick();
}

void RelayManager::setMode(uint8_t mode) {
	this->mode = mode;
}


void RelayManager::setThermSensor(int8_t ds18b20_index) {
	SensorsManager* sensors = system->getSensorsManager();
	this->therm_sensor_index = constrain(ds18b20_index, -1, sensors->getDS18B20Count() - 1);
}

void RelayManager::setThermSetT(float t) {
	this->therm_set_t = t;
}

void RelayManager::setThermDelta(float delta) {
	this->therm_delta = delta;
}

void RelayManager::setThermMode(uint8_t mode) {
	this->therm_mode = mode;
}

void RelayManager::setThermErrorRelayFlag(bool relay_flag) {
	this->therm_error_relay_flag = relay_flag;
}


bool RelayManager::getRelayFlag() {
	return relay_flag;
}


bool RelayManager::getInvertFlag() {
	return invert_flag;
}

uint8_t RelayManager::getMode() {
	return mode;
}


uint8_t RelayManager::getThermStatus() {
	SensorsManager* sensors = system->getSensorsManager();

	if (getThermSensor() >= sensors->getDS18B20Count() || getThermSensor() < 0) return 1;
	if (sensors->getDS18B20Status(getThermSensor()) ) return 2;

	return 0;
}

float RelayManager::getThermT() {
	SensorsManager* sensors = system->getSensorsManager();
	return sensors->getDS18B20T(getThermSensor());
}

int8_t RelayManager::getThermSensor() {
	return therm_sensor_index;
}

float RelayManager::getThermSetT() {
	return therm_set_t;
}

float RelayManager::getThermDelta() {
	return therm_delta;
}

uint8_t RelayManager::getThermMode() {
	return therm_mode;
}

bool RelayManager::getThermErrorRelayFlag() {
	return therm_error_relay_flag;
}


void RelayManager::notifyObservers(String code, void* data, uint8_t type) {
	for (uint8_t i = 0;i < observers.size();i++) {
		observers[i]->handleEvent(code.c_str(), data, type);
	}
}

void RelayManager::relayTick() {//Serial.println(getRelayFlag());
	digitalWrite(RELAY_PORT, getInvertFlag() ? !getRelayFlag() : getRelayFlag());
}