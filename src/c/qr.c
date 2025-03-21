#include "qr.h"
#include "qr-consts.h"

uint8_t qr_get_version(size_t str_len) {
  size_t len = sizeof(qr_sizes) / sizeof(uint16_t);
  for (size_t i = 0; i < len; i++) {
    if (qr_sizes[i] > str_len) {
      return i + 1;
    }
  }
  return 0;
}
