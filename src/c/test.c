#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <stdio.h>


int main(const int argc, const char **argv) {
  int file_i2c;

  char *filename = "/dev/i2c-1";
  if ((file_i2c = open(filename, O_RDWR)) < 0) {
    fprintf(stderr, "Failed to open I2C bus\n");
    return -1;
  }

  int addr = 0x58;
  if (ioctl(file_i2c, I2C_SLAVE, addr) < 0) {
    fprintf(stderr, "Failed to open I2C bus\n");
    return -1;
  }

  unsigned char buffer[60] = { 0 };
  buffer[0] = 0x36;
  buffer[1] = 0x82;
  int length = 2;
  if (write(file_i2c, buffer, length) != length) {
    fprintf(stderr, "Failed to write to I2C bus\n"); 
    return -1;
  }

  length = 9;
  if (read(file_i2c, buffer, length) != length) {
    fprintf(stderr, "Failed to read from I2C bus\n");
    return -1;
  }
  for (int i = 0; i < 9; i++) {
    printf("%02x", buffer[i]);
  }
  printf("\n");

  return 0;
}
