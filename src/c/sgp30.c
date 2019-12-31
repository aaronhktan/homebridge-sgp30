#include "sgp30.h"

#include "bcm2835.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_626;

static uint8_t crc_generate(char *input, int input_len) {
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

  return crc;
}

static int crc_compare(char *input, int input_len, uint8_t expected) {
  uint8_t crc = crc_generate(input, input_len);
  debug_print(stdout, "CRC: %02x, Expected: %02x\n", crc, expected);
  return !(crc == expected);
}

// Convenience function to write a command then read
// Write write_len bytes from array write_bytes to I2C bus
// Then read read_len bytes from I2C bus into read_bytes
// Then check CRC
static int i2c_write_delay_read(char *write_bytes, int write_len,
                                char *read_bytes, int read_len,
                                int delay_ms) {
  if (!write_bytes || !read_bytes) {
    return ERROR_INVAL;
  }

  if (bcm2835_i2c_write(write_bytes, write_len)) {
    return ERROR_I2C;
  }

  usleep(delay_ms * 1000);

  if (bcm2835_i2c_read(read_bytes, read_len)) {
    return ERROR_I2C;
  }

  debug_print(stdout, "%s", "Read: ");
  for (int i = 0; i < 6; i++) {
    debug_print(stdout, "%02x", read_bytes[i]);
  }
  debug_print(stdout, "%s", "\n");

  // Each word is 2 bytes + 1 byte CRC in the SGP30
  for (int i = 0; i < read_len; i += 3) {
    if (crc_compare(read_bytes+i, 2, read_bytes[i+2])) {
      debug_print(stdout, "%s", "CRC error!");
      return ERROR_CRC;
    }
  }

  return NO_ERROR;
}

// Convenience wrapper for i2c_write_delay_read;
// defaults to 10ms delay
static int i2c_write_read(char *write_bytes, int write_len,
                          char *read_bytes, int read_len) {
  return i2c_write_delay_read(write_bytes, write_len, read_bytes, read_len, 10);
}

int SGP30_init(void) {
  bcm2835_init();
  bcm2835_i2c_begin();
  bcm2835_i2c_setSlaveAddress(SGP30_ADDRESS);
  bcm2835_i2c_setClockDivider(clk_div);
  
  char init_cmd[2] = { 0x20, 0x03 };
  bcm2835_i2c_write(init_cmd, 2);
  return 0;
}

int SGP30_deinit(void) {
  bcm2835_i2c_end();
  bcm2835_close();
  return 0;
}

// Functions to get data from SGP30
int SGP30_measure_air_quality(uint16_t *eCO2_out,
                              uint16_t *TVOC_out) {
  char measure_cmd[2] = { 0x20, 0x08 };
  char cmd_size = 2;

  char response[6];
  char response_size = 6;
  memset(response, 0, response_size);

  // Measuring air quality takes 10ms < t < 12ms
  int err = i2c_write_delay_read(measure_cmd, cmd_size, response, response_size, 12);
  if (err) { return err; }

  *eCO2_out = (response[0] << 8) | response[1]; 
  *TVOC_out = (response[3] << 8) | response[4];
  
  return err;
}

int SGP30_measure_raw_signals(uint16_t *h2_out,
                              uint16_t *ethanol_out) {
  char raw_cmd[2] = { 0x20, 0x50 };
  int cmd_size = 2;

  char response[6];
  int response_size = 6;
  memset(response, 0, response_size);

  // Measuring raw signals requires 20ms < t < 25ms
  int err = i2c_write_delay_read(raw_cmd, cmd_size, response, response_size, 25);
  if (err) { return err; }

  *h2_out = (response[0] << 8) | response[1];
  *ethanol_out = (response[3] << 8) | response[4];

  return NO_ERROR;
}

int SGP30_get_baseline(uint16_t *eCO2_out,
                              uint16_t *TVOC_out) {
  char baseline_cmd[2] = { 0x20, 0x15 };
  int cmd_size = 2;

  char response[6];
  int response_size = 6;
  memset(response, 0, response_size);

  int err = i2c_write_read(baseline_cmd, cmd_size, response, response_size);
  if (err) { return err; }

  *eCO2_out = (response[0] << 8) | response[1];
  *TVOC_out = (response[3] << 8) | response[4];

  crc_compare(response, 2, response[2]);
  crc_compare(response + 3, 2, response[5]);
  return 0;
}

// Functions to set data in SGP30
int SGP30_set_baseline(const uint16_t eCO2_in,  
                       const uint16_t TVOC_in) {
  char eCO2[2] = { eCO2_in >> 8, eCO2_in & 0xFF }; 
  uint8_t eCO2_CRC = crc_generate(eCO2, 2);

  char TVOC[2] = { TVOC_in >> 8, TVOC_in & 0xFF };
  uint8_t TVOC_CRC = crc_generate(TVOC, 2);

  // Setting baseline:
  // - 2 bytes for command
  // - 3 bytes for TVOC word + CRC
  // - 3 bytes for eCO2 word + CRC
  char baseline_cmd[8] = { 0x20, 0x1E };
  memcpy(baseline_cmd + 2, TVOC, 2);
  baseline_cmd[4] = TVOC_CRC;
  memcpy(baseline_cmd + 5, eCO2, 2);
  baseline_cmd[7] = eCO2_CRC; 
  int baseline_size = 8;

  if (bcm2835_i2c_write(baseline_cmd, baseline_size)) {
    return ERROR_I2C;
  }
  return 0;
}

int SGP30_set_humidity(uint16_t humidity_in) {
  char humidity[2] = { humidity_in >> 8, humidity_in & 0xFF };
  uint8_t humidity_CRC = crc_generate(humidity, 2);

  char humidity_cmd[5] = { 0x20, 0x61 };
  memcpy(humidity_cmd + 2, humidity, 2);
  humidity_cmd[4] = humidity_CRC;
  int humidity_size = 5;

  if (bcm2835_i2c_write(humidity_cmd, humidity_size)) {
    return ERROR_I2C;
  }
  return 0;
}

// Data about SGP30
int SGP30_measure_test(void) {
  // This causes the chip to run an on-chip self-test
  // The chip returns the pattern 0xD400 if test is successful.
  char measure_cmd[2] = { 0x20, 0x32 };
  int cmd_size = 2;

  char response[3];
  char response_size = 3;

  // The test takes minimum 200ms and maximum 220ms, so delay 220ms
  i2c_write_delay_read(measure_cmd, cmd_size, response, response_size, 220);
  
  return !(response[0] == 0xD4 && response[1] == 0x00);
}

int SGP30_get_feature_set_version(uint16_t *version_out) {
  char feature_cmd[2] = { 0x20, 0x2F };
  int cmd_size = 2;

  char response[3];
  char response_size = 3;

  int err = i2c_write_delay_read(feature_cmd, cmd_size, response, response_size, 3);
  if (err) { return err; }

  *version_out = (response[0] << 8) | response[1];
  return NO_ERROR; 
}

int SGP30_get_serial_id(uint64_t *serial_id) {
  char serial_cmd[2] = { 0x36, 0x82 };
  int cmd_size = 2;

  char response[9];
  int response_size = 9;

  int err = i2c_write_delay_read(serial_cmd, cmd_size, response, response_size, 1);
  if (err) { return err; }

  *serial_id = ((uint64_t)response[0] << 40) | ((uint64_t)response[1] << 32)
               | ((uint64_t)response[3] << 24) | ((uint64_t)response[4] << 16) 
               | ((uint64_t)response[6] << 8) | (uint64_t)response[7];
  return NO_ERROR;
}
 
