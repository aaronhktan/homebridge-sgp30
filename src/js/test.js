const SGP30 = require('bindings')('homebridge-sgp30');

SGP30.init();
setInterval(() => { console.log(SGP30.measureAirQuality()); }, 1000);
SGP30.deinit();
