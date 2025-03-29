/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

NetworkManager::NetworkManager() {
	makeDefault();
}


void NetworkManager::makeDefault() {
	setSystemManager(NULL);
	
	setMode(DEFAULT_NETWORK_MODE);
	setWifi("", "");
	setAp("", "");
	
	reset_request = true;
	tick_allow = true;
	wifi_reconnect_timer = 0;
}

void NetworkManager::begin() {
	tick();
}

void NetworkManager::tick() {
	if (!tick_allow) {
		return;
	}
	
	if (reset_request) {
		Serial.println("reset");

		reset_request = false;
		off();
	}

	if (getMode() == NETWORK_OFF) {
		if (WiFi.getMode() != WIFI_OFF) {
			off();
		}

		return;
	}

	else if (getMode() == NETWORK_STA) {
		if (WiFi.getMode() != WIFI_STA) {
			Serial.println("sta");
			
			WiFi.mode(WIFI_STA);
			
			if (!system->getSleepFlag()) {
				web.stop();
				web.start();
			}
		}
	}

	else if (getMode() == NETWORK_AP_STA) {
		if (WiFi.getMode() != WIFI_AP_STA) {
			Serial.println("ap_sta");
			
			WiFi.mode(WIFI_AP_STA);
			
			if (!system->getSleepFlag()) {
				web.stop();
				web.start();
			}
		}
	}

	else if (getMode() == NETWORK_AUTO) {
		if (getStatus() == WL_CONNECTED && WiFi.getMode() != WIFI_STA) {
			Serial.println("auto sta");
			
			WiFi.mode(WIFI_STA);
			
			if (!system->getSleepFlag()) {
				web.stop();
				web.start();
			}
		}
		
		else if (getStatus() != WL_CONNECTED && WiFi.getMode() != WIFI_AP_STA) {
			Serial.println("auto ap sta");
			
			WiFi.mode(WIFI_AP_STA);
			
			if (!system->getSleepFlag()) {
				web.stop();
				web.start();
			}
		}
	}


	if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
		if (getStatus() != WL_CONNECTED) {
			connect();
		}
	}

	if (!system->getSleepFlag()) {
		web.tick();
	}
}

void NetworkManager::addElementCodes(DynamicArray<String>* array) {

}


bool NetworkManager::handleEvent(const char* code, void* data, uint8_t type) {
	return false;
}

void NetworkManager::addObserver(IObserver* observer) {
	
}


void NetworkManager::writeSettings(char* buffer) {
	setParameter(buffer, "SNm", getMode());
	setParameter(buffer, "SNWs", (const char*) getWifiSsid());
	setParameter(buffer, "SNWp", (const char*) getWifiPass());
	setParameter(buffer, "SNAs", (const char*) getApSsid());
	setParameter(buffer, "SNAp", (const char*) getApPass());
}

void NetworkManager::readSettings(char* buffer) {
	getParameter(buffer, "SNm", &mode);
	getParameter(buffer, "SNWs", ssid_sta, NETWORK_SSID_PASS_SIZE);
	getParameter(buffer, "SNWp", pass_sta, NETWORK_SSID_PASS_SIZE);
	getParameter(buffer, "SNAs", ssid_ap, NETWORK_SSID_PASS_SIZE);
	getParameter(buffer, "SNAp", pass_ap, NETWORK_SSID_PASS_SIZE);
	
	setMode(mode);
	setAp(ssid_ap, pass_ap);
	setWifi(ssid_sta, pass_sta);
}


void NetworkManager::endBegin() {
	if (!system->getSleepFlag()) {
		web.init();
	}
}

bool NetworkManager::connect(String ssid, String pass, uint8_t connect_time, bool auto_save) {
	bool connect_status = false;
	
	if (!ssid[0]) {
		if (!wifi_reconnect_timer || millis() - wifi_reconnect_timer >= SEC_TO_MLS(NETWORK_RECONNECT_TIME)) {
			wifi_reconnect_timer = millis();
			
			WiFi.begin(getWifiSsid(), getWifiPass());
			connect_status = getStatus();

			Serial.println("connect wifi");
		}
	}
	else {
		uint32_t connect_timer = millis();
		tick_allow = false;
		
		off();
   		WiFi.mode(WIFI_STA);
		WiFi.begin(ssid, pass);

		while (connect_time && millis() - connect_timer < SEC_TO_MLS(connect_time)) {
			if (getStatus() == WL_CONNECTED) {
				connect_status = true;
				break;
			}
			
			system->tick();
		}
		
		if (auto_save && getStatus()) {
			setWifi(ssid.c_str(), pass.c_str());
		}

   		tick_allow = true;
		reset_request = true;
	}

	return connect_status;
}


bool NetworkManager::isWifiOn() {
  	return (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA);
}

bool NetworkManager::isApOn() {
  	return (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
}


void NetworkManager::setSystemManager(SystemManager* system) {
	this->system = system;
	web.setSystemManager(system);
}


void NetworkManager::setMode(uint8_t mode) {
  	this->mode = mode;
}

void NetworkManager::setWifi(String* ssid, String* pass) {
  	setWifi((ssid != NULL) ? ssid->c_str() : NULL, (pass != NULL) ? pass->c_str() : NULL);
}
void NetworkManager::setWifi(const char* ssid, const char* pass) {
	if (ssid != NULL) {
		strcpy(ssid_sta, ssid);
	}
	if (pass != NULL) {
		strcpy(pass_sta, pass);
	}

	reset_request = true;
}

void NetworkManager::setAp(String* ssid, String* pass) {
  	setAp((ssid != NULL) ? ssid->c_str() : NULL, (pass != NULL) ? pass->c_str() : NULL);
}
void NetworkManager::setAp(const char* ssid, const char* pass) {
	if (ssid != NULL) {
		strcpy(ssid_ap, (!*ssid) ? DEFAULT_NETWORK_SSID_AP : ssid);
	}
	if (pass != NULL) {
		strcpy(pass_ap, (!*pass) ? DEFAULT_NETWORK_PASS_AP : pass);
	}
	
	WiFiMode_t current_mode = WiFi.getMode();
	WiFi.softAP(ssid_ap, pass_ap);
	WiFi.mode(current_mode);
}


wl_status_t NetworkManager::getStatus() {
  	return WiFi.status();
}


uint8_t NetworkManager::getMode() {
  	return mode;
}

char* NetworkManager::getWifiSsid() {
  	return ssid_sta;
}

char* NetworkManager::getWifiPass() {
  	return pass_sta;
}

char* NetworkManager::getApSsid() {
  	return ssid_ap;
}

char* NetworkManager::getApPass() {
  	return pass_ap;
}


void NetworkManager::off() {
	if (!system->getSleepFlag()) {
		web.stop();
	}

	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	
	wifi_reconnect_timer = 0;
}