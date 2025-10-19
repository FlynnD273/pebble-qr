#include "qrcode.h"

#include "qr-version.h"
#include <pebble.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static void save_settings();

#define BUF_LEN 4096
#define ALL_ERROR (error < ONE_LENGTH)

typedef struct ClaySettings {
  char strings[BUF_LEN];
  size_t displayed_index;
  size_t num_strings;
} ClaySettings;

static ClaySettings settings;

static size_t mem_offset = 0;

static Window *s_main_window;
static Layer *s_layer;
static TextLayer *s_text_layer;
static int width;
static int height;

static const QRCode EMPTY_QR;
static QRCode qr_code;
static uint8_t *qr_data = NULL;
static GDrawCommandImage *error_image;
typedef enum {
  ALL_EMPTY = 0,
  ALL_LENGTH,
  ONE_LENGTH,
  NONE,
} ErrorCode;
static ErrorCode error = NONE;
static char *error_texts[] = {
    "No QR code",
    "Too many QRs",
    "Text too long",
};
#define LABEL_LEN 18
static char *label;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void default_settings() {
  memset(settings.strings, 0, BUF_LEN);
  settings.num_strings = 6;
  char *default_strings[] = {"https://github.com/flynnD273/pebble-qr",
                             "Source code",
                             "This is the second QR code",
                             "QR 2",
                             "And this, the third",
                             "QR 3"};
  size_t buf_idx = 0;
  for (uint8_t i = 0; i < settings.num_strings; i++) {
    size_t len = strlen(default_strings[i]);
    strncpy(&settings.strings[buf_idx], default_strings[i], len);
    buf_idx += strlen(default_strings[i]) + 1;
  }
  settings.displayed_index = 0;
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  // One of the error codes for the state of the app
  if (ALL_ERROR) {
    gdraw_command_image_draw(ctx, error_image, GPoint(0, 0));
    text_layer_set_text(s_text_layer, error_texts[error]);
    return;
  }
  uint8_t module_size =
      MIN(width / (qr_code.size + 2), height / (qr_code.size + 2));

  // Fit the QR code to the round dimensions
#ifdef PBL_ROUND
  module_size = width * 707 / 1000 / (qr_code.size + 2);
#endif

  // Module size is too small for the screen :(
  if (module_size < 1) {
    error = ONE_LENGTH;
  }

  uint8_t offset_x = (width - module_size * qr_code.size) / 2;
  uint8_t offset_y = (height - module_size * qr_code.size) / 2;

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  size_t qr_count = settings.num_strings / 2;
  size_t qr_index = settings.displayed_index / 2;
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Index %d/%d", qr_index, qr_count);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "displayed index %d/%d",
          settings.displayed_index, settings.num_strings);
