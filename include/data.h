/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 * 
 * Features:
 * 1. Supports reading up to 10 ds18b20 temperature sensors with calibration capability.
 * 2. Sends temperature data to MQTT broker.
 * 3. Blynk platform support for remote monitoring.
 * 4. Saves power by sleeping between readings.
 * 5. Allows settings from web interface.
 * 7. Compact & Efficient – Optimized for low power consumption and minimal resource usage.
 */

#pragma once
#include <Arduino.h>
#include <DallasTemperature.h>
#include <settings.h>
#include <DynamicArray.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <GyverPortal.h>

#define NO_GLOBAL_BLYNK
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

/* --- Ports --- */
#define DS18B20_PORT D5
#define BUTTON_PORT D6
#define RELAY_PORT D7

/* --- Types --- */
#define TYPE_BOOL 0
#define TYPE_UINT8_T 1
#define TYPE_INT8_T 2
#define TYPE_UINT16_T 3
#define TYPE_INT16_T 4
#define TYPE_UINT32_T 5
#define TYPE_INT32_T 6
#define TYPE_FLOAT 7

/* --- Defaults --- */
/* SensorsManager */
#define DEFAULT_SLEEP_STATUS false
#define DEFAULT_SLEEP_TIME 10 // min

/* SensorsManager */
#define DEFAULT_READ_DATA_TIME 5 // sec
#define DEFAULT_DS18B20_NAME "Tn"
#define DEFAULT_DS18B20_RESOLUTION 12

/* RelayManager */
#define DEFAULT_RELAY_INVERT_FLAG true
#define DEFAULT_RELAY_MODE 0
#define DEFAULT_RELAY_THERM_SENSOR_INDEX -1
#define DEFAULT_RELAY_THERM_T 20.0
#define DEFAULT_RELAY_THERM_DELTA 1.0
#define DEFAULT_RELAY_THERM_MODE 0
#define DEFAULT_RELAY_THERM_ERROR_RELE_FLAG false

/* NetworkManager */
#define DEFAULT_NETWORK_MODE NETWORK_AUTO
#define DEFAULT_NETWORK_SSID_AP "nztr_solar"
#define DEFAULT_NETWORK_PASS_AP "nazotronic"

/* BlynkManager */
#define DEFAULT_BLYNK_WORK_STATUS true
#define DEFAULT_BLYNK_SEND_DATA_TIME DEFAULT_READ_DATA_TIME // sec

/* MqttManager */
#define DEFAULT_MQTT_WORK_STATUS true

/* --- Macroces --- */
/* SystemManager */
#define SAVE_SETTINGS_TIME 5 // sec
#define WORK_TIME 18 // sec
#define SETTINGS_BUFFER_SIZE 1100

/* SensorsManager */
#define UNSPECIFIED_STATUS 255
#define DS_SENSORS_MAX_COUNT 10
#define DS_NAME_SIZE 3

/* RelayManager */
#define RELAY_MODE_SIMPLE 0
#define RELAY_MODE_THERM 1

#define RELAY_THERM_MODE_HEATING 0
#define RELAY_THERM_MODE_COOLING 1

/* NetworkManager */
#define NETWORK_OFF 0
#define NETWORK_STA 1
#define NETWORK_AP_STA 2
#define NETWORK_AUTO 3
#define NETWORK_SSID_PASS_SIZE 15
#define NETWORK_RECONNECT_TIME 20 // sec

/* Web */
#define WEB_UPDATE_TIME 5 // sec

/* BlynkManager */
#define BLYNK_LINKS_MAX 20
#define BLYNK_AUTH_SIZE 35
#define BLYNK_ELEMENT_CODE_SIZE 40
#define BLYNK_RECONNECT_TIME 20 // sec

/* MqttManager */
#define MQTT_SERVER_SIZE 60
#define MQTT_SSID_PASS_SIZE 20
#define MQTT_RECONNECT_TIME 20 // sec

