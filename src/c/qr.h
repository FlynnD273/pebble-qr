#include <pebble.h>

#define MAX(a, b) (a > b) ? (a) : (b)
#define MIN(a, b) (a < b) ? (a) : (b)
typedef struct array {
  uint8_t *data;
  size_t len;
  size_t size;
} array;
typedef struct point {
  uint8_t x;
  uint8_t y;
} point;
/**
 * Heavily taken from
 * https://www.thonky.com/qr-code-tutorial/
 * https://dev.to/maxart2501/let-s-develop-a-qr-code-generator-part-iii-error-correction-1kbm
 */
array gen_qr(char *str);
uint8_t idx_to_size(uint8_t size);
uint8_t get_version_idx(size_t str_len);
array new_array(size_t len, uint8_t size);
array free_array(array arr);

uint8_t get_char_count(uint8_t size);
uint8_t multiply(uint8_t a, uint8_t b);
uint8_t divide(uint8_t a, uint8_t b);
array poly_multiply(array a, array b);
array get_remainder(array a, array b);
array get_generator_poly(uint8_t degree);
array get_ec(array data, uint8_t total_codewords);
array interleave(array data, array *group1, array *group2, uint8_t version_idx);
void draw_rect(array cells, uint8_t x, uint8_t y, uint8_t width, uint8_t height,
               uint8_t val);
void draw_finder(array cells, uint8_t x, uint8_t y);
void draw_alignment(array cells, uint8_t x, uint8_t y);
uint8_t get_module(array cells, uint8_t x, uint8_t y);
void set_module(array cells, uint8_t x, uint8_t y, uint8_t val);
point get_position(uint16_t size, uint16_t idx);
array *gen_masks(array cells, array reserved_mask);
uint16_t eval_board(array cells);
uint8_t bin_len(uint16_t val);
