#include "qr.h"
#include "qr-consts.h"
bool init = true;

array gen_qr(char *str) {
  if (init) {
    init = false;
    for (uint16_t exponent = 1, value = 1; exponent < 256; exponent++) {
      value = value > 127 ? ((value << 1) ^ 285) : value << 1;
      log_table[value] = exponent % 255;
      exp_table[exponent % 255] = value;
    }
  }
  size_t str_len = strlen(str);
  uint8_t version_idx = get_version_idx(str_len);
  uint8_t size = idx_to_size(version_idx);
  if (!size) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "String too long :(");
    return new_array(0, 0);
  }
  uint16_t len = ((uint16_t)size * size + 7) / 8;
  array cells = new_array(len, size);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "generate data");

  uint8_t mode = 0b0111;
  uint8_t char_indicator_len = get_char_count(size);
  uint16_t req_byte_len = required_bytes[version_idx];
  array data = new_array(req_byte_len, 0);
  if (char_indicator_len == 2) {
    // 0b_mode1234_--------_--------
    data.data[0] = (mode << 4) | ((str_len >> 12) & 0x0F);
    // 0b_mode----_12345678_--------
    data.data[1] = (str_len >> 4) & 0xFF;
    // 0b_mode----_--------_1234----
    data.data[2] = (str_len << 4) & 0xF0;
  } else {
    // 0b_mode1234_--------
    data.data[0] = (mode << 4) | ((str_len >> 4) & 0x0F);
    // 0b_--------_1234----
    data.data[1] = (str_len << 4) & 0xF0;
  }
  for (size_t i = 0; i < str_len; i++) {
    data.data[i + char_indicator_len] |= (str[i] >> 4 & 0x0F);
    data.data[i + char_indicator_len + 1] = (str[i] << 4) & 0xF0;
  }
  for (size_t i = 0; i < data.len - str_len - char_indicator_len - 1; i++) {
    if (i % 2) {
      data.data[i + str_len + char_indicator_len + 1] = 0b00010001;
    } else {
      data.data[i + str_len + char_indicator_len + 1] = 0b11101100;
    }
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "required number of bytes: %d", req_byte_len);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "ec groups");
  array *ec_group1 =
      (array *)calloc(num_blocks_in_group1[version_idx], sizeof(array));
  for (uint8_t i = 0; i < num_blocks_in_group1[version_idx]; i++) {
    uint8_t num_codewords = num_codewords_in_block1[version_idx];
    array block;
    block.data = data.data + i * num_codewords;
    block.len = num_codewords;
    ec_group1[i] =
        get_ec(block, num_codewords + ec_codewords_per_block[version_idx]);
  }

  array *ec_group2 =
      (array *)calloc(num_blocks_in_group2[version_idx], sizeof(array));
  for (uint8_t i = 0; i < num_blocks_in_group2[version_idx]; i++) {
    uint8_t num_codewords = num_codewords_in_block2[version_idx];
    array block;
    block.data =
        data.data + (i + num_blocks_in_group1[version_idx]) * num_codewords;
    block.len = num_codewords;
    ec_group2[i] = get_ec(block, ec_codewords_per_block[version_idx]);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "ec size: %d",
          num_blocks_in_group1[version_idx] *
                  ec_codewords_per_block[version_idx] +
              num_blocks_in_group2[version_idx] *
                  ec_codewords_per_block[version_idx]);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "interleave");
  array ec_data = interleave(data, ec_group1, ec_group2, version_idx);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%x", ec_data.data[0]);

  for (size_t i = 0; i < num_blocks_in_group1[version_idx]; i++) {
    ec_group1[i] = free_array(ec_group1[i]);
  }
  free(ec_group1);
  for (size_t i = 0; i < num_blocks_in_group2[version_idx]; i++) {
    ec_group2[i] = free_array(ec_group2[i]);
  }
  free(ec_group2);

  array reserved_mask = new_array(len, size);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "draw structure");

  draw_finder(cells, 0, 0);
  draw_rect(reserved_mask, 0, 0, 9, 9, 1);
  draw_finder(cells, size - 7, 0);
  draw_rect(reserved_mask, size - 8, 0, 8, 9, 1);
  draw_finder(cells, 0, size - 7);
  draw_rect(reserved_mask, 0, size - 8, 9, 8, 1);
  for (uint8_t x = 8; x < size - 6; x += 2) {
    set_module(cells, x, 6, 1);
    set_module(reserved_mask, x, 6, 1);
    set_module(reserved_mask, x + 1, 6, 1);
  }
  for (uint8_t y = 8; y < size - 6; y += 2) {
    set_module(cells, 6, y, 1);
    set_module(reserved_mask, 6, y, 1);
    set_module(reserved_mask, 6, y + 1, 1);
  }
  set_module(cells, 8, size - 7, 1);
  if (version_idx > 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "drawing alignments...");
    uint8_t curr_version_idx = 1;
    uint8_t start_idx = 0;
    uint8_t num_coords = 2;
    while (curr_version_idx < version_idx) {
      num_coords = 1;
      while (alignment_coords[++start_idx] != 6) {
        num_coords++;
      }
      curr_version_idx++;
    }
    for (uint8_t y = 0; y < num_coords; y++) {
      for (uint8_t x = 0; x < num_coords; x++) {
        if ((x == 0 && y == 0) || (x == 0 && y == num_coords - 1) ||
            (x == num_coords - 1 && y == 0)) {
          continue;
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Alignment drawn");
        draw_alignment(cells, alignment_coords[start_idx + x] - 2,
                       alignment_coords[start_idx + y] - 2);
        draw_rect(reserved_mask, alignment_coords[start_idx + x] - 2,
                  alignment_coords[start_idx + y] - 2, 5, 5, 1);
      }
    }
  }
  if (version_idx > 7) {
    draw_rect(reserved_mask, size - 11, 0, 3, 6, 1);
    draw_rect(reserved_mask, 0, size - 11, 6, 3, 1);
  }

  int n = 0;
  for (uint8_t y = 0; y < size; y++) {
    for (uint8_t x = 0; x < size; x++) {
      if (get_module(reserved_mask, x, y)) {
        n++;
      }
    }
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "write data %d, with %d free tiles in code",
          ec_data.len, (size * size - n) / 8);
  size_t pos_idx = 0;
  for (size_t i = 0; i < ec_data.len * 8; i++) {
    point p = get_position(size, pos_idx++);
    while (get_module(reserved_mask, p.x, p.y)) {
      p = get_position(size, pos_idx++);
    }
    if ((ec_data.data[i / 8] >> (7 - (i % 8))) & 1) {
      set_module(cells, p.x, p.y, 1);
    }
  }

  if (version_idx >= 6) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "version str");
    uint32_t version_str = version_strs[version_idx - 6];

    APP_LOG(APP_LOG_LEVEL_DEBUG, "write str");
    uint8_t i = 0;
    for (uint8_t long_i = 0; long_i < size; long_i++) {
      for (uint8_t short_i = 0; short_i < size; short_i++) {
        uint8_t val = (version_str >> (17 - i++)) & 1;
        if (val) {
          set_module(cells, long_i, size - 11 + short_i, 1);
          set_module(cells, size - 11 + short_i, long_i, 1);
        }
      }
    }
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Generate masks");
  array *masks = gen_masks(cells, reserved_mask);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done with masks");
  uint16_t scores[8];
  for (uint8_t i = 0; i < 8; i++) {
    scores[i] = eval_board(masks[i]);
  }
  uint16_t min_score = scores[0];
  uint8_t min_idx = 0;
  for (uint8_t i = 1; i < 8; i++) {
    if (min_score > scores[i]) {
      min_idx = i;
      min_score = scores[i];
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Chose mask %d", min_idx);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Free cells");
  cells = free_array(cells);
  cells = masks[min_idx];
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Free reserved");
  reserved_mask = free_array(reserved_mask);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "free masks");
  for (uint8_t i = 0; i < 8; i++) {
    if (i != min_idx) {
      masks[i] = free_array(masks[i]);
    }
  }
  free(masks);

  return cells;
}

