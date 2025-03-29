/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

SystemManager::SystemManager() {
	makeDefault();
}


void SystemManager::makeDefault() {
	setSleepFlag(DEFAULT_SLEEP_STATUS);
	setSleepTime(DEFAULT_SLEEP_TIME);

	observers.clear();
	sleep_reqs.makeDefault();

	save_settings_request = false;
	save_settings_timer = 0;
	work_timer = 0;
}

void SystemManager::begin() {
	Serial.begin(9600);
	LittleFS.begin();

	/* SystemManager */
	addObserver(&mqtt);
	/* SystemManager */

	/* SensorsManager */
	sensors.setSystemManager(this);
	sensors.addObserver(&mqtt);
	sensors.addObserver(&blynk);
	/* SensorsManager */

	/* RelayManager */
	relay.setSystemManager(this);
	relay.addObserver(&mqtt);
	relay.addObserver(&blynk);
	/* RelayManager */

	/* NetworkManager */
	network.setSystemManager(this);
	/* NetworkManager */

	/* BlynkManager */
	blynk.setSystemManager(this);
	blynk.addObserver(this);
	blynk.addObserver(&relay);
	/* BlynkManager */

	/* MqttManager */
	mqtt.setSystemManager(this);
	mqtt.addObserver(this);
	mqtt.addObserver(&sensors);
	mqtt.addObserver(&relay);
	/* MqttManager */

	readSettings();
	
	pinMode(BUTTON_PORT, INPUT_PULLUP);
	
	if (getButtonStatus()) {
		setSleepFlag(false);
	}

	sensors.begin();
	relay.begin();
	network.begin();
	mqtt.begin();
	blynk.begin();

	network.endBegin();
}

void SystemManager::tick() {
	// uint32_t tick = millis();
	if (getSleepFlag()) {
		if (!work_timer) {
			work_timer = millis();
		}

		else if (millis() - work_timer > SEC_TO_MLS(WORK_TIME)) {
			Serial.println("timeout sleep");
			ESP.deepSleep(MIN_TO_MLS(getSleepTime()) * 1000);
		}
	}

	sensors.tick();

	if (!getSleepFlag()) {
		relay.tick();
	}

	network.tick();
	mqtt.tick();
	blynk.tick();

	saveSettings();

	if (getSleepFlag()) {
		if (sleep_reqs.isReqsDone()) {
			Serial.println("reqsDone sleep");

			ESP.deepSleep(MIN_TO_MLS(getSleepTime()) * 1000);
		}
	}
	// Serial.println(millis() - tick);
}

void SystemManager::addElementCodes(DynamicArray<String>* array) {
	if (array == NULL) {
		return;
	}

	array->add(String("/system/settings/reset"));
}


void SystemManager::addObserver(IObserver* observer) {
	if (observer == NULL) {
		return;
	}

	observers.add(observer);
}

bool SystemManager::handleEvent(const char* code, void* data, uint8_t type) {
	if (!strcmp(code, "/system/settings/reset")) {
		reset();
		return true;
	}

	return false;
}


void SystemManager::reset() {
	ESP.reset();
}

void SystemManager::resetAll() {
	LittleFS.remove("/config.nztr");

  	ESP.reset();
}

void SystemManager::saveSettingsRequest() {
	save_settings_request = true;
}


void SystemManager::makeElementCodesList(DynamicArray<String>* array) {
	if (array == NULL) {
		return;
	}
	array->clear();

	addElementCodes(array);
	sensors.addElementCodes(array);
	relay.addElementCodes(array);
	network.addElementCodes(array);
	mqtt.addElementCodes(array);
	blynk.addElementCodes(array);
}

int8_t SystemManager::scanElementCodeIndex(DynamicArray<String>* array, String element_code) {
	if (array == NULL) {
		return -1;
	}

	for (uint8_t i = 0; i < array->size();i++) {
		if (!strcmp((*array)[i].c_str(), element_code.c_str()) ) {
			return i;
		}
	}

	return -1;
}