/* --- Macro functions --- */
#define SEC_TO_MLS(TIME) ((TIME) * 1000)
#define MIN_TO_MLS(TIME) ((TIME) * 60000)
#define IS_EVEN_SECOND(MLS) ((MLS / 1000) % 2)

#define POINTER_TO_TYPE(POINTER, TYPE) (\
    (TYPE == TYPE_BOOL)     ? *(bool*)(POINTER) : \
    (TYPE == TYPE_UINT8_T)  ? *(uint8_t*)(POINTER) : \
    (TYPE == TYPE_INT8_T)   ? *(int8_t*)(POINTER) : \
    (TYPE == TYPE_UINT16_T) ? *(uint16_t*)(POINTER) : \
    (TYPE == TYPE_INT16_T)  ? *(int16_t*)(POINTER) : \
    (TYPE == TYPE_UINT32_T) ? *(uint32_t*)(POINTER) : \
    (TYPE == TYPE_INT32_T)  ? *(int32_t*)(POINTER) : \
    (TYPE == TYPE_FLOAT)    ? *(float*)(POINTER) : *(uint8_t*)(POINTER) \
)

#define TYPE_TO_LEN(TYPE) (\
    (TYPE == TYPE_BOOL)     ? sizeof(bool) : \
    (TYPE == TYPE_UINT8_T)  ? sizeof(uint8_t) : \
    (TYPE == TYPE_INT8_T)   ? sizeof(int8_t) : \
    (TYPE == TYPE_UINT16_T) ? sizeof(uint16_t) : \
    (TYPE == TYPE_INT16_T)  ? sizeof(int16_t) : \
    (TYPE == TYPE_UINT32_T) ? sizeof(uint32_t) : \
    (TYPE == TYPE_INT32_T)  ? sizeof(int32_t) : \
    (TYPE == TYPE_FLOAT)    ? sizeof(float) : 0 \
)


struct ds18b20_data_t {
	void operator=(const ds18b20_data_t& other) {
		strcpy(name, other.name);
		memcpy(address, other.address, sizeof(DeviceAddress));
		resolution = other.resolution;
		correction = other.correction;

		t = other.t;
		status = other.status;
	}
	
	char name[DS_NAME_SIZE];
	DeviceAddress address;
	uint8_t resolution;
	float correction;
	
	float t;
	uint8_t status;
};

struct blynk_link_t {
	void operator=(const blynk_link_t& other) {
		port = other.port;
		strcpy(element_code, other.element_code);
	}

	uint8_t port;
	char element_code[BLYNK_ELEMENT_CODE_SIZE];
};


class SystemManager;

class IObserver {
public:
	virtual void addObserver(IObserver* observer) = 0;
	virtual bool handleEvent(const char* code, void* data, uint8_t type) = 0;
};

class IManager : public IObserver {
public:
	virtual void makeDefault() = 0;
	virtual void begin() = 0;
	virtual void tick() = 0;
	virtual void addElementCodes(DynamicArray<String>* array) = 0;
};

class SensorsManager : public IManager {
public:
	SensorsManager();
	
	/* --- IManager --- */
	void makeDefault() override;
	void begin() override;
	void tick() override;
	void addElementCodes(DynamicArray<String>* array) override;

	/* --- IObserver --- */
	void addObserver(IObserver* observer) override;
	bool handleEvent(const char* code, void* data, uint8_t type) override;

	void writeSettings(char* buffer);
	void readSettings(char* buffer);
	void updateSensorsData();

	bool addDS18B20();
	bool deleteDS18B20(uint8_t index);
	
	uint8_t makeDS18B20AddressList(DynamicArray<DeviceAddress>* array, DynamicArray<String>* string_array = NULL);
	int8_t scanDS18B20AddressIndex(DynamicArray<DeviceAddress>* array, uint8_t* address);
	
	void setSystemManager(SystemManager* system);
	void setReadDataTime(uint8_t time);

