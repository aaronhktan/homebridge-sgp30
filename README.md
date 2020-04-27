# Homebridge Plugin for SGP30

This is a Homebridge plugin for SGP30 eCO2 and TVOC sensor, working on the Raspberry Pi 3.

It uses read() and write() syscalls on the I2C devices exposed by Linux's filesystem.

## Configuration

| Field name           | Description                                                   | Type / Unit    | Default value       | Required? |
| -------------------- |:--------------------------------------------------------------|:--------------:|:-------------------:|:---------:|
| name                 | Name of the accessory                                         | string         | —                   | Y         |
| enableFakeGato       | Enable storing data in Eve Home app                           | bool           | false               | N         |
| fakeGatoStoragePath  | Path to store data for Eve Home app                           | string         | (fakeGato default)  | N         |
| enableMQTT           | Enable sending data to MQTT server                            | bool           | false               | N         |
| mqttConfig           | Object containing some config for MQTT                        | object         | —                   | N         |
| showTemperatureTile  | Enable showing temperature tile in Home that represents TVOC  | bool           | false               | N         |

If `showTemperatureTile` is `true`, a temperature tile will show up in Home with the TVOC in ppb scaled by a factor of ten (e.g. 25.2 C = 252ppb).

The mqttConfig object is **only required if enableMQTT is true**, and is defined as follows:

| Field name  | Description                                      | Type / Unit  | Default value       | Required? |
| ----------- |:-------------------------------------------------|:------------:|:-------------------:|:---------:|
| url         | URL of the MQTT server, must start with mqtt://  | string       | —                   | Y         |
| eCO2Topic   | MQTT topic to which eCO2 data is sent            | string       | SGP30/eCO2          | N         |
| TVOCTopic   | MQTT topic to which TVOC data is sent            | string       | SGP30/TVOC          | N         |

### Example Configuration

```
{
  "bridge": {
    "name": "Homebridge",
    "username": "XXXX",
    "port": XXXX
  },

  "accessories": [
    {
      "accessory": "SGP30",
      "name": "SGP30",
      "enableFakeGato": true,
      "enableMQTT": true,
      "mqtt": {
         "url": "mqtt://192.168.0.38",
         "eCO2Topic": "sgp30/eCO2",
         "TVOCTopic": "sgp30/TVOC"
      }
    },
  ]
}
```

## Project Layout

- All things required by Node are located at the root of the repository (i.e. package.json and index.js).
- The rest of the code is in `src`, further split up by language.
  - `c` contains the C code that runs on the device to communicate with the sensor. It also contains a simple C program that communicates with the sensor.
  - `binding` contains the C++ code using node-addon-api to communicate between C and the Node.js runtime.
  - `js` contains a simple project that tests that the binding between C/Node.js is correctly working. It also contains a characteristic that enables Eve to keep a historical graph of air quality (i.e. eCO2).