void SystemManager::handleElementRemoval(String element_code) {
	blynk.deleteLink(element_code);
}

void SystemManager::handleElementCodeUpdate(String previous_code, String new_code) {
	blynk.modifyLinkElementCode(previous_code, new_code);
}


void SystemManager::setSleepFlag(bool sleep_flag) {
	this->sleep_flag = sleep_flag;
}

void SystemManager::setSleepTime(uint8_t sleep_time) {
	this->sleep_time = sleep_time;
}


void SystemManager::setSensorsReadFlag(bool flag) {
	sleep_reqs.sensors_read_flag = flag;
}

void SystemManager::setMqttSentFlag(bool flag) {
	if (getSensorsReadFlag()) {
		sleep_reqs.mqtt_sent_flag = flag;
	}
}

void SystemManager::setBlynkSentFlag(bool flag) {
	if (getSensorsReadFlag()) {
		sleep_reqs.blynk_sent_flag = flag;
	}
}


SensorsManager* SystemManager::getSensorsManager() {
	return &sensors;
}

RelayManager* SystemManager::getRelayManager() {
	return &relay;
}

NetworkManager* SystemManager::getNetworkManager() {
	return &network;
}

MqttManager* SystemManager::getMqttManager() {
	return &mqtt;
}

BlynkManager* SystemManager::getBlynkManager() {
	return &blynk;
}


bool SystemManager::getSleepFlag() {
	return sleep_flag;
}

uint8_t SystemManager::getSleepTime() {
	return sleep_time;
}


bool SystemManager::getSensorsReadFlag() {
	return sleep_reqs.sensors_read_flag;
}

bool SystemManager::getMqttSentFlag() {
	return sleep_reqs.mqtt_sent_flag;
}

bool SystemManager::getBlynkSentFlag() {
	return sleep_reqs.blynk_sent_flag;
}


void SystemManager::notifyObservers(String code, void* data, uint8_t type) {
	for (uint8_t i = 0;i < observers.size();i++) {
		observers[i]->handleEvent(code.c_str(), data, type);
	}
}


void SystemManager::saveSettings(bool ignore_flag) {
	if (!ignore_flag) {
		if (!save_settings_request) {
			return;
		}

		if (millis() - save_settings_timer < SEC_TO_MLS(SAVE_SETTINGS_TIME)) {
			return;
		}
	}
	Serial.println("save");

	File file = LittleFS.open("/config.nztr", "w");
	char buffer[SETTINGS_BUFFER_SIZE + 1] = "";

	setParameter(buffer, "SSsf", getSleepFlag());
	setParameter(buffer, "SSst", getSleepTime());

	sensors.writeSettings(buffer);
	relay.writeSettings(buffer);
	network.writeSettings(buffer);
	mqtt.writeSettings(buffer);
	blynk.writeSettings(buffer);

	file.write(buffer, strlen(buffer));
  	file.close();

	save_settings_request = false;
	save_settings_timer = millis();
}

void SystemManager::readSettings() {
	File file = LittleFS.open("/config.nztr", "r");

	if (!file) {
		saveSettings(true);
		return;
	}

	uint16_t file_size = file.size();
	char* buffer = new char[file_size + 1];

	file.read((uint8_t*) buffer, file_size);
	buffer[file_size] = 0;
	
	getParameter(buffer, "SSsf", &sleep_flag);
	getParameter(buffer, "SSst", &sleep_time);

	setSleepFlag(sleep_flag);
	setSleepTime(sleep_time);
	
	sensors.readSettings(buffer);
	relay.readSettings(buffer);
	network.readSettings(buffer);
	mqtt.readSettings(buffer);
	blynk.readSettings(buffer);

	delete[] buffer;
	file.close();
}

bool SystemManager::getButtonStatus() {
	return !digitalRead(BUTTON_PORT);
}