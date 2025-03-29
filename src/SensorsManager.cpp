/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

SensorsManager::SensorsManager() {
	makeDefault();
}


void SensorsManager::makeDefault() {
	setSystemManager(NULL);

	setReadDataTime(DEFAULT_READ_DATA_TIME);

	observers.clear();
	ds18b20_data.clear();
	ds18b20_data.setMaxSize(DS_SENSORS_MAX_COUNT);

	read_data_timer = 0;
}

void SensorsManager::begin() {
	oneWire.begin(DS18B20_PORT);
	ds18b20_sensor.setOneWire(&oneWire);
	
	ds18b20_sensor.begin();
	ds18b20_sensor.setResolution(12);
}

void SensorsManager::tick() {
	if (getReadDataTime()) {
		if (!read_data_timer || millis() - read_data_timer >= SEC_TO_MLS(getReadDataTime()) ) {
			read_data_timer = millis();
			updateSensorsData();
		}
	}
}

void SensorsManager::addElementCodes(DynamicArray<String>* array) {
	if (array == NULL) {
		return;
	}

	for (uint8_t i = 0; i < ds18b20_data.size();i++) {
		array->add(String("/sensors/data/ds18b20/temp/") + getDS18B20Name(i));
	}
}


void SensorsManager::addObserver(IObserver* observer) {
	if (observer == NULL) {
		return;
	}

	observers.add(observer);
}

bool SensorsManager::handleEvent(const char* code, void* data, uint8_t type) {
	if (strstr(code, "/sensors/data") != NULL) {
		return true;
	}

	return false;
}


void SensorsManager::writeSettings(char* buffer) {
	setParameter(buffer, "SSrdt", getReadDataTime());

	for (uint8_t i = 0;i < getDS18B20Count();i++) {
		setParameter(buffer, String("SSDSn") + i, (const char*) getDS18B20Name(i));
		setParameter(buffer, String("SSDSa") + i, getDS18B20Address(i), 8);
		setParameter(buffer, String("SSDSr") + i, getDS18B20Resolution(i));
		setParameter(buffer, String("SSDSc") + i, getDS18B20Correction(i));
  	}
}

void SensorsManager::readSettings(char* buffer) {
	uint8_t ds18b20_index = 0;
	char ds18b20_name[DS_NAME_SIZE];

	getParameter(buffer, "SSrdt", &read_data_time);
	
	while (getParameter(buffer, String("SSDSn") + ds18b20_index, ds18b20_name, DS_NAME_SIZE)) {
		if (addDS18B20()) {
			uint8_t ds18b20_address[8];
			uint8_t ds18b20_resolution;
			float ds18b20_correction;
			
			setDS18B20Name(ds18b20_index, ds18b20_name);

			if (getParameter(buffer, String("SSDSa") + ds18b20_index, ds18b20_address, 8)) {
				setDS18B20Address(ds18b20_index, ds18b20_address, false);
			}
			
			if (getParameter(buffer, String("SSDSr") + ds18b20_index, &ds18b20_resolution)) {
				setDS18B20Resolution(ds18b20_index, ds18b20_resolution, false);
			}

			if (getParameter(buffer, String("SSDSc") + ds18b20_index, &ds18b20_correction)) {
				setDS18B20Correction(ds18b20_index, ds18b20_correction);
			}
		}

		ds18b20_index++;
	}

	setReadDataTime(read_data_time);
}

void SensorsManager::updateSensorsData() {
	ds18b20_sensor.requestTemperatures();

	for (uint8_t i = 0;i < getDS18B20Count();i++) {
		ds18b20_data[i].t = ds18b20_sensor.getTempC(getDS18B20Address(i));

		if (getDS18B20T(i) < -100) {
			ds18b20_data[i].status = 1;
		}
		else if (getDS18B20T(i) == 85) {
			ds18b20_data[i].status = 2;
		}
		else {
			ds18b20_data[i].status = 0;
			ds18b20_data[i].t += getDS18B20Correction(i);
		}

		system->setSensorsReadFlag(true);
		notifyObservers(String("/sensors/data/ds18b20/temp/") + getDS18B20Name(i), &ds18b20_data[i].t, TYPE_FLOAT);
	}
}


bool SensorsManager::addDS18B20() {
	if (ds18b20_data.add()) {
		setDS18B20Name(ds18b20_data.size() - 1, DEFAULT_DS18B20_NAME);
		setDS18B20Resolution(ds18b20_data.size() - 1, DEFAULT_DS18B20_RESOLUTION);
		ds18b20_data[ds18b20_data.size() - 1].status = UNSPECIFIED_STATUS;

		return true;
	}

	return false;
}

bool SensorsManager::deleteDS18B20(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return false;
	}

	if (ds18b20_data.del(index)) {
		system->handleElementRemoval(String("/sensors/data/ds18b20/temp/") + getDS18B20Name(index));
		return true;
	}

	return false;
}


uint8_t SensorsManager::makeDS18B20AddressList(DynamicArray<DeviceAddress>* array, DynamicArray<String>* string_array) {
	if (array == NULL) {
		return 0;
	}

	uint8_t sensors_count = getGlobalDS18B20Count();
	array->clear();

	if (string_array != NULL) {
		string_array->clear();
	}

	if (!sensors_count) {
		return 0;
	}

	for (uint8_t i = 0;i < sensors_count;i++) {
		DeviceAddress address;

		ds18b20_sensor.getAddress(address, i);
		array->add(&address);

		if (string_array != NULL) {
			String string_address;

			DS18B20AddressToString(address, &string_address);
			string_array->add(string_address);
		}
	}

	return sensors_count;
}

