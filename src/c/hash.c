#include <pebble.h>
#define FNV_PRIME 0x00000100000001b3
#define FNV_OFFSET 0xcbf29ce484222325

uint64_t hash(uint8_t *arr, size_t len) {
  uint64_t hash = FNV_OFFSET;
  for (size_t i = len; i; i--) {
    hash ^= arr[i] << (i % (sizeof(hash) - 1));
    hash *= FNV_PRIME;
  }
  return hash;
}
