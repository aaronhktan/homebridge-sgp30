#include "sgp30.h"

#include "bcm2835.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_626;

static void int_handle(int signal) {
  SGP30_deinit();
  exit(0);
}

static int crc_compare(char *input, int input_len, uint8_t expected) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < input_len; i++) {
    crc ^= input[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
    }
  }

  debug_print(stdout, "CRC: %02x, Expected: %02x\n", crc, expected);
  return crc == expected;
}

int SGP30_init(void) {
  bcm2835_init();
  bcm2835_i2c_begin();
  bcm2835_i2c_setSlaveAddress(SGP30_ADDRESS);
  bcm2835_i2c_setClockDivider(clk_div);
  
  char id_cmd[3] = { 0x36, 0x82 };
  bcm2835_i2c_write(id_cmd, 2);
  bcm2835_delay(1);
  char response[6];
  bcm2835_i2c_read(response, 6);
  debug_print(stdout, "%s", "Serial ID: ");
  for (int i = 0; i < 6; i++) {
    debug_print(stdout, "%02x", response[i]);
  }
  debug_print(stdout, "%s", "\n");

  char init_cmd[2] = { 0x20, 0x03 };
  bcm2835_i2c_write(init_cmd, 2);
  return 0;
}

int SGP30_deinit(void) {
  bcm2835_i2c_end();
  bcm2835_close();
  return 0;
}

int SGP30_read_data(const int max_retries,
                    uint16_t *eCO2_out,
                    uint16_t *TVOC_out) {
  char measure_cmd[2] = { 0x20, 0x08 };
  bcm2835_i2c_write(measure_cmd, 2);

  usleep(10000);

  char response[6];
  bcm2835_i2c_read(response, 6);

  debug_print(stdout, "%s", "Read: ");
  for (int i = 0; i < 6; i++) {
    debug_print(stdout, "%02x", response[i]);
  }
  debug_print(stdout, "%s", "\n");

  *eCO2_out = (response[0] << 8) | response[1]; 
  *TVOC_out = (response[3] << 8) | response[4];

  crc_compare(response, 2, response[2]);
  crc_compare(response + 3, 2, response[5]); 
  
  return 0;
}

int main(int argc, char **argv) {
  signal(SIGINT, int_handle);  

  SGP30_init();
  
  usleep(1000000);

  uint16_t eCO2, TVOC;
  while (1) {
    SGP30_read_data(50, &eCO2, &TVOC);
    printf("eCO2: %d ppm, TVOC: %d ppb\n", eCO2, TVOC);
    usleep(1000000);
  }

  SGP30_deinit();
}
