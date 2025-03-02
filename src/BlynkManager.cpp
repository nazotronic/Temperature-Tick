/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

BlynkManager::BlynkManager() {
	makeDefault();
}


void BlynkManager::makeDefault() {
	setSystemManager(NULL);

	setWorkFlag(DEFAULT_BLYNK_WORK_STATUS);
	setAuth("");
	
	observers.clear();
	links.clear();

	reset_request = true;
	reconnect_timer = 0;
}

void BlynkManager::begin() {
	tick();
}

void BlynkManager::tick() {
	NetworkManager* network = system->getNetworkManager();

	if (reset_request) {
		Serial.println("reset blynk");

		reset_request = false;
		off();
	}

	if (!getWorkFlag() || !*getAuth() || network->getStatus() != WL_CONNECTED) {
		return;
	}

	if (!getStatus()) {
		connect();
	}

	Blynk.run();
}

void BlynkManager::addElementCodes(DynamicArray<String>* array) {
	
}


bool BlynkManager::handleEvent(const char* code, void* data, uint8_t type) {
	if (getWorkFlag() && getStatus()) {
		for (uint8_t i = 0;i < getLinksCount();i++) {
			if (!strcmp(getLinkElementCode(i), code)) {
				Blynk.virtualWrite(getLinkPort(i), POINTER_TO_TYPE(data, type));
				delay(10);

				system->setBlynkSentFlag(true);
				return true;
			}
		}
	}

	return false;
}

void BlynkManager::addObserver(IObserver* observer) {
	if (observer == NULL) {
		return;
	}

	observers.add(observer);
}


void BlynkManager::writeSettings(char* buffer) {
	setParameter(buffer, "BSwf", getWorkFlag());
	setParameter(buffer, "BSa", (const char*) getAuth());

	for (uint8_t i = 0;i < links.size();i++) {
		setParameter(buffer, String("BSLp") + i, getLinkPort(i));
		setParameter(buffer, String("BSLe") + i, (const char*) getLinkElementCode(i));
	}
}

void BlynkManager::readSettings(char* buffer) {
	uint8_t link_index = 0;
	char element_code[BLYNK_ELEMENT_CODE_SIZE];

	getParameter(buffer, "BSwf", &work_flag);
	getParameter(buffer, "BSa", auth, BLYNK_AUTH_SIZE);

	while (getParameter(buffer, String("BSLe") + link_index, element_code, BLYNK_ELEMENT_CODE_SIZE)) {
		if (addLink()) {
			uint8_t link_port;
			setLinkElementCode(link_index, element_code);

			if (getParameter(buffer, String("BSLp") + link_index, &link_port)) {
				setLinkPort(link_index, link_port);
			}
		}

		link_index++;
	}
	
	setWorkFlag(work_flag);
	setAuth(auth);
}


bool BlynkManager::addLink() {
	if (links.add()) {
		setLinkPort(links.size() - 1, links.size() - 1);

		return true;
	}

	return false;
}

bool BlynkManager::deleteLink(uint8_t index) {
	if (links.del(index)) {
		return true;
	}

	return false;
}

bool BlynkManager::deleteLink(String element_code) {
	return deleteLink(scanLinkIndex(element_code));
}

bool BlynkManager::modifyLinkElementCode(String previous_code, String new_code) {
	int8_t link_index = scanLinkIndex(previous_code);

	if (link_index < 0) {
		return false;
	}

	setLinkElementCode(link_index, new_code);
	return true;
}


void BlynkManager::setSystemManager(SystemManager* system) {
	this->system = system;
}


void BlynkManager::setWorkFlag(bool work_flag) {
	this->work_flag = work_flag;

	if (!work_flag) {
		off();
	}
}

void BlynkManager::setAuth(String auth) {
	strcpy(this->auth, auth.c_str());
	reset_request = true;
}


void BlynkManager::setLinkPort(uint8_t index, uint8_t port) {
	if (!isCorrectLinkIndex(index)) {
		return;
	}

	links[index].port = port;
}

void BlynkManager::setLinkElementCode(uint8_t index, String code) {
	if (!isCorrectLinkIndex(index)) {
		return;
	}

	strcpy(links[index].element_code, code.c_str());
}


bool BlynkManager::getStatus() {
	return Blynk.connected();
}


bool BlynkManager::getWorkFlag() {
	return work_flag;
}

char* BlynkManager::getAuth() {
	return auth;
}


uint8_t BlynkManager::getLinksCount() {
	return links.size();
}

uint8_t BlynkManager::getLinkPort(uint8_t index) {
	if (!isCorrectLinkIndex(index)) {
		return 0;
	}

	return links[index].port;
}

char* BlynkManager::getLinkElementCode(uint8_t index) {
	if (!isCorrectLinkIndex(index)) {
		return NULL;
	}

	return links[index].element_code;
}


void BlynkManager::notifyObservers(String code, void* data, uint8_t type) {
	for (uint8_t i = 0;i < observers.size();i++) {
		if (observers[i]->handleEvent(code.c_str(), data, type)) {
			return;
		}
	}
}


bool BlynkManager::isCorrectLinkIndex(uint8_t index) {
	if (index >= getLinksCount()) {
		return false;
	}

	return true;
}

int8_t BlynkManager::scanLinkIndex(String element_code) {
	if (!links.size()) {
		return -1;
	}

	for (uint8_t i = 0; i < links.size();i++) {
		if (!strcmp(links[i].element_code, element_code.c_str()) ) {
			return i;
		}
	}

	return -1;
}


void BlynkManager::off() {
	Blynk.disconnect();
}

void BlynkManager::connect() {
	if (!*getAuth()) {
		return;
	}

	if (!reconnect_timer || millis() - reconnect_timer >= SEC_TO_MLS(BLYNK_RECONNECT_TIME)) {
		reconnect_timer = millis();
		
		Blynk.config(this->auth);
		Blynk.connect(10);
	}
}

extern SystemManager systemManager;
BLYNK_WRITE_DEFAULT() {
	BlynkManager* blynk = systemManager.getBlynkManager();

	for (uint8_t i = 0;i < blynk->getLinksCount();i++) {
		if (blynk->getLinkPort(i) == request.pin) {
			float data = param.asFloat();

			blynk->notifyObservers(blynk->getLinkElementCode(i), &data, TYPE_FLOAT);
			return;
		}
	}
}

WiFiClient BlynkManager::_blynkWifiClient = WiFiClient();
BlynkArduinoClient BlynkManager::_blynkTransport = BlynkArduinoClient(_blynkWifiClient);
BlynkWifi BlynkManager::Blynk = BlynkWifi(_blynkTransport); 