uint8_t bin_len(uint16_t val) {
  uint8_t len = 0;
  while (val > 0) {
    len++;
    val = val >> 1;
  }
  return len;
}

array *gen_masks(array cells, array reserved_mask) {
  array *masks = (array *)calloc(8, sizeof(array));
  for (uint8_t i = 0; i < 8; i++) {
    masks[i].data = (uint8_t *)calloc(cells.len, 1);
    masks[i].len = cells.len;
    masks[i].size = cells.size;
    for (uint8_t col = 0; col < cells.size; col++) {
      for (uint8_t row = 0; row < cells.size; row++) {
        uint8_t val;
        switch (i) {
        case 0:
          val = (row + col) % 2;
          break;
        case 1:
          val = row % 2;
          break;
        case 2:
          val = col % 3;
          break;
        case 3:
          val = (row + col) % 3;
          break;
        case 4:
          val = (row / 2 + col / 3) % 2;
          break;
        case 5:
          val = ((row * col) % 2) + ((row * col) % 3);
          break;
        case 6:
          val = (((row * col) % 2) + ((row * col) % 3)) % 2;
          break;
        case 7:
          val = (((row + col) % 2) + ((row * col) % 3)) % 2;
          break;
        default:
          APP_LOG(APP_LOG_LEVEL_DEBUG, "???");
          val = 0;
        }
        if (val > 4) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", val);
        }
        set_module(masks[i], col, row, val == 0);
      }
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "apply data");
  for (size_t i = 0; i < cells.len; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      masks[j].data[i] &= ~reserved_mask.data[i];
      masks[j].data[i] ^= cells.data[i];
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Write format string");
  for (uint8_t i = 0; i < 8; i++) {
    uint16_t format_str = format_strs[i];
    for (uint8_t j = 0; j < 15; j++) {
      uint8_t val = (format_str >> (14 - j)) & 1;
      if (j < 7) {
        if (j < 6) {
          set_module(masks[i], j, 8, val);
        } else {
          set_module(masks[i], j + 1, 8, val);
        }
        set_module(masks[i], 8, cells.size - j - 1, val);
      } else {
        uint8_t k = j - 7;
        if (j < 9) {
          set_module(masks[i], 8, 8 - k, val);
        } else {
          set_module(masks[i], 8, 7 - k, val);
        }
        set_module(masks[i], cells.size - 8 + k, 8, val);
      }
    }
  }
  return masks;
}

uint16_t eval_board(array cells) {
  uint16_t score = 0;
  uint16_t num_dark = 0;

  for (uint8_t y = 0; y < cells.size; y++) {
    uint8_t streak = 0;
    for (uint8_t x = 0; x < cells.size; x++) {
      if (get_module(cells, x, y)) {
        num_dark++;
        streak++;
      } else {
        if (streak > 5) {
          score += 3 + streak - 5;
          streak = 0;
        }
      }
    }
    if (streak > 5) {
      score += 3 + streak - 5;
      streak = 0;
    }
  }
  for (uint8_t x = 0; x < cells.size; x++) {
    uint8_t streak = 0;
    for (uint8_t y = 0; y < cells.size; y++) {
      if (get_module(cells, x, y)) {
        streak++;
      } else {
        if (streak > 5) {
          score += 3 + streak - 5;
          streak = 0;
        }
      }
    }
    if (streak > 5) {
      score += 3 + streak - 5;
      streak = 0;
    }
  }

  for (uint8_t x = 0; x < cells.size - 1; x++) {
    for (uint8_t y = 0; y < cells.size - 1; y++) {
      if (get_module(cells, x, y) && get_module(cells, x + 1, y) &&
          get_module(cells, x, y + 1) && get_module(cells, x + 1, y + 1)) {
        score += 3;
      }
    }
  }

  uint16_t pattern1 = 0b10111010000;
  uint16_t pattern2 = 0b00001011101;
  for (uint8_t y = 0; y < cells.size; y++) {
    for (uint8_t x = 0; x < cells.size - 11; x++) {
      uint16_t curr_pattern = 0;
      for (uint8_t i = 0; i < 11; i++) {
        curr_pattern |= get_module(cells, x + i, y) << (10 - i);
      }
      if (!(pattern1 ^ curr_pattern) || !(pattern2 ^ curr_pattern)) {
        score += 40;
      }
    }
  }
  for (uint8_t y = 0; y < cells.size - 11; y++) {
    for (uint8_t x = 0; x < cells.size; x++) {
      uint16_t curr_pattern = 0;
      for (uint8_t i = 0; i < 11; i++) {
        curr_pattern |= get_module(cells, x, y + i) << (10 - i);
      }
      if (!(pattern1 ^ curr_pattern) || !(pattern2 ^ curr_pattern)) {
        score += 40;
      }
    }
  }

  uint8_t percent = 100 * num_dark / cells.size / cells.size;
  uint8_t prev = percent - (percent % 10);
  uint8_t next = percent + (10 - (percent % 10));
  prev = MAX(50 - prev, prev - 50);
  next = MAX(50 - next, next - 50);
  prev /= 5;
  next /= 5;
  score += MIN(prev, next) * 10;

  return score;
}

point get_position(uint16_t size, uint16_t idx) {
  point p;
  p.x = size - (idx / 2 / size) * 2 - 2;
  if ((idx / 2 / size) % 2) {
    p.y = (idx / 2) % size;
  } else {
    p.y = size - ((idx / 2) % size) - 1;
  }
  if (p.x < 6) {
    p.x--;
  }
  if (!(idx % 2)) {
    p.x++;
  }
  return p;
}

void draw_finder(array cells, uint8_t x, uint8_t y) {
  for (uint8_t cy = 0; cy < 7; cy++) {
    for (uint8_t cx = 0; cx < 7; cx++) {
      uint8_t val = 1;
      if (!(cx == 0 || cx == 6 || cy == 0 || cy == 6) &&
          (cx == 1 || cx == 5 || cy == 1 || cy == 5)) {
        val = 0;
      }
      set_module(cells, x + cx, y + cy, val);
    }
  }
}

void draw_alignment(array cells, uint8_t x, uint8_t y) {
  for (uint8_t cy = 0; cy < 5; cy++) {
    for (uint8_t cx = 0; cx < 5; cx++) {
      uint8_t val = 1;
      if (!(cx == 0 || cx == 4 || cy == 0 || cy == 4) &&
          (cx == 1 || cx == 3 || cy == 1 || cy == 3)) {
        val = 0;
      }
      set_module(cells, x + cx, y + cy, val);
    }
  }
}

void draw_rect(array cells, uint8_t x, uint8_t y, uint8_t width, uint8_t height,
               uint8_t val) {
  for (uint8_t cy = 0; cy < height; cy++) {
    for (uint8_t cx = 0; cx < width; cx++) {
      set_module(cells, x + cx, y + cy, val);
    }
  }
}

uint8_t get_version_idx(size_t str_len) {
  size_t len = sizeof(qr_sizes) / sizeof(uint16_t);
  for (size_t i = 0; i < len; i++) {
    if (qr_sizes[i] > str_len) {
      return i;
    }
  }
  return 0;
}

uint8_t get_char_count(uint8_t size) {
  if (size < 53) {
    return 1;
  }
  return 2;
}

uint8_t idx_to_size(uint8_t idx) { return idx * 4 + 21; }

uint8_t multiply(uint8_t a, uint8_t b) {
  return a && b ? exp_table[(log_table[a] + log_table[b]) % 255] : 0;
}

uint8_t divide(uint8_t a, uint8_t b) {
  return exp_table[(log_table[a] + log_table[b] * 254) % 255];
}

array poly_multiply(array a, array b) {
  array coeffs = new_array(a.len + b.len - 1, 0);

  for (uint16_t index = 0; index < coeffs.len; index++) {
    uint8_t coeff = 0;
    for (uint8_t p1index = 0; p1index <= index; p1index++) {
      uint8_t p2index = index - p1index;
      coeff ^= multiply(a.data[p1index], b.data[p2index]);
    }
    coeffs.data[index] = coeff;
  }
  return coeffs;
}

array get_remainder(array dividend, array divisor) {
  uint8_t quotientLength = dividend.len - divisor.len + 1;
  // Let's just say that the dividend is the rest right away
  uint8_t rest_start = 0;
  array rest = new_array(dividend.len, 0);
  memcpy(rest.data, dividend.data, dividend.len);
  for (uint8_t count = 0; count < quotientLength; count++) {
    // If the first term is 0, we can just skip this iteration
    if (rest.data[rest_start]) {
      array factor = new_array(1, 0);
      factor.data[0] = divide(rest.data[0], divisor.data[0]);
      array subtr = poly_multiply(divisor, factor);
      for (uint8_t i = ++rest_start; i < dividend.len; i++) {
        rest.data[i] = rest.data[i] ^ subtr.data[i - rest_start];
      }
      subtr = free_array(subtr);
      factor = free_array(factor);
    } else {
      rest_start++;
    }
  }
  return rest;
}

array get_generator_poly(uint8_t degree) {
  array last_poly = new_array(1, 0);
  last_poly.data[0] = 1;
  for (uint8_t index = 0; index < degree; index++) {
    array b = new_array(2, 0);
    b.data[0] = 1;
    b.data[1] = exp_table[index];
    array tmp = poly_multiply(last_poly, b);
    b = free_array(b);
    last_poly = free_array(last_poly);
    last_poly = tmp;
  }
  return last_poly;
}

array get_ec(array data, uint8_t total_codewords) {
  uint8_t degree = total_codewords - data.len;
  array messagePoly = new_array(total_codewords, 0);
  memcpy(messagePoly.data, data.data, data.len);
  array gen_poly = get_generator_poly(degree);
  array ret = get_remainder(messagePoly, gen_poly);
  messagePoly = free_array(messagePoly);
  gen_poly = free_array(gen_poly);
  return ret;
}

array interleave(array data, array *group1, array *group2,
                 uint8_t version_idx) {
  uint8_t group1_block_count = num_blocks_in_group1[version_idx];
  uint8_t group1_block_size = num_codewords_in_block1[version_idx];
  uint8_t ec_count_per_block = ec_codewords_per_block[version_idx];
  uint8_t group2_block_count = num_blocks_in_group2[version_idx];
  uint8_t group2_block_size = num_codewords_in_block2[version_idx];

  size_t data_group1_size = group1_block_size * group1_block_count;
  size_t ec_group1_size = ec_count_per_block * group1_block_count;
  size_t data_group2_size = group2_block_size * group2_block_count;
  size_t ec_group2_size = ec_count_per_block * group2_block_count;

  array arr = new_array(
      data_group1_size + ec_group1_size + data_group2_size + ec_group2_size, 0);
  // Group 1
  for (size_t i = 0; i < data_group1_size; i++) {
    uint8_t block = i % group1_block_count;
    uint8_t block_word_idx = i / group1_block_count;
    arr.data[i] = data.data[block * group1_block_size + block_word_idx];
  }
  for (size_t i = 0; i < ec_group1_size; i++) {
    uint8_t block = i % group1_block_count;
    uint8_t block_word_idx = i / group1_block_count;
    arr.data[i + data_group1_size] = group1[block].data[block_word_idx];
  }
  // Group 2
  for (size_t i = 0; i < data_group2_size; i++) {
    uint8_t block = i % group2_block_count;
    uint8_t block_word_idx = i / group2_block_count;
    arr.data[i + data_group1_size + ec_group1_size] =
        data.data[block * group2_block_size + block_word_idx];
  }
  for (size_t i = 0; i < ec_group2_size; i++) {
    uint8_t block = i % group2_block_count;
    uint8_t block_word_idx = i / group2_block_count;
    arr.data[i + data_group1_size + ec_group1_size + data_group2_size] =
        group2[block].data[block_word_idx];
  }

  return arr;
}

uint8_t get_module(array cells, uint8_t x, uint8_t y) {
  size_t idx = y * cells.size + x;
  size_t actual_idx = idx / 8;
  size_t shift = 7 - (idx % 8);
  if (actual_idx >= cells.len) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "GET OUT OF BOUNDS %d, %d", x, y);
    return 0;
  }
  return (cells.data[actual_idx] >> shift) & 1;
}

void set_module(array cells, uint8_t x, uint8_t y, uint8_t val) {
  size_t idx = y * cells.size + x;
  size_t actual_idx = idx / 8;
  size_t shift = 7 - (idx % 8);
  if (actual_idx >= cells.len) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "SET OUT OF BOUNDS %d, %d", x, y);
    return;
  }
  cells.data[actual_idx] =
      (cells.data[actual_idx] & ~(1 << shift)) | ((val & 1) << shift);
}

array new_array(size_t len, uint8_t size) {
  array arr;
  arr.len = len;
  arr.size = size;
  arr.data = (uint8_t *)calloc(arr.len, 1);
  return arr;
}

array free_array(array arr) {
  free(arr.data);
  arr.data = NULL;
  arr.len = 0;
  arr.size = 0;
  return arr;
}
