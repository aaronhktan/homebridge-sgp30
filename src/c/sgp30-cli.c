#include "sgp30.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void int_handle(int signal) {
  SGP30_deinit();
  exit(0);
}

int main(int argc, char **argv) {
  signal(SIGINT, int_handle);
  
  // Init SGP30
  SGP30_init("/dev/i2c-1");

  usleep(10000);
  
  // Get serial number
  uint64_t serial;
  SGP30_get_serial_id(&serial);
  printf("Serial: 0x%012llx\n", serial);

  // Get feature set version
  uint16_t version;
  SGP30_get_feature_set_version(&version);
  printf("Feature set version: 0x%04x\n", version);

  // Set and get baseline
  // uint16_t eCO2_baseline = 37769;
  // uint16_t TVOC_baseline = 36320;

  // SGP30_set_baseline(eCO2_baseline, TVOC_baseline);
  // SGP30_get_baseline(&eCO2_baseline, &TVOC_baseline);
 
  // printf("Baseline: eCO2: %d, TVOC: %d\n", eCO2_baseline, TVOC_baseline);

  // Measure test
  // int err = SGP30_measure_test();
  // printf("%s\n", err ? "Measure test failed" : "Measure test succeeded"); 

  usleep(1000000);

  uint16_t eCO2, TVOC;
  uint16_t h2, ethanol;
  int index = 0;
  while (++index) {
    SGP30_measure_air_quality(&eCO2, &TVOC);
    printf("eCO2: %d ppm, TVOC: %d ppb\n", eCO2, TVOC);
    SGP30_measure_raw_signals(&h2, &ethanol);
    printf("H2: %d, Ethanol: %d\n", h2, ethanol);

    usleep(1000000);

    if (index > 30) {
      SGP30_get_baseline(&eCO2, &TVOC);
      printf("===BASELINE=== eCO2: %d ppm, TVOC: %d ppb\n", eCO2, TVOC);
      index = 0;
    }
  }

  SGP30_deinit();
}
