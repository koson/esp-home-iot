
homeassistant/sensor/espnode-993102/power/config
{"unit_of_measurement":"W","state_class":"measurement","device_class":"power","value_template":"{{ value_json.power }}","state_topic":"home-iot-node/espnode-993102/power","json_attributes_topic":"home-iot-node/espnode-993102/power","name":"Instant power","unique_id":"espnode-993102_power","device":{"identifiers":["espnode-993102"],"name":"power_meter","sw_version":"1.0","model":"latonita.power","manufacturer":"Anton Viktorov"},"availability_topic":"home-iot-node/espnode-993102/$online"}

homeassistant/sensor/espnode-993102/temperature/config
{"unit_of_measurement":"°C","state_class":"measurement","device_class":"temperature","value_template":"{{ value_json.temperature }}","state_topic":"home-iot-node/espnode-993102/temperature","json_attributes_topic":"home-iot-node/espnode-993102/temperature","name":"Temperature","unique_id":"espnode-993102_temperature","device":{"identifiers":["espnode-993102"],"name":"power_meter","sw_version":"1.0","model":"latonita.power","manufacturer":"Anton Viktorov"},"availability_topic":"home-iot-node/espnode-993102/$online"}

homeassistant/sensor/espnode-993102/humidity/config
{"unit_of_measurement":"%","state_class":"measurement","device_class":"humidity","value_template":"{{ value_json.humidity }}","state_topic":"home-iot-node/espnode-993102/temperature","json_attributes_topic":"home-iot-node/espnode-993102/temperature","name":"Humidity","unique_id":"espnode-993102_humidity","device":{"identifiers":["espnode-993102"],"name":"power_meter","sw_version":"1.0","model":"latonita.power","manufacturer":"Anton Viktorov"},"availability_topic":"home-iot-node/espnode-993102/$online"}


homeassistant/sensor/espnode-993102/energy-t1/config
{"unit_of_measurement":"kWh","state_class":"measurement","device_class":"energy","value_template":
"{{value.split(';')[0]}}","state_topic":"home-iot-node/espnode-993102/readings","json_attributes_topic":"home-iot-node/espnode-993102/readings","name":"Energy T1","unique_id":"espnode-993102_energy_t1","device":{"identifiers":["espnode-993102"],"name":"power_meter","sw_version":"1.0","model":"latonita.power","manufacturer":"Anton Viktorov"},"availability_topic":"home-iot-node/espnode-993102/$online"}


homeassistant/sensor/espnode-993102/energy-t2/config
{"unit_of_measurement":"kWh","state_class":"measurement","device_class":"energy","value_template":
"{{value.split(';')[1]}}","state_topic":"home-iot-node/espnode-993102/readings","json_attributes_topic":"home-iot-node/espnode-993102/readings","name":"Energy T2","unique_id":"espnode-993102_energy_t2","device":{"identifiers":["espnode-993102"],"name":"power_meter","sw_version":"1.0","model":"latonita.power","manufacturer":"Anton Viktorov"},"availability_topic":"home-iot-node/espnode-993102/$online"}
