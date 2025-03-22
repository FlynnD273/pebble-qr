#include "qrcode.h"

#include "qr.h"
#include <pebble.h>
static void save_settings();

#define NUM_BUFS 4
// Needs to be < 1024 to fit 4 strings + 1 more byte for selected index in 4KB
#define BUF_LEN 1023

typedef struct ClaySettings {
  char strings[NUM_BUFS][BUF_LEN];
  uint8_t idx;
  uint8_t num_strings;
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;
static Layer *s_layer;
static int width;
static int height;

static const QRCode EMPTY_QR;
static QRCode qr_code;
static uint8_t *qr_data = NULL;
static GDrawCommandImage *error_image;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void default_settings() {
  for (uint8_t i = 0; i < NUM_BUFS; i++) {
    strncpy(settings.strings[i], "", BUF_LEN);
  }
  strncpy(settings.strings[0], "https://github.com/flynnD273/pebble-qr",
          BUF_LEN);
  strncpy(settings.strings[1], "This is the second QR code", BUF_LEN);
  strncpy(settings.strings[2], "And this, the third", BUF_LEN);
  settings.idx = 0;
  settings.num_strings = 3;
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  if (!qr_data) {
    gdraw_command_image_draw(ctx, error_image, GPoint(0, 0));
    return;
  }
  uint8_t idx = 0;
  // Figure out what index out of the non-empty strings we've selected
  for (uint8_t i = 0; i < settings.idx; i++) {
    if (strlen(settings.strings[i]) > 0) {
      idx++;
    }
  }

  uint8_t module_size =
      MIN(width / (qr_code.size + 2), height / (qr_code.size + 2));

  // Fit the QR code to the round dimensions
#ifdef PBL_ROUND
  module_size = width * 707 / 1000 / (qr_code.size + 2);
#endif
  uint8_t offset_x = (width - module_size * qr_code.size) / 2;
  uint8_t offset_y = (height - module_size * qr_code.size) / 2;

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  // Draw the selection indicator
  if (settings.num_strings > 1) {
#ifdef PBL_RECT
    graphics_fill_rect(ctx,
                       GRect(idx * width / settings.num_strings, 0,
                             width / settings.num_strings, offset_y / 4),
                       0, 0);
    graphics_context_set_stroke_width(ctx, 2);
    for (uint8_t i = 1; i < settings.num_strings; i++) {
      graphics_draw_line(
          ctx, GPoint(i * width / settings.num_strings, 0),
          GPoint(i * width / settings.num_strings, offset_y / 4 - 2));
    }
#else
    uint8_t thickness = 5;
    graphics_context_set_stroke_width(ctx, thickness);
    graphics_draw_arc(
        ctx, GRect(thickness / 2, height / 2, width - thickness, 1),
        GOvalScaleModeFillCircle,
        idx * TRIG_MAX_ANGLE / settings.num_strings / 2 - TRIG_MAX_ANGLE / 4,
        (idx + 1) * TRIG_MAX_ANGLE / settings.num_strings / 2 -
            TRIG_MAX_ANGLE / 4);
    graphics_context_set_stroke_width(ctx, 2);
    for (uint8_t i = 0; i <= settings.num_strings; i++) {
      int16_t sx = cos_lookup(-i * TRIG_MAX_ANGLE / 2 / settings.num_strings) *
                   width / 2 / TRIG_MAX_RATIO;
      int16_t sy = sin_lookup(-i * TRIG_MAX_ANGLE / 2 / settings.num_strings) *
                   width / 2 / TRIG_MAX_RATIO;
      int16_t ex = cos_lookup(-i * TRIG_MAX_ANGLE / 2 / settings.num_strings) *
                   (width / 2 - thickness + 2) / TRIG_MAX_RATIO;
      int16_t ey = sin_lookup(-i * TRIG_MAX_ANGLE / 2 / settings.num_strings) *
                   (width / 2 - thickness + 2) / TRIG_MAX_RATIO;
      graphics_draw_line(ctx, GPoint(sx + width / 2, sy + height / 2),
                         GPoint(ex + width / 2, ey + height / 2));
    }
#endif
  }

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

static void calc_qr() {
  if (settings.num_strings > 0) {
    uint8_t version = qr_get_version(strlen(settings.strings[settings.idx]));
    free(qr_data);
    qr_data = (uint8_t *)calloc(qrcode_getBufferSize(version), 1);
    qrcode_initText(&qr_code, qr_data, version, 0,
                    settings.strings[settings.idx]);
  } else {
    free(qr_data);
    qr_data = NULL;
    qr_code = EMPTY_QR;
  }
  if (s_layer) {
    layer_mark_dirty(s_layer);
  }
}

/**
 * Select the next non-empty string
 */
static void next(uint8_t jump) {
  if (settings.num_strings > 1) {
    settings.idx += jump;
    settings.idx %= NUM_BUFS;
    for (uint8_t i = 0; i < NUM_BUFS; i++) {
      if (strlen(settings.strings[settings.idx]) > 0) {
        break;
      }
      settings.idx += jump;
      settings.idx %= NUM_BUFS;
    }
    calc_qr();
  }
}

static void down_click(ClickRecognizerRef recognizer, void *context) {
  next((uint8_t)NUM_BUFS - 1);
}

static void up_click(ClickRecognizerRef recognizer, void *context) { next(1); }
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
}

/**
 * Load the strings from persistent storage
 */
static void load_settings() {
  if (E_DOES_NOT_EXIST == persist_read_data(0, settings.strings[0], 1)) {
    default_settings();
  } else {
    bool set_idx = false;
    uint8_t buf_num = 0;
    size_t string_byte = 0;
    uint32_t key = 0;
    while (buf_num < NUM_BUFS) {
      if (BUF_LEN - string_byte >= PERSIST_DATA_MAX_LENGTH) {
        persist_read_data(key, settings.strings[buf_num],
                          PERSIST_DATA_MAX_LENGTH);
        if (BUF_LEN - string_byte == PERSIST_DATA_MAX_LENGTH) {
          buf_num++;
          string_byte = 0;
        } else {
          string_byte += PERSIST_DATA_MAX_LENGTH;
        }
        key++;
      } else {
        char data[PERSIST_DATA_MAX_LENGTH];
        persist_read_data(key, data, PERSIST_DATA_MAX_LENGTH);
        key++;
        memcpy(&settings.strings[buf_num][string_byte], data,
               BUF_LEN - string_byte);
        buf_num++;
        if (buf_num < NUM_BUFS) {
          memcpy(settings.strings[buf_num], &data[BUF_LEN - string_byte],
                 PERSIST_DATA_MAX_LENGTH - (BUF_LEN - string_byte));
          string_byte = BUF_LEN - string_byte;
        } else {
          memcpy(&settings.idx, &data[BUF_LEN - string_byte], 1);
          set_idx = true;
        }
      }
    }
    if (!set_idx) {
      persist_read_data(key, &settings.idx, 1);
    }
    settings.num_strings = 0;
    for (uint8_t i = 0; i < NUM_BUFS; i++) {
      if (strlen(settings.strings[i]) > 0) {
        settings.num_strings++;
      }
    }
  }
  calc_qr();
}

/**
 * Save the strings to persistent storage
 */
static void save_settings() {
  // This is cursed. Please don't do this 😭
  uint8_t buf_num = 0;
  size_t string_byte = 0;
  uint32_t key = 0;
  bool set_idx = false;
  while (buf_num < NUM_BUFS) {
    if (BUF_LEN - string_byte >= PERSIST_DATA_MAX_LENGTH) {
      persist_write_data(key, settings.strings[buf_num],
                         PERSIST_DATA_MAX_LENGTH);
      if (BUF_LEN - string_byte == PERSIST_DATA_MAX_LENGTH) {
        buf_num++;
        string_byte = 0;
      } else {
        string_byte += PERSIST_DATA_MAX_LENGTH;
      }
      key++;
    } else {
      char data[PERSIST_DATA_MAX_LENGTH];
      memcpy(data, &settings.strings[buf_num][string_byte],
             BUF_LEN - string_byte);
      buf_num++;
      if (buf_num < NUM_BUFS) {
        memcpy(&data[BUF_LEN - string_byte], settings.strings[buf_num],
               PERSIST_DATA_MAX_LENGTH - (BUF_LEN - string_byte));
        string_byte = BUF_LEN - string_byte;
      } else {
        memcpy(&data[BUF_LEN - string_byte], &settings.idx, 1);
        set_idx = true;
      }
      persist_write_data(key, data, PERSIST_DATA_MAX_LENGTH);
      key++;
    }
  }
  if (!set_idx) {
    persist_write_data(key, &settings.idx, 1);
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  settings.num_strings = 0;
  for (uint8_t i = 0; i < NUM_BUFS; i++) {
    Tuple *string_t = dict_find(iter, i);
    if (string_t) {
      strncpy(settings.strings[i], string_t->value->cstring, BUF_LEN);
      if (strlen(settings.strings[i]) > 0) {
        settings.num_strings++;
      }
    }
  }
  // Make sure we land on a valid selection
  next(1);
  next(NUM_BUFS - 1);
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
  app_message_open(MIN(BUF_LEN * NUM_BUFS, app_message_inbox_size_maximum()),
                   0);
}

int main(void) {
  init();
  app_event_loop();
}
