const SGP30 = require('bindings')('homebridge-sgp30');
const inherits = require('util').inherits;

const moment = require('moment'); // Time formatting
const mqtt = require('mqtt'); // MQTT client
const os = require('os'); // Hostname

var Service, Characteristic;
var CustomCharacteristic = {};
var FakeGatoHistoryService;

module.exports = homebridge => {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  FakeGatoHistoryService = require('fakegato-history')(homebridge);

  // Need to add two custom characteristic for CO2 PPM to show historical graphs in Eve app
  // See https://github.com/skrollme/homebridge-eveatmo/issues/1
  // and https://github.com/simont77/fakegato-history/issues/65
  // Custom characteristic for Air Quality PPM to show up in Eve app
  CustomCharacteristic.EveAirQuality = function() {
    Characteristic.call(this, 'Eve Air Quality', 'E863F10B-079E-48FF-8F27-9C2605A29F52');
    this.setProps({
      format: Characteristic.Formats.UINT16,
      unit: "ppm",
      maxValue: 5000,
      minValue: 0,
      minStep: 1,
      perms: [ Characteristic.Perms.READ, Characteristic.Perms.NOTIFY ],
    });
    this.value = this.getDefaultValue();
  }
  CustomCharacteristic.EveAirQuality.UUID = 'E863F10B-079E-48FF-8F27-9C2605A29F52';
  inherits(CustomCharacteristic.EveAirQuality, Characteristic);

  // Custom characteristic for Air Quality to show up in Eve app
  CustomCharacteristic.EveAirQualityUnknown = function() {
    Characteristic.call(this, 'Eve Air Quality', 'E863F132-079E-48FF-8F27-9C2605A29F52');
    this.setProps({
      format: Characteristic.Formats.FLOAT,
      maxValue: 5000,
      minValue: 0,
      minStep: 1,
      perms: [ Characteristic.Perms.READ, Characteristic.Perms.NOTIFY ],
    });
    this.value = this.getDefaultValue();
  }
  CustomCharacteristic.EveAirQualityUnknown.UUID = 'E863F132-079E-48FF-8F27-9C2605A29F52';
  inherits(CustomCharacteristic.EveAirQualityUnknown, Characteristic);

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

  // Internal variables to keep track of current eCO2 and TVOC
  this._currenteCO2 = null;
  this._currentTVOC = null;
  this._eCO2Samples = [];
  this._TVOCSamples = [];
  this._eCO2CumSum = 0;
  this._TVOCCumSum = 0;
  this._eCO2Counter = 0;
  this._TVOCCounter = 0;

  let informationService = new Service.AccessoryInformation();
  informationService
    .setCharacteristic(Characteristic.Manufacturer, "Sensirion")
    .setCharacteristic(Characteristic.Model, "SGP30")
    .setCharacteristic(Characteristic.SerialNumber, `${os.hostname}-0`)
    .setCharacteristic(Characteristic.FirmwareRevision, require('./package.json').version);

  let airQualityService = new Service.AirQualitySensor();
  airQualityService.addCharacteristic(Characteristic.CarbonDioxideLevel)
  airQualityService.addCharacteristic(Characteristic.VOCDensity);
  airQualityService.addCharacteristic(CustomCharacteristic.EveAirQuality);
  airQualityService.addCharacteristic(CustomCharacteristic.EveAirQualityUnknown);

  // Make these services available to class
  this.informationService = informationService;
  this.airQualityService = airQualityService;

  // Start FakeGato for logging historical data
  if (this.enableFakeGato) {
    this.fakeGatoHistoryCarbonDioxideService = new FakeGatoHistoryService("room", this, {
      storage: 'fs',
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

    // Calculate running average of eCO2 over the last 30 samples
    this._eCO2Counter++;
    if (this._eCO2Samples.length == 30) {
      let firstSample = this._eCO2Samples.shift();
      this._eCO2CumSum -= firstSample;
    }
    this._eCO2Samples.push(eCO2Reading);
    this._eCO2CumSum += eCO2Reading;

    // Update current eCO2 value, and publish to MQTT/FakeGato once every 30 seconds
    if (this._eCO2Counter == 30) {
      this._eCO2Counter = 0;
      this._currenteCO2 = this._eCO2CumSum / 30;
      this.log(`eCO2: ${this._currenteCO2}`);

      this.airQualityService.getCharacteristic(Characteristic.CarbonDioxideLevel)
        .updateValue(this._currenteCO2);
      this.airQualityService.getCharacteristic(CustomCharacteristic.EveAirQuality)
        .updateValue(this._currenteCO2);
      if (this._currenteCO2 <= 600) {
        this.airQualityService.getCharacteristic(Characteristic.AirQuality)
          .updateValue(1);
      } else if (this._currenteCO2 > 600 && this._currenteCO2 <= 800) {
        this.airQualityService.getCharacteristic(Characteristic.AirQuality)
          .updateValue(2);
      } else if (this._currenteCO2 > 800 && this._currenteCO2 <= 1000) {
        this.airQualityService.getCharacteristic(Characteristic.AirQuality)
          .updateValue(3);
      } else if (this._currenteCO2 > 1000 && this._currenteCO2 <= 1500) {
        this.airQualityService.getCharacteristic(Characteristic.AirQuality)
          .updateValue(4);
      } else {
        this.airQualityService.getCharacteristic(Characteristic.AirQuality)
          .updateValue(5);
      }
 
      if (this.enableFakeGato) {
        this.fakeGatoHistoryCarbonDioxideService.addEntry({
          time: moment().unix(),
          ppm: this._currenteCO2 + 50, // Eve has a 450ppm floor for graphing, but SGP30's is 400ppm; add 50ppm to compensate 
        });
      }
 
      if (this.enableMQTT) {
        this.publishToMQTT(this.eCO2Topic, this._currenteCO2);
      }
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

    // Calculate running average of TVOC over the last 30 samples
    this._TVOCCounter++;
    if (this._TVOCSamples.length == 30) {
      let firstSample = this._TVOCSamples.shift();
      this._TVOCCumSum -= firstSample;
    }
    this._TVOCSamples.push(TVOCReading);
    this._TVOCCumSum += TVOCReading;

    if (this._TVOCCounter == 30) {
      this._TVOCCounter = 0;
      this._currentTVOC = this._TVOCCumSum / 30;
      this.log(`TVOC: ${this._currentTVOC}`);

      this.airQualityService.getCharacteristic(Characteristic.VOCDensity)
       .updateValue(this._currentTVOC);
 
      if (this.enableMQTT) {
        this.publishToMQTT(this.TVOCTopic, this._currentTVOC);
      }
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
  if (!this.mqttClient.connected || !topic) {
    this.log.error("MQTT client not connected, or no topic or value for MQTT");
    return;
  }
  this.mqttClient.publish(topic, String(value));
}

// Set up sensor
SGP30Accessory.prototype.setupSGP30 = function() {
  data = SGP30.init();
  if (data.hasOwnProperty('errcode')) {
    this.log(`Error: ${data.errmsg}`);
  }
}

// Get data from the sensor
SGP30Accessory.prototype.refreshData = function() {
  let data;
  data = SGP30.measureAirQuality();

  if (data.hasOwnProperty('errcode')) {
    this.log(`Error: ${data.errmsg}`);
    // Updating a value with Error class sets status in HomeKit to 'Not responding'
    this.airQualityService.getCharacteristic(Characteristic.CarbonDioxideLevel)
      .updateValue(Error(data.errmsg));
    this.airQualityService.getCharacteristic(Characteristic.VOCDensity)
      .updateValue(Error(data.errmsg));
    return;
  }
  
  // Set eCO2 and TVOC from what we polled
  this.log.debug(`Read: eCO2: ${data.eCO2}ppm, TVOC: ${data.TVOC}ppb`); 
  this.eCO2 = data.eCO2;
  this.TVOC = data.TVOC;
}

SGP30Accessory.prototype.getServices = function() {
  return [this.informationService,
          this.airQualityService,
          this.fakeGatoHistoryCarbonDioxideService];
}
