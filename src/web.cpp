/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

void Web::init() {
	update_codes += "_NSm,_NSWs,_NSAs,_NSAp,";
	update_codes += "_MSwf,_MSSs,_MSSp,_MSAs,_MSAp,";
	update_codes += "_BSwf,_BSa,";
	update_codes += "_SSrdt,";
	update_codes += "_SSsf,_SSst,";

	ui.setFS(&LittleFS);
	ui.enableOTA();
	
	ui.attachBuild([this]() {
		SensorsManager* sensors = system->getSensorsManager();
		NetworkManager* network = system->getNetworkManager();
		MqttManager* mqtt = system->getMqttManager();
		BlynkManager* blynk = system->getBlynkManager();
		String update_codes = this->update_codes;

		for (byte i = 0;i < sensors->getDS18B20Count();i++) {
			update_codes += "SDDt";
			update_codes += i;
			update_codes += ",";
	
			update_codes += "_SSDn";
			update_codes += i;
			update_codes += ",";
			update_codes += "_SSDa";
			update_codes += i;
			update_codes += ",";
			update_codes += "_SSDr";
			update_codes += i;
			update_codes += ",";
			update_codes += "_SSDc";
			update_codes += i;
			update_codes += ",";
		}

		for (uint8_t i = 0;i < blynk->getLinksCount();i++) {
			update_codes += "_BSLp";
			update_codes += i;
			update_codes += ",";
			update_codes += "_BSLe";
			update_codes += i;
	
			if (i != blynk->getLinksCount() - 1) {
				update_codes += ",";
			}
		}

		GP.BUILD_BEGIN(550);
		GP.THEME(GP_DARK);
		GP.UPDATE(update_codes, SEC_TO_MLS(WEB_UPDATE_TIME));
		
		GP.TITLE("nazotronic");
		GP.NAV_TABS_LINKS("/,/settings,/memory", "Home,Settings,Memory", GP_ORANGE);
		GP.HR();

		if (ui.uri("/")) {
			M_SPOILER("Info", GP_ORANGE,
				GP.SYSTEM_INFO("1.0.0");
			);
			
			M_BLOCK(GP_THIN,
				GP.LABEL("Sensors");
				
				for (uint8_t i = 0;i < sensors->getDS18B20Count();i++) {
					M_BOX(GP_LEFT,
						GP.LABEL(sensors->getDS18B20Name(i), String("_SSDn") + i); // sensors settings ds name
						GP.LABEL(":");
						
						if (!sensors->getDS18B20Status(i)) {
							GP.PLAIN(String(sensors->getDS18B20T(i), 1) + "°", String("SDDt") + i);
						}
						else {
							GP.PLAIN("err", String("SDDt") + i);
						}
					);
				}
			);

			GP.HR();
			GP.SPAN("Temperature Tick", GP_LEFT);
			GP.SPAN("Author: Vereshchynskyi Nazar", GP_LEFT);
			GP.SPAN("Version: 1.0.0", GP_LEFT);
			GP.SPAN("Date: 02.03.2025", GP_LEFT);
		}

		if (ui.uri("/settings")) {
			M_SPOILER("Network", GP_ORANGE,
				M_BOX(GP_LEFT,
					GP.LABEL("Mode:");
					GP.SELECT("_NSm", "off,sta,ap_sta,auto", network->getMode());
				);
				
				M_FORM2("/_NSW",
					M_BLOCK(GP_THIN,
						GP.TITLE("WiFi");
						GP.TEXT("_NSWs", "ssid", network->getWifiSsid(), "50%", NETWORK_SSID_PASS_SIZE);
						GP.PASS_EYE("_NSWp", "pass", "", "", NETWORK_SSID_PASS_SIZE);
						GP.BREAK();
						GP.SUBMIT_MINI(" OK ", GP_ORANGE);
					);
				);
				M_BLOCK(GP_THIN,
					GP.TITLE("AP");
					GP.TEXT("_NSAs", "ssid", network->getApSsid(), "50%", NETWORK_SSID_PASS_SIZE);
					GP.PASS_EYE("_NSAp", "pass", network->getApPass(), "", NETWORK_SSID_PASS_SIZE);
				);
			);
			GP.BREAK();

			M_SPOILER("MQTT", GP_ORANGE,
				M_BOX(GP_LEFT,
					GP.LABEL("Status:");
					GP.SWITCH("_MSwf", mqtt->getWorkFlag());
				);
				
				M_FORM2("/_MSS",
					M_BLOCK(GP_THIN,
						GP.TITLE("Server");
						GP.TEXT("_MSSs", "ssid", mqtt->getServer(), "50%", MQTT_SERVER_SIZE);
						GP.NUMBER("_MSSp", "port", mqtt->getPort(), "25%");
						GP.BREAK();
						GP.SUBMIT_MINI(" OK ", GP_ORANGE);
					);
				);
				M_FORM2("/_MSA",
					M_BLOCK(GP_THIN,
						GP.TITLE("Access");
						GP.TEXT("_MSAs", "ssid", mqtt->getSsid(), "50%", MQTT_SSID_PASS_SIZE);
						GP.PASS_EYE("_MSAp", "pass","", "", MQTT_SSID_PASS_SIZE);
						GP.BREAK();
						GP.SUBMIT_MINI(" OK ", GP_ORANGE);
					);
				);
			);
			GP.BREAK();
			
			M_SPOILER("Blynk", GP_ORANGE,
				M_BOX(GP_LEFT,
					GP.LABEL("Status:");
					GP.SWITCH("_BSwf", blynk->getWorkFlag());
				);
				
				M_BOX(GP_LEFT,
					GP.LABEL("Auth:");
					GP.TEXT("_BSa", "auth", blynk->getAuth(), "100%", BLYNK_AUTH_SIZE);
				);
	
				M_BLOCK(GP_THIN,
					GP.TITLE("Links");
					GP.BUTTON("BSLs", "Scan", "", GP_ORANGE, "45%", false, true);
					
					for (uint8_t i = 0;i < blynk->getLinksCount();i++) {
						M_BOX(GP_LEFT,
							char* link_element_code = blynk->getLinkElementCode(i);
							uint8_t index = system->scanElementCodeIndex(&blynk_block.element_codes, link_element_code);
	
							GP.LABEL("V");
							GP.NUMBER(String("_BSLp") + i, "port", blynk->getLinkPort(i), "30%");
							GP.SELECT(String("_BSLe") + i, blynk_block.element_codes_string, index);
							GP.BUTTON(String("_BSLd") + i, "Delete", "", GP_ORANGE, "20%", false, true);
						);
					}

					GP.BUTTON("_BSLnl", "New link", "", GP_ORANGE, "45%", false, true);
				);
			);
			GP.BREAK();
	
			M_SPOILER("Sensors", GP_ORANGE,
				M_BOX(GP_LEFT,
					GP.LABEL("Read data time:");
					GP.NUMBER("_SSrdt", "time", sensors->getReadDataTime(), "25%");
					GP.PLAIN("sec");
				);
	
				M_BLOCK(GP_THIN,
					GP.TITLE("DS18B20");
					GP.BUTTON("SSDs", "Scan", "", GP_ORANGE, "45%", false, true); // sensors settings ds scan
	
					for (uint8_t i = 0;i < sensors->getDS18B20Count();i++) {
						M_BLOCK(GP_THIN, 
							M_BOX(GP_CENTER,
								GP.TEXT(String("_SSDn") + i, "", sensors->getDS18B20Name(i), "17%", 2);
							);
	
							M_BOX(GP_LEFT,
								uint8_t* ds18b20_address = sensors->getDS18B20Address(i);
								uint8_t index = sensors->scanDS18B20AddressIndex(&sensors_block.ds18b20_addresses, ds18b20_address);
								
								GP.LABEL("Address:");
								GP.SELECT(String("_SSDa") + i, sensors_block.ds18b20_addresses_string, index);
							);
							
							M_BOX(GP_LEFT,
								GP.LABEL("Resolution:");
								GP.NUMBER(String("_SSDr") + i, "", sensors->getDS18B20Resolution(i), "25%");
								GP.PLAIN("bit");
							);
	
							M_BOX(GP_LEFT,
								GP.LABEL("Correction:");
								GP.NUMBER_F(String("_SSDc") + i, "", sensors->getDS18B20Correction(i), 2, "25%");
								GP.PLAIN("°");
							);
	
							GP.BUTTON(String("_SSDd") + i, "Delete", "", GP_ORANGE, "20%", false, true);
						);
					}
	
					GP.BUTTON("_SSDnd", "New", "", GP_ORANGE, "45%", false, true);
				);
			);
			GP.BREAK();
	
			M_SPOILER("System", GP_ORANGE,
				M_BOX(GP_LEFT,
					GP.LABEL("Sleep:");
					GP.SWITCH("_SSsf", system->getSleepFlag());
				);
				
				M_BOX(GP_LEFT,
					GP.LABEL("Sleep time:");
					GP.NUMBER("_SSst", "", system->getSleepTime(), "25%");
				);

				M_BLOCK(GP_THIN,
					GP.TITLE("Management");
	
					GP.BUTTON("SSr", "RESET", "", GP_ORANGE, "45%");
					GP.BUTTON("SSra", "ALL", "", GP_ORANGE, "45%");
					GP.BUTTON_LINK("/ota_update", "OTA", GP_YELLOW, "45%");
				);
			);
		}
	
		if (ui.uri("/memory")) {
			GP.FILE_MANAGER(&LittleFS);
			GP.FILE_UPLOAD("file");
		}
	
		GP.BUILD_END();
	});

	ui.attach([this]() {
		SensorsManager* sensors = system->getSensorsManager();
		NetworkManager* network = system->getNetworkManager();
		MqttManager* mqtt = system->getMqttManager();
		BlynkManager* blynk = system->getBlynkManager();

		/* --- Home --- */
		// update
		for (uint8_t i = 0;i < sensors->getDS18B20Count();i++) {
			if (ui.update(String("SDDt") + i)) {
				ui.answer(!sensors->getDS18B20Status(i) ? String(sensors->getDS18B20T(i), 1) + "°" : String("err"));
				return;
			}
		}
		/* --- Home --- */

		if (ui.clickSub("_") || ui.formSub("/_")) {
			system->saveSettingsRequest();
		}
	
		/* --- NetworkManager --- */
		// update
		if (ui.update("_NSm")) {
			ui.answer(network->getMode());
			return;
		}
	
		if (ui.update("_NSWs")) {
			ui.answer(network->getWifiSsid());
			return;
		}
	
		if (ui.update("_NSAs")) {
			ui.answer(network->getApSsid());
			return;
		}
		if (ui.update("_NSAp")) {
			ui.answer(network->getApPass());
			return;
		}
	
		// parse
		if (ui.click("_NSm")) {
			network->setMode(ui.getInt());
			return;
		}
	
		if (ui.form("/_NSW")) {
			char read_ssid[NETWORK_SSID_PASS_SIZE];
			char read_pass[NETWORK_SSID_PASS_SIZE];
	
			ui.copyStr("_NSWs", read_ssid, NETWORK_SSID_PASS_SIZE);
			ui.copyStr("_NSWp", read_pass, NETWORK_SSID_PASS_SIZE);
	
			network->setWifi(read_ssid, read_pass);
			return;
		}
	
		if (ui.click("_NSAs")) {
			String read_string(ui.getString());
			network->setAp(&read_string, NULL);
			
			return;
		}
		if (ui.click("_NSAp")) {
			String read_string(ui.getString());
			network->setAp(NULL, &read_string);
			
			return;
		}
		/* --- NetworkManager --- */

		/* --- MqttManager --- */
		// update
		if (ui.update("_MSwf")) {
			ui.answer(mqtt->getWorkFlag());
			return;
		}

		if (ui.update("_MSSs")) {
			ui.answer(mqtt->getServer());
			return;
		}
		if (ui.update("_MSSp")) {
			ui.answer(mqtt->getPort());
			return;
		}

		if (ui.update("_MSAs")) {
			ui.answer(mqtt->getSsid());
			return;
		}
		if (ui.update("_MSAp")) {
			ui.answer(mqtt->getPass());
			return;
		}

		// parse
		if (ui.click("_MSwf")) {
			mqtt->setWorkFlag(ui.getBool());
			return;
		}

		if (ui.form("/_MSS")) {
			char mqtt_server[MQTT_SERVER_SIZE];
			uint16_t mqtt_port;
	
			ui.copyStr("_MSSs", mqtt_server, MQTT_SERVER_SIZE);
			ui.copyInt("_MSSp", mqtt_port);
	
			mqtt->setServer(mqtt_server, mqtt_port);
			return;
		}
		if (ui.form("/_MSA")) {
			char mqtt_ssid[MQTT_SSID_PASS_SIZE];
			char mqtt_pass[MQTT_SSID_PASS_SIZE];
	
			ui.copyStr("_MSAs", mqtt_ssid, MQTT_SSID_PASS_SIZE);
			ui.copyStr("_MSAp", mqtt_pass, MQTT_SSID_PASS_SIZE);
	
			mqtt->setAccess(mqtt_ssid, mqtt_pass);
			return;
		}
		/* --- MqttManager --- */

		/* --- BlynkManager --- */
		// update
		if (ui.update("_BSwf")) {
			ui.answer(blynk->getWorkFlag());
			return;
		}
		if (ui.update("_BSa")) {
			ui.answer(blynk->getAuth());
			return;
		}

		for (uint8_t i = 0;i < blynk->getLinksCount();i++) {
			if (ui.update(String("_BSLp") + i)) {
				ui.answer(blynk->getLinkPort(i));

				return;
			}
			if (ui.update(String("_BSLe") + i)) {
				char* link_element_code = blynk->getLinkElementCode(i);
				uint8_t index = system->scanElementCodeIndex(&blynk_block.element_codes, link_element_code);
			
				ui.answer(index);
				return;
			}
		}

		// parse
		if (ui.click("_BSwf")) {
			blynk->setWorkFlag(ui.getBool());
			return;
		}
		if (ui.click("_BSa")) {
			blynk->setAuth(ui.getString());
			return;
		}

		if (ui.click("BSLs")) {
			updateBlynkBlock();
			return;
		}
		if (ui.click("_BSLnl")) {
			blynk->addLink();
			return;
		}
		
		for (uint8_t i = 0;i < blynk->getLinksCount();i++) {
			if (ui.click(String("_BSLp") + i)) {
				blynk->setLinkPort(i, ui.getInt());
				
				return;
			}
			if (ui.click(String("_BSLe") + i)) {
				uint8_t index = ui.getInt();

				if (index < blynk_block.element_codes.size()) {
					blynk->setLinkElementCode(i, blynk_block.element_codes[index]);
				}
				
				return;
			}
			if (ui.click(String("_BSLd") + i)) {
				blynk->deleteLink(i);
				return;
			}
		}
		/* --- BlynkManager --- */

		/* --- SensorsManager --- */
		// update
		if (ui.update("_SSrdt")) {
			ui.answer(sensors->getReadDataTime());
			return;
		}

		for (byte i = 0;i < sensors->getDS18B20Count();i++) {
			if (ui.update(String("_SSDn") + i)) {
				ui.answer(sensors->getDS18B20Name(i));

				return;
			}

			if (ui.update(String("_SSDa") + i)) {
				uint8_t* ds18b20_address = sensors->getDS18B20Address(i);
				uint8_t index = sensors->scanDS18B20AddressIndex(&sensors_block.ds18b20_addresses, ds18b20_address);
												
				ui.answer(index);
				return;
			}

			if (ui.update(String("_SSDr") + i)) {
				ui.answer(sensors->getDS18B20Resolution(i));
				return;
			}

			if (ui.update(String("_SSDc") + i)) {
				ui.answer(sensors->getDS18B20Correction(i), 1);
				return;
			}
		}

		// parse
		if (ui.click("_SSrdt")) {
			sensors->setReadDataTime(ui.getInt());
			return;
		}
		if (ui.click("SSDs")) {
			updateSensorsBlock();
			return;
		}
		if (ui.click("_SSDnd")) {
			sensors->addDS18B20();
			return;
		}

		for (byte i = 0;i < sensors->getDS18B20Count();i++) {
			if (ui.click(String("_SSDn") + i)) {
				sensors->setDS18B20Name(i, ui.getString());

				return;
			}

			if (ui.click(String("_SSDa") + i)) {
				uint8_t index = ui.getInt();

				if (index < sensors_block.ds18b20_addresses.size()) {
					sensors->setDS18B20Address(i, sensors_block.ds18b20_addresses[index]);
				}
				
				return;
			}

			if (ui.click(String("_SSDr") + i)) {
				sensors->setDS18B20Resolution(i, ui.getInt());
				return;
			}

			if (ui.click(String("_SSDc") + i)) {
				sensors->setDS18B20Correction(i, ui.getFloat());
				return;
			}

			if (ui.click(String("_SSDd") + i)) {
				sensors->deleteDS18B20(i);
				return;
			}
		}
		/* --- SensorsManager --- */

		/* --- SystemManager --- */
		// update
		if (ui.update("_SSsf")) {
			ui.answer(system->getSleepFlag());
			return;
		}
		if (ui.update("_SSst")) {
			ui.answer(system->getSleepTime());
			return;
		}

		// parse
		if (ui.click("_SSsf")) {
			system->setSleepFlag(ui.getBool());
			return;
		}
		if (ui.click("_SSst")) {
			system->setSleepTime(ui.getInt());
			return;
		}	

		if (ui.click("SSr")) {
			ESP.reset();
		}
		if (ui.click("SSra")) {
			system->resetAll();
		}
		/* --- SystemManager --- */
	});

	updateBlynkBlock();
	updateSensorsBlock();
}

void Web::start() {
	ui.start();
}

void Web::stop() {
	ui.stop();
}

void Web::tick() {
	ui.tick();
}


void Web::setSystemManager(SystemManager* system) {
	this->system = system;
}

bool Web::getStatus() {
	return ui.state();
}


void Web::updateSensorsBlock() {
	SensorsManager* sensors = system->getSensorsManager();
	DynamicArray<String> addresses;

	sensors_block.ds18b20_addresses_string.clear();
	sensors->makeDS18B20AddressList(&sensors_block.ds18b20_addresses, &addresses);

	for (uint8_t i = 0;i < addresses.size();i++) {
		sensors_block.ds18b20_addresses_string += addresses[i];

		if (i != addresses.size() - 1) {
			sensors_block.ds18b20_addresses_string += ',';
		}
	}
}

void Web::updateBlynkBlock() {
	blynk_block.element_codes_string.clear();
	system->makeElementCodesList(&blynk_block.element_codes);

	for (uint8_t i = 0;i < blynk_block.element_codes.size();i++) {
		blynk_block.element_codes_string += blynk_block.element_codes[i];

		if (i != blynk_block.element_codes.size() - 1) {
			blynk_block.element_codes_string += ',';
		}
	}
}