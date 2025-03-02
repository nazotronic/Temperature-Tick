/*
 * Project: Temperature Tick
 *
 * Author: Vereshchynskyi Nazar
 * Email: verechnazar12@gmail.com
 * Version: 1.0.0
 * Date: 02.03.2025
 */

#include "data.h"

SystemManager systemManager;

void setup() {
	systemManager.begin();
}

void loop() {
	systemManager.tick();
}