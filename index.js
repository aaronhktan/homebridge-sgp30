const SGP30 = require('bindings')('homebridge-sgp30');

const moment = require('moment'); // Time formatting
const mqtt = require('mqtt'); // MQTT client
const os = require('os'); // Hostname

var Service, Characteristic;

var FakeGatoHistoryService;

module.exports = homebridge => {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  FakeGatoHistoryService = require('fakegato-history')(homebridge);

  homebridge.registerAccessory("homebridge-sgp30", "SGP30", SGP30Accessory);
}

// SGP30 constructor
function SGP30Accessory(log, config) {
  // Basic setup from config file
  this.log = log;
  this.displayName = config['name'];
  this.baseline_values = config['baseline'];
  this.enableFakeGato = config['enableFakeGato'] || false;
  this.fakeGatoStoragePath = config['fakeGatoStoragePath'];
  this.enableMQTT = config['enableMQTT'] || false;
  this.mqttConfig = config['mqtt'];

  // Internal variables to keep track of current temperature and humidity
  this._currenteCO2 = null;
  this._currentTVOC = null;

  let informationService = new Service.AccessoryInformation();
  informationService
    .setCharacteristic(Characteristic.Manufacturer, "Sensirion")
    .setCharacteristic(Characteristic.Model, "SGP30")
    .setCharacteristic(Characteristic.SerialNumber, `${os.hostname}-0`);
    .setCharacteristic(Characteristic.FirmwareRevision, require('./package.json').version);

  let airQualityService = new Service.AirQualitySensor();
  airQualityService
    .addCharacteristic(Characteristic.CarbonDioxideLevel)
    .addCharacteristic(Characteristic.VOCDensity);

  // Make these services available to class
  this.informationService = informationService;
  this.airQualityService = airQualityService;

  // Start FakeGato for logging historical data
  if (this.enableFakeGato) {
    this.fakeGatoHistoryCarbonDioxideService = new FakeGatoHistoryService("room", this, {
      storage: 'fs',
      filename: `SGP30-${os.hostname}-0-CarbonDioxide.json`,
      folder: this.fakeGatoStoragePath
    });
    this.fakeGatoHistoryTVOCService = new FakeGatoHistoryService("room", this, {
      storage: 'fs',
      filename: `SGP30-${os.hostname}-0-TVOC.json`,
      folder: this.fakeGatoStoragePath
    });
  }

  // Set up MQTT client
  if (this.enableMQTT) {
    this.setUpMQTT();
  }

  // Periodically update the values
  this.setupSGP30();
  this.refreshData();
  setInterval(() => this.refreshData(), 1000);
}

Object.defineProperty(SGP30Accessory.prototype, "eCO2", {
  set: function(eCO2Reading) {
    if (eCO2Reading > 60000 || eCO2Reading < 0) {
      this.log(`Error: eCO2 reading out of range: ` +
               `Reading: ${eCO2Reading}, Current: ${this._currenteCO2}`);
      return;
    }

    this._currenteCO2 = eCO2Reading;  
    this.airQualityService.getCharacteristic(Characteristic.CarbonDioxideLevel)
      .updateValue(this._currenteCO2);
    if (this.enableFakeGato) {
      this.fakeGatoHistoryCarbonDioxideService.addEntry({
        time: moment().unix(),
        ppm: this._currenteCO2,
      });
    }
    if (this.enableMQTT) {
      this.publishToMQTT(this.eCO2Topic, this._currenteCO2);
    }
  },

  get: function() {
    return this._currenteCO2;
  }
});

Object.defineProperty(SGP30Accessory.prototype, "TVOC", {
  set: function(TVOCReading) {
    if (TVOCReading > 60000 || TVOCReading < 0) {
      this.log(`Error: TVOC reading out of range: ` +
               `Reading: ${TVOCReading}, Current: ${this._currentTVOC}`);
      return;
    }

    this._currentTVOC = TVOCReading;  
    this.airQualityService.getCharacteristic(Characteristic.VOCDensity)
      .updateValue(this._currentTVOC);
    if (this.enableFakeGato) {
      this.fakeGatoHistoryTVOCService.addEntry({
        time: moment().unix(),
        ppm: this._currentTVOC,
      });
    }
    if (this.enableMQTT) {
      this.publishToMQTT(this.TVOCTopic, this._currentTVOC);
    }
  },

  get: function() {
    return this._currentTVOC;
  }
});

// Sets up MQTT client so that we can send data
SGP30Accessory.prototype.setUpMQTT = function() {
  if (!this.enableMQTT) {
    this.log.info("MQTT not enabled");
    return;
  }

  if (!this.mqttConfig) {
    this.log.error("No MQTT config found");
    return;
  }

  this.mqttUrl = this.mqttConfig.url;
  this.eCO2Topic = this.mqttConfig.eCO2Topic || 'SGP30/eCO2';
  this.TVOCTopic = this.mqttConfig.TVOCTopic || 'SGP30/TVOC';

  this.mqttClient = mqtt.connect(this.mqttUrl);
  this.mqttClient.on("connect", () => {
    this.log(`MQTT client connected to ${this.mqttUrl}`);
  });
  this.mqttClient.on("error", (err) => {
    this.log(`MQTT client error: ${err}`);
    client.end();
  });
}

// Sends data to MQTT broker
SGP30Accessory.prototype.publishToMQTT = function(topic, value) {
  if (!this.mqttClient.connected || !topic || !value) {
    this.log.error("MQTT client not connected, or no topic or value for MQTT");
    return;
  }
  this.mqttClient.publish(topic, String(value));
}

// Set up sensor
SGP30Accessory.prototype.setupSGP30() {
  data = SGP30.init();
  if (data.hasOwnProperty('errcode')) {
    this.log(`Error: ${data.errmsg}`);
  }
}

// Get data from the sensor
SGP30Accessory.prototype.refreshData = function() {
  let data;
  data = SGP30.measureAirQuality();

  // If error, set to error state
  if (data.hasOwnProperty('errcode')) {
    this.log(`Error: ${data.errmsg}`);
    // Updating a value with Error class sets status in HomeKit to 'Not responding'
    this.airQualityService.getCharacteristic(Characteristic.CarbonDioxideLevel)
      .updateValue(Error(data.errmsg));
    this.airQualityService.getCharacteristic(Characteristic.VOCDensity)
      .updateValue(Error(data.errmsg));
    return;
  }
  
  // Set temperature and humidity from what we polled
  this.log(`eCO2: ${data.eCO2}, TVOC: ${data.TVOC}`);
  this.eCO2 = data.eCO2;
  this.TVOC = data.TVOC;
}

SGP30Accessory.prototype.getServices = function() {
  return [this.informationService,
          this.temperatureService,
          this.humidityService,
          this.fakeGatoHistoryService];
}
