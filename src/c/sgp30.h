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

// Chip defines
#define SGP30_ADDRESS 0x58

// Set up and tear down SGP30 I2C interface
int SGP30_init(void);
int SGP30_deinit(void);

// Fetch data from SGP30
int SGP30_read_data(const int max_retries,
                    uint16_t *eCO2_out,
                    uint16_t *TVOC_out);

#endif