	void setDS18B20(uint8_t index, ds18b20_data_t* ds18b20);
	void setDS18B20Name(uint8_t index, String name);
	void setDS18B20Address(uint8_t index, uint8_t* address, bool sync_flag = true);
	void setDS18B20Resolution(uint8_t index, uint8_t resolution, bool sync_flag = true);
	void setDS18B20Correction(uint8_t index, float correction);

	DallasTemperature* getDallasTemperature();
	uint8_t getReadDataTime();

	uint8_t getGlobalDS18B20Count();
	float getDS18B20TByAddress(uint8_t* address);

	uint8_t getDS18B20Count();
	ds18b20_data_t* getDS18B20(uint8_t index);
	char* getDS18B20Name(uint8_t index);
	uint8_t* getDS18B20Address(uint8_t index);
	uint8_t getDS18B20Resolution(uint8_t index, bool sync_flag = true);
	float getDS18B20Correction(uint8_t index);
	float getDS18B20T(uint8_t index);
	uint8_t getDS18B20Status(uint8_t index);

private:
	void notifyObservers(String code, void* data, uint8_t type);
	
	bool isCorrectDS18B20Index(uint8_t index);
	void DS18B20AddressToString(uint8_t* address, String* string);

	/* --- classes & structures --- */
	OneWire oneWire;
	DallasTemperature ds18b20_sensor;
	SystemManager* system;

	/* --- settings --- */
	uint8_t read_data_time;

	DynamicArray<ds18b20_data_t> ds18b20_data;

	/* --- variables --- */
	DynamicArray<IObserver*> observers;
	uint32_t read_data_timer;
};

class RelayManager : public IManager {
public:
	RelayManager();

	/* --- IManager --- */
	void makeDefault() override;
	void begin() override;
	void tick() override;
	void addElementCodes(DynamicArray<String>* array) override;

	/* --- IObserver --- */
	void addObserver(IObserver* observer) override;
	bool handleEvent(const char* code, void* data, uint8_t type) override;

	void writeSettings(char* buffer);
	void readSettings(char* buffer);

	void setSystemManager(SystemManager* system);
	void setRelayFlag(bool relay_flag, bool sync_flag = false);

	void setInvertFlag(bool invert_flag);
	void setMode(uint8_t mode);

	void setThermSensor(int8_t ds18b20_index);
	void setThermSetT(float t);
	void setThermDelta(float delta);
	void setThermMode(uint8_t mode);
	void setThermErrorRelayFlag(bool relay_flag);

	bool getRelayFlag();

	bool getInvertFlag();
	uint8_t getMode();
	
	uint8_t getThermStatus();
	float getThermT();
	int8_t getThermSensor();
	float getThermSetT();
	float getThermDelta();
	uint8_t getThermMode();
	bool getThermErrorRelayFlag();

private:
	void notifyObservers(String code, void* data, uint8_t type);
	void relayTick();

	/* --- classes & structures --- */
	SystemManager* system;

	/* --- settings --- */
	bool invert_flag;
	uint8_t mode;

	int8_t therm_sensor_index;
	float therm_set_t;
	float therm_delta;
	uint8_t therm_mode;
	bool therm_error_relay_flag;

	/* --- variables --- */
	DynamicArray<IObserver*> observers;
	bool relay_flag;
};

class Web {
public:
	void init();
	void start();
	void stop();
	void tick();

	void setSystemManager(SystemManager* system);
	bool getStatus();

private:
	/* --- functions --- */
	void updateSensorsBlock();
	void updateBlynkBlock();

	/* --- classes & structures --- */
	GyverPortal ui;

	struct sensors_block_t {
		String ds18b20_addresses_string;
		DynamicArray<DeviceAddress> ds18b20_addresses;
	};

	struct blynk_block_t {
		String element_codes_string;
		DynamicArray<String> element_codes;
	};
	
	/* --- variables --- */
	SystemManager* system;

	String update_codes;
	sensors_block_t sensors_block;
	blynk_block_t blynk_block;
};

