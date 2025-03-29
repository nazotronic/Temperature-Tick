/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

MqttManager::MqttManager() {
	makeDefault();
}


void MqttManager::makeDefault() {
	mqtt_client.setClient(esp_client);
	setSystemManager(NULL);

	setWorkFlag(DEFAULT_MQTT_WORK_STATUS);
	setServer("", 0);
	setAccess("", "");

	observers.clear();
	
	reset_request = true;
	reconnect_timer = 0;
}

void MqttManager::begin() {
	esp_client.setInsecure();

	mqtt_client.setCallback([this](char* topic, byte* payload, unsigned int length) {
		// Serial.print("Topic: ");
		// Serial.print(topic); Serial.print(" "); Serial.println(length);
	
		char* buffer = new char[length + 1];

		memcpy(buffer, payload, length);
		buffer[length] = 0;
	
		// Serial.print("Payload: ");
		// Serial.println(buffer);

		float data = atoff(buffer);
		notifyObservers(topic, &data, TYPE_FLOAT);

		// Serial.print("number: ");
		// Serial.println(data);

		delete[] buffer;
	});

	tick();
}

void MqttManager::tick() {
	NetworkManager* network = system->getNetworkManager();

	if (reset_request) {
		Serial.println("reset mqtt");

		reset_request = false;
		off();
	}

	if (!getWorkFlag() || !*getServer() || network->getStatus() != WL_CONNECTED) {
		return;
	}

	if (getStatus()) {
		connect();
	}

	mqtt_client.loop();
}

void MqttManager::addElementCodes(DynamicArray<String>* array) {
	
}


void MqttManager::addObserver(IObserver* observer) {
	if (observer == NULL) {
		return;
	}

	observers.add(observer);
}

bool MqttManager::handleEvent(const char* code, void* data, uint8_t type) {
	if (getWorkFlag()) {
		if (mqtt_client.publish(code, String(POINTER_TO_TYPE(data, type)).c_str()) ) {
			system->setMqttSentFlag(true);
			return true;
		}
	}

	return false;
}


void MqttManager::writeSettings(char* buffer) {
	setParameter(buffer, "MSwf", getWorkFlag());
	
	setParameter(buffer, "MSSs", (const char*) getServer());
	setParameter(buffer, "MSSp", getPort());
	setParameter(buffer, "MSAs", (const char*) getSsid());
	setParameter(buffer, "MSAp", (const char*) getPass());
}

void MqttManager::readSettings(char* buffer) {
	getParameter(buffer, "MSwf", &work_flag);

	getParameter(buffer, "MSSs", mqtt_server, MQTT_SERVER_SIZE);
	getParameter(buffer, "MSSp", &mqtt_port);
	getParameter(buffer, "MSAs", mqtt_ssid, MQTT_SSID_PASS_SIZE);
	getParameter(buffer, "MSAp", mqtt_pass, MQTT_SSID_PASS_SIZE);

	setWorkFlag(work_flag);
	setServer(mqtt_server, mqtt_port);
	setAccess(mqtt_ssid, mqtt_pass);
}


void MqttManager::setSystemManager(SystemManager* system) {
	this->system = system;
}


void MqttManager::setWorkFlag(bool work_flag) {
	this->work_flag = work_flag;

	if (!getWorkFlag()) {
		off();
	}
}

void MqttManager::setServer(String* mqtt_server, uint16_t mqtt_port) {
	setServer((mqtt_server != NULL) ? mqtt_server->c_str() : NULL, mqtt_port);
}
void MqttManager::setServer(const char* mqtt_server, uint16_t mqtt_port) {
	if (mqtt_server != NULL) {
		strcpy(this->mqtt_server, mqtt_server);
	}
	
	this->mqtt_port = mqtt_port;
	reset_request = true;

	mqtt_client.setServer(mqtt_server, mqtt_port);
}

void MqttManager::setAccess(String* mqtt_ssid, String* mqtt_pass) {
	setAccess((mqtt_ssid != NULL) ? mqtt_ssid->c_str() : NULL, (mqtt_pass != NULL) ? mqtt_pass->c_str() : NULL);
}
void MqttManager::setAccess(const char* mqtt_ssid, const char* mqtt_pass) {
	if (mqtt_ssid != NULL) {
		strcpy(this->mqtt_ssid, mqtt_ssid);
	}
	if (mqtt_pass != NULL) {
		strcpy(this->mqtt_pass, mqtt_pass);
	}

	reset_request = true;
}


int8_t MqttManager::getStatus() {
	return mqtt_client.state();
}

bool MqttManager::getWorkFlag() {
	return work_flag;
}


char* MqttManager::getServer() {
	return mqtt_server;
}

uint16_t MqttManager::getPort() {
	return mqtt_port;
}

char* MqttManager::getSsid() {
	return mqtt_ssid;
}

char* MqttManager::getPass() {
	return mqtt_pass;
}


void MqttManager::notifyObservers(String code, void* data, uint8_t type) {
	for (uint8_t i = 0;i < observers.size();i++) {
		if (observers[i]->handleEvent(code.c_str(), data, type)) {
			return;
		}
	}
}


void MqttManager::off() {
	mqtt_client.disconnect();
	reconnect_timer = 0;
}

void MqttManager::connect() {
	if (!reconnect_timer || millis() - reconnect_timer >= SEC_TO_MLS(MQTT_RECONNECT_TIME)) {
		reconnect_timer = millis();
		
		mqtt_client.setServer(mqtt_server, mqtt_port);
		if (mqtt_client.connect("ESP8266Client", getSsid(), getPass()) ) {
			mqtt_client.subscribe("/#");
		}
		Serial.println("connect mqtt");
	}
}