int8_t SensorsManager::scanDS18B20AddressIndex(DynamicArray<DeviceAddress>* array, uint8_t* address) {
	if (array == NULL || address == NULL) {
		return -1;
	}

	for (uint8_t i = 0; i < array->size();i++) {
		if (!memcmp((*array)[i], address, 8)) {
			return i;
		}
	}

	return -1;
}


void SensorsManager::setSystemManager(SystemManager* system) {
	this->system = system;
}

void SensorsManager::setReadDataTime(uint8_t time) {
	read_data_time = constrain(time, 0, 100);
}


void SensorsManager::setDS18B20(uint8_t index, ds18b20_data_t* ds18b20) {
	setDS18B20Name(index, ds18b20->name);
	setDS18B20Address(index, ds18b20->address);
	setDS18B20Resolution(index, ds18b20->resolution);
	setDS18B20Correction(index, ds18b20->correction);
}

void SensorsManager::setDS18B20Name(uint8_t index, String name) {
	if (!isCorrectDS18B20Index(index)) {
		return;
	}

	system->handleElementCodeUpdate(String("/sensors/data/ds18b20/temp/") + getDS18B20Name(index), String("/sensors/data/ds18b20/temp/") + name);
	name.toCharArray(ds18b20_data[index].name, 3);
}

void SensorsManager::setDS18B20Address(uint8_t index, uint8_t* address, bool sync_flag) {
	if (!isCorrectDS18B20Index(index) || !*address) {
		return;
	}

	memcpy(ds18b20_data[index].address, address, 8);

	if (sync_flag) {
		setDS18B20Resolution(index, getDS18B20Resolution(index, false));
	};
}

void SensorsManager::setDS18B20Resolution(uint8_t index, uint8_t resolution, bool sync_flag) {
	if (!isCorrectDS18B20Index(index)) {
		return;
	}

	if (sync_flag && *getDS18B20Address(index)) {
		ds18b20_sensor.setResolution(getDS18B20Address(index), resolution);
		ds18b20_data[index].resolution = ds18b20_sensor.getResolution(getDS18B20Address(index));
	}
	else {
		ds18b20_data[index].resolution = resolution;
	}
}

void SensorsManager::setDS18B20Correction(uint8_t index, float correction) {
	if (!isCorrectDS18B20Index(index)) {
		return;
	}

	ds18b20_data[index].correction = constrain(correction, -20.0, 20.0);
}


DallasTemperature* SensorsManager::getDallasTemperature() {
	return &ds18b20_sensor;
}

uint8_t SensorsManager::getReadDataTime() {
	return read_data_time;
}


uint8_t SensorsManager::getGlobalDS18B20Count() {
	ds18b20_sensor.begin();
	return ds18b20_sensor.getDS18Count();
}

float SensorsManager::getDS18B20TByAddress(uint8_t* address) {
	ds18b20_sensor.requestTemperaturesByAddress(address);
	return ds18b20_sensor.getTempC(address);
}


uint8_t SensorsManager::getDS18B20Count() {
	return ds18b20_data.size();
}

ds18b20_data_t* SensorsManager::getDS18B20(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return NULL;
	}

	return &ds18b20_data[index];
}

char* SensorsManager::getDS18B20Name(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return NULL;
	}

	return ds18b20_data[index].name;
}

uint8_t* SensorsManager::getDS18B20Address(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return NULL;
	}

	return ds18b20_data[index].address;
}

uint8_t SensorsManager::getDS18B20Resolution(uint8_t index, bool sync_flag) {
	if (!isCorrectDS18B20Index(index)) {
		return 0;
	}

	if (*getDS18B20Address(index) && sync_flag) {
		ds18b20_data[index].resolution = ds18b20_sensor.getResolution(getDS18B20Address(index));
	}

	return ds18b20_data[index].resolution;
}

float SensorsManager::getDS18B20Correction(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return 0;
	}

	return ds18b20_data[index].correction;
}

float SensorsManager::getDS18B20T(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return 0;
	}

	return ds18b20_data[index].t;
}

uint8_t SensorsManager::getDS18B20Status(uint8_t index) {
	if (!isCorrectDS18B20Index(index)) {
		return UNSPECIFIED_STATUS;
	}

	return ds18b20_data[index].status;
}


void SensorsManager::notifyObservers(String code, void* data, uint8_t type) {
	for (uint8_t i = 0;i < observers.size();i++) {
		observers[i]->handleEvent(code.c_str(), data, type);
	}
}


bool SensorsManager::isCorrectDS18B20Index(uint8_t index) {
	if (index >= getDS18B20Count()) {
		return false;
	}

	return true;
}

void SensorsManager::DS18B20AddressToString(uint8_t* address, String* string) {
	if (address == NULL || string == NULL) {
		return;
	}

	for (uint8_t i = 0;i < 8;i++) {
		*string += String(address[i], HEX);

		if (i != 7) {
			*string += "-";
		}
	}
}