class NetworkManager : public IManager {
public:
	NetworkManager();

	/* --- IManager --- */
	void makeDefault() override;
	void begin() override;
	void tick() override;
	void addElementCodes(DynamicArray<String>* array) override;

	/* --- IObserver --- */
	void addObserver(IObserver* observer) override;
	bool handleEvent(const char* code, void* data, uint8_t type) override;

	void writeSettings(char* buffer);
	void readSettings(char* buffer);
	
	void endBegin();
	bool connect(String ssid = String(""), String pass = String(""), uint8_t connect_time = 0, bool auto_save = false);

	bool isWifiOn();
	bool isApOn();

	void setSystemManager(SystemManager* system);

	void setMode(uint8_t mode);
	void setWifi(String* ssid, String* pass);
	void setWifi(const char* ssid, const char* pass);
	void setAp(String* ssid, String* pass);
	void setAp(const char* ssid, const char* pass);

	wl_status_t getStatus();

	uint8_t getMode();
	char* getWifiSsid();
	char* getWifiPass();
	char* getApSsid();
	char* getApPass();

private:
	void off();

	/* --- classes & structures --- */
	Web web;

	/* --- settings --- */
	uint8_t mode;

	char ssid_sta[NETWORK_SSID_PASS_SIZE];
	char pass_sta[NETWORK_SSID_PASS_SIZE];
	char ssid_ap[NETWORK_SSID_PASS_SIZE];
	char pass_ap[NETWORK_SSID_PASS_SIZE];

	/* --- variables --- */
	SystemManager* system;

	bool reset_request;
	bool tick_allow;
	uint32_t wifi_reconnect_timer;
};

class MqttManager : public IManager {
public:
	MqttManager();

	/* --- IManager --- */
	void makeDefault() override;
	void begin() override;
	void tick() override;
	void addElementCodes(DynamicArray<String>* array) override;

	/* --- IObserver --- */
	bool handleEvent(const char* code, void* data, uint8_t type) override;
	void addObserver(IObserver* observer) override;

	void writeSettings(char* buffer);
	void readSettings(char* buffer);

	void setSystemManager(SystemManager* system);

	void setWorkFlag(bool work_flag);
	void setServer(String* mqtt_server, uint16_t mqtt_port);
	void setServer(const char* mqtt_server, uint16_t mqtt_port);
	void setAccess(String* mqtt_ssid, String* mqtt_pass);
	void setAccess(const char* mqtt_ssid, const char* mqtt_pass);

	int8_t getStatus();
	bool getWorkFlag();

	char* getServer();
	uint16_t getPort();
	char* getSsid();
	char* getPass();

private:
	void notifyObservers(String code, void* data, uint8_t type);

	void off();
	void connect();

	/* --- classes & structures --- */
	WiFiClientSecure esp_client;
	PubSubClient mqtt_client;

	/* --- settings --- */
	bool work_flag;

	char mqtt_server[MQTT_SERVER_SIZE];
	uint16_t mqtt_port;
	char mqtt_ssid[MQTT_SSID_PASS_SIZE];
	char mqtt_pass[MQTT_SSID_PASS_SIZE];

	/* --- variables --- */
	DynamicArray<IObserver*> observers;
	SystemManager* system;

	bool reset_request;
	uint32_t reconnect_timer;
};

class BlynkManager : public IManager {
public:
	BlynkManager();
	
	/* --- IManager --- */
	void makeDefault() override;
	void begin() override;
	void tick() override;
	void addElementCodes(DynamicArray<String>* array) override;

	/* --- IObserver --- */
	bool handleEvent(const char* code, void* data, uint8_t type) override;
	void addObserver(IObserver* observer) override;

	void writeSettings(char* buffer);
	void readSettings(char* buffer);

	bool addLink();
	bool deleteLink(uint8_t index);
	bool deleteLink(String code);
	bool modifyLinkElementCode(String previous_code, String new_code);

	void setSystemManager(SystemManager* system);

