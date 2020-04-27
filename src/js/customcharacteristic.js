const inherits = require('util').inherits;

var Service, Characteristic;

module.exports = (homebridge) => {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;

  var CustomCharacteristic = {};

  // Need to add two custom characteristic for CO2 PPM to show historical graphs in Eve app
  // See https://github.com/skrollme/homebridge-eveatmo/issues/1
  // and https://github.com/simont77/fakegato-history/issues/65
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

  CustomCharacteristic.EveAirQualityUnknown = function() {
    Characteristic.call(this, 'Unknown Eve Characteristic', 'E863F132-079E-48FF-8F27-9C2605A29F52');
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

  return CustomCharacteristic;
}

