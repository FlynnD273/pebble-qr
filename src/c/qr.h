#include <pebble.h>

#define MAX(a, b) (a > b) ? (a) : (b)
#define MIN(a, b) (a < b) ? (a) : (b)
typedef struct uint8_array {
  uint8_t *data;
  size_t len;
} uint8_array;
typedef struct point {
  uint8_t x;
  uint8_t y;
} point;
/**
 * Heavily taken from
 * https://www.thonky.com/qr-code-tutorial/
 * https://dev.to/maxart2501/let-s-develop-a-qr-code-generator-part-iii-error-correction-1kbm
 */
uint8_t gen_qr(char *str, uint8_t **cells);
uint8_t size_to_idx(uint8_t size);

uint8_t get_size(char *str, size_t str_len);
uint8_t get_char_count(uint8_t size);
uint8_t multiply(uint8_t a, uint8_t b);
uint8_t divide(uint8_t a, uint8_t b);
uint8_t *poly_multiply(uint8_t *a, uint8_t a_len, uint8_t *b, uint8_t b_len);
uint8_t *poly_divide(uint8_t *a, uint8_t a_len, uint8_t *b, uint8_t b_len);
uint8_t *get_generator_poly(uint8_t degree);
uint8_t *get_ec(uint8_t *data, uint8_t data_len, uint8_t codewords);
uint8_array interleave(uint8_t *data, uint8_t **group1, uint8_t **group2,
                       uint8_t version_idx);
void draw_rect(uint8_t *cells, uint8_t cells_size, uint8_t x, uint8_t y,
               uint8_t width, uint8_t height, uint8_t val);
void draw_finder(uint8_t *cells, uint8_t cells_size, uint8_t x, uint8_t y);
void draw_alignment(uint8_t *cells, uint8_t cells_size, uint8_t x, uint8_t y);
uint8_t get_module(uint8_t *cells, uint8_t size, uint8_t x, uint8_t y);
void set_module(uint8_t *cells, uint8_t size, uint8_t x, uint8_t y,
                uint8_t val);
point get_position(uint8_t size, uint16_t idx);
uint8_t *gen_masks(uint8_t *cells, uint8_t *reserved_mask, uint8_t size);
uint16_t eval_board(uint8_t *cells, uint8_t size);
uint8_t bin_len(uint16_t val);
