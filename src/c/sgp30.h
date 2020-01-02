#ifndef SGP30
#define SGP30

#include <stdint.h>

// Print function that only prints if DEBUG is defined
#ifdef DEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif
#define debug_print(fd, fmt, ...) \
            do { if (DEBUG_PRINT) fprintf(fd, fmt, __VA_ARGS__); } while (0)

enum Error {
  NO_ERROR,
  ERROR_TIME,           // Timeout (in I2C, due to clock stretching)
  ERROR_DRIVER,         // Driver failed to init
  ERROR_CRC,            // Checksum failed
  ERROR_INVAL,          // Invalid argument
  ERROR_I2C             // I2C driver failed to read or write data
};

// Chip defines
#define SGP30_ADDRESS 0x58

// Set up and tear down SGP30 I2C interface
int SGP30_init(const char *i2c_adaptor);
int SGP30_deinit(void);

// Fetch data from SGP30
int SGP30_measure_air_quality(uint16_t *eCO2_out,
                              uint16_t *TVOC_out);
int SGP30_measure_raw_signals(uint16_t *h2_out,
                              uint16_t *ethanol_out);
int SGP30_get_baseline(uint16_t *eCO2_out,
                       uint16_t *TVOC_out);

// Set data in SGP30
int SGP30_set_baseline(const uint16_t eCO2_in,
                       const uint16_t TVOC_in);
int SGP30_set_humidity(const uint16_t humidity_in);

// Data about SGP30
int SGP30_measure_test(void);
int SGP30_get_feature_set_version(uint16_t *version_out);
int SGP30_get_serial_id(uint64_t *serial_id);
#endif