	void setWorkFlag(bool work_flag);
	void setAuth(String auth);
	
	void setLinkPort(uint8_t index, uint8_t port);
	void setLinkElementCode(uint8_t index, String code);

	bool getStatus();

	bool getWorkFlag();
	char* getAuth();

	uint8_t getLinksCount();
	uint8_t getLinkPort(uint8_t index);
	char* getLinkElementCode(uint8_t index);

private:
	void notifyObservers(String code, void* data, uint8_t type);

	bool isCorrectLinkIndex(uint8_t index);
	int8_t scanLinkIndex(String element_code);

	void off();
	void connect();

	friend BLYNK_WRITE_DEFAULT();

	/* --- classes & structures --- */
	static WiFiClient _blynkWifiClient;
  	static BlynkArduinoClient _blynkTransport;
  	static BlynkWifi Blynk;
	
	/* --- settings --- */
	bool work_flag;
	char auth[BLYNK_AUTH_SIZE];

	/* --- variables --- */
	DynamicArray<IObserver*> observers;
	DynamicArray<blynk_link_t> links;
	SystemManager* system;

	bool reset_request;
	uint32_t reconnect_timer;
};

class SystemManager : public IManager {
public:
	SystemManager();
	
	/* --- IManager --- */
	void makeDefault() override;
	void begin() override;
	void tick() override;
	void addElementCodes(DynamicArray<String>* array) override;

	/* --- IObserver --- */
	void addObserver(IObserver* observer) override;
	bool handleEvent(const char* code, void* data, uint8_t type) override;
	
	void reset();
	void resetAll();
	void saveSettingsRequest();

	void makeElementCodesList(DynamicArray<String>* array);
	int8_t scanElementCodeIndex(DynamicArray<String>* array, String element_code);
	void handleElementRemoval(String element_code);
	void handleElementCodeUpdate(String previous_code, String new_code);

	void setSleepFlag(bool sleep_flag);
	void setSleepTime(uint8_t sleep_time);

	void setSensorsReadFlag(bool flag);
	void setMqttSentFlag(bool flag);
	void setBlynkSentFlag(bool flag);

	SensorsManager* getSensorsManager();
	RelayManager* getRelayManager();
	NetworkManager* getNetworkManager();
	MqttManager* getMqttManager();
	BlynkManager* getBlynkManager();

	bool getSleepFlag();
	uint8_t getSleepTime();

	bool getSensorsReadFlag();
	bool getMqttSentFlag();
	bool getBlynkSentFlag();

private:
	void notifyObservers(String code, void* data, uint8_t type);

	void saveSettings(bool ignore_flag = false);
	void readSettings();

	bool getButtonStatus();

	/* --- classes & structures --- */
	SensorsManager sensors;
	RelayManager relay;
	NetworkManager network;
	MqttManager mqtt;
	BlynkManager blynk;

	struct SleepReqs {
		bool sensors_read_flag = false;
		bool mqtt_sent_flag = false;
		bool blynk_sent_flag = false;

		void makeDefault() {
			sensors_read_flag = false;
			mqtt_sent_flag = false;
			blynk_sent_flag = false;
		}

		bool isReqsDone() {
			return sensors_read_flag &&
				   mqtt_sent_flag &&
				   blynk_sent_flag;
		}
	};
	
	/* --- settings --- */
	bool sleep_flag;
	uint8_t sleep_time;

	/* --- variables --- */
	DynamicArray<IObserver*> observers;
	SleepReqs sleep_reqs;

	bool save_settings_request;
	uint32_t save_settings_timer;
	uint32_t work_timer;
};


template <class T1, class T2, class T3, class T4>
T1 smartIncr(T1& value, T2 incr_step, T3 min, T4 max) {
	if (!incr_step) {
		return value;
	}

	if ((value == min && incr_step < 0) || (value == max && incr_step > 0)) {
		return value;
	}
	
	value += (T1) incr_step;
	value = (T1) constrain(value, min, max);
	
	return value;
}