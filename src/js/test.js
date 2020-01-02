const SGP30 = require('bindings')('homebridge-sgp30');

function measureAirQuality() {
  console.log(SGP30.measureAirQuality());
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function test() {
  SGP30.init();
  console.log(SGP30.getSerialID());

  for (i = 0; i < 30; i++) {
    console.log(SGP30.measureAirQuality());
    await sleep(1000);
  }

  SGP30.deinit();
}

test();