#endif
  // Draw the selection indicator
  if (qr_count > 1) {
#ifdef PBL_RECT
    graphics_fill_rect(
        ctx, GRect(qr_index * width / qr_count, 0, width / qr_count, 4), 0, 0);
    graphics_context_set_stroke_width(ctx, 2);
    for (uint8_t i = 1; i < qr_count; i++) {
      graphics_draw_line(ctx, GPoint(i * width / qr_count, 0),
                         GPoint(i * width / qr_count, 4 - 2));
    }
#else
    uint8_t thickness = 5;
    graphics_context_set_stroke_width(ctx, thickness);
    graphics_draw_arc(
        ctx, GRect(thickness / 2, height / 2, width - thickness, 1),
        GOvalScaleModeFillCircle,
        qr_index / 2 * TRIG_MAX_ANGLE / qr_count - TRIG_MAX_ANGLE / 4,
        (qr_index / 2 + 1) * TRIG_MAX_ANGLE / qr_count - TRIG_MAX_ANGLE / 4);
    graphics_context_set_stroke_width(ctx, 2);
    for (uint8_t i = 0; i <= qr_count; i++) {
      int16_t sx = cos_lookup(-i * TRIG_MAX_ANGLE / 2 / qr_count) * width / 2 /
                   TRIG_MAX_RATIO;
      int16_t sy = sin_lookup(-i * TRIG_MAX_ANGLE / 2 / qr_count) * width / 2 /
                   TRIG_MAX_RATIO;
      int16_t ex = cos_lookup(-i * TRIG_MAX_ANGLE / 2 / qr_count) *
                   (width / 2 - thickness + 2) / TRIG_MAX_RATIO;
      int16_t ey = sin_lookup(-i * TRIG_MAX_ANGLE / 2 / qr_count) *
                   (width / 2 - thickness + 2) / TRIG_MAX_RATIO;
      graphics_draw_line(ctx, GPoint(sx + width / 2, sy + height / 2),
                         GPoint(ex + width / 2, ey + height / 2));
    }
#endif
  }
  // One of the error codes for the state of the specific string
  if (error != NONE) {
    gdraw_command_image_draw(ctx, error_image, GPoint(0, 0));
    text_layer_set_text(s_text_layer, error_texts[error]);
    return;
  }
  text_layer_set_text(s_text_layer, label);

  // Draw the QR code, reading one byte at a time
  size_t i = 0;
  uint8_t val = 0;
  uint8_t shift = 0;
  for (uint8_t y = 0; y < qr_code.size; y++) {
    for (uint8_t x = 0; x < qr_code.size; x++) {
      if (shift == 0) {
        val = qr_code.modules[i++];
        shift = 8;
      }

      if ((val >> --shift) & 1) {
        graphics_fill_rect(ctx,
                           GRect(x * module_size + offset_x,
                                 y * module_size + offset_y, module_size,
                                 module_size),
                           0, 0);
      }
    }
  }
}

static bool starts_with(char *string, char *prefix) {
  size_t len = strlen(prefix);
  if (strlen(string) < len) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if (string[i] != prefix[i]) {
      return false;
    }
  }
  return true;
}

static void calc_qr() {
  if (error != ALL_LENGTH) {
    if (settings.num_strings > 0) {
      error = NONE;
      mem_offset = 0;
      size_t len = strlen(settings.strings);
      size_t i = 0;
      while (i < settings.displayed_index || len == 0) {
        if (len > 0) {
          i++;
        }
        mem_offset += len + 1;
        len = strlen(&settings.strings[mem_offset]);
      }
      uint8_t version = qr_get_version(len);
      if (version == 0) {
        free(qr_data);
        qr_data = NULL;
        qr_code = EMPTY_QR;
        error = ONE_LENGTH;
      } else {
        free(qr_data);
        uint16_t qr_len = qrcode_getBufferSize(version);
        qr_data = (uint8_t *)calloc(qr_len, 1);
        char *curr_str = &settings.strings[mem_offset];
        if (qr_data == NULL ||
            qrcode_initText(&qr_code, qr_data, version, 0, curr_str) != 0) {
          free(qr_data);
          qr_data = NULL;
          qr_code = EMPTY_QR;
          error = ONE_LENGTH;
        } else {
          if (label == NULL) {
            label = calloc(LABEL_LEN, 1);
          }
          if (strcmp(curr_str + strlen(curr_str) + 1, " ") == 0) {
            size_t offset = 0;
            if (starts_with(curr_str, "https://")) {
              offset = 8;
            } else if (starts_with(curr_str, "http://")) {
              offset = 7;
            }
            strncpy(label, curr_str + offset, LABEL_LEN - 1);
          } else {
            strncpy(label, curr_str + strlen(curr_str) + 1, LABEL_LEN - 1);
#ifdef DEBUG
            APP_LOG(APP_LOG_LEVEL_DEBUG, "label: %s",
                    curr_str + strlen(curr_str) + 1);
#endif
          }
        }
      }
    } else {
      free(qr_data);
      qr_data = NULL;
      qr_code = EMPTY_QR;
      error = ALL_EMPTY;
    }
  }
  if (s_layer) {
    layer_mark_dirty(s_layer);
  }
}

/**
 * Select the next non-empty string
 */
static void next(int8_t jump) {
  if (settings.num_strings > 1) {
    settings.displayed_index += jump + settings.num_strings;
    settings.displayed_index %= settings.num_strings;
    calc_qr();
  }
}

static void down_click(ClickRecognizerRef recognizer, void *context) {
  next(2);
}
static void up_click(ClickRecognizerRef recognizer, void *context) { next(-2); }
static void back_click(ClickRecognizerRef recognizer, void *context) {
  save_settings();
  window_stack_pop(true);
}

static void config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click);
}

static void main_window_load(Window *window) {
  error_image = gdraw_command_image_create_with_resource(RESOURCE_ID_ERROR);
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_bounds(window_layer);
  s_layer = layer_create(frame);
  width = frame.size.w;
  height = frame.size.h;
  s_text_layer = text_layer_create(GRect(0, height - 26, width, 24));
  text_layer_set_background_color(s_text_layer, GColorWhite);
  text_layer_set_font(s_text_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  layer_add_child(window_layer, s_layer);
  layer_set_update_proc(s_layer, frame_redraw);
  window_set_click_config_provider(window,
                                   (ClickConfigProvider)config_provider);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_layer);
  window_destroy(s_main_window);
  gdraw_command_image_destroy(error_image);
  free(qr_data);
  free(label);
}

/**
 * Load the strings from persistent storage
 */
static void load_settings() {
  if (E_DOES_NOT_EXIST == persist_read_data(0, settings.strings, 1)) {
    default_settings();
  } else {
    uint32_t key = 1;
    size_t buf_idx = 0;
    persist_read_data(0, &settings.displayed_index, sizeof(size_t));
    while (buf_idx < BUF_LEN) {
      persist_read_data(key, &settings.strings[buf_idx],
                        PERSIST_DATA_MAX_LENGTH);
      key++;
      buf_idx += PERSIST_DATA_MAX_LENGTH;
    }
    settings.num_strings = 0;
    buf_idx = 0;
    while (buf_idx < BUF_LEN) {
      size_t len = strlen(&settings.strings[buf_idx]);
      if (len > 0) {
        settings.num_strings++;
      }
      buf_idx += len + 1;
    }
  }
  calc_qr();
}

/**
 * Save the strings to persistent storage
 */
static void save_settings() {
  size_t buf_idx = 0;
  uint32_t key = 1;
  persist_write_data(0, &settings.displayed_index, 1);
  while (buf_idx < BUF_LEN) {
    persist_write_data(key, &settings.strings[buf_idx],
                       PERSIST_DATA_MAX_LENGTH);
    buf_idx += PERSIST_DATA_MAX_LENGTH;
    key++;
  }
}

static void get_string_key(DictionaryIterator *iter, uint32_t key,
                           size_t *buf_idx) {
  Tuple *string_t = dict_find(iter, key);
  if (string_t) {
    size_t offset = 0;
    while (offset < string_t->length) {
      size_t len = strlen(string_t->value->cstring + offset);
      if (len > BUF_LEN - *buf_idx) {
        len = BUF_LEN - *buf_idx;
      }
      strncpy(&settings.strings[*buf_idx], string_t->value->cstring + offset,
              len);
#ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "value %d is: %s", settings.num_strings + 1,
              &settings.strings[*buf_idx]);
#endif
      if (len > 0) {
        settings.num_strings++;
      }
      (*buf_idx) += len + 1;
      offset += len + 1;
    }
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  settings.num_strings = 0;
  size_t buf_idx = 0;
  memset(settings.strings, 0, BUF_LEN);
  get_string_key(iter, MESSAGE_KEY_STRING_0, &buf_idx);
  if (settings.strings[BUF_LEN - 1] != '\0') {
    error = ALL_LENGTH;
  } else {
    error = NONE;
  }
  settings.displayed_index %= settings.num_strings;
  save_settings();
  calc_qr();
}

static void init() {
  load_settings();
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), 0);
}

int main(void) {
  init();
  app_event_loop();
}
