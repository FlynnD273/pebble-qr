#include "qrcode.h"

#include "qr.h"
#include <pebble.h>

#define SETTINGS_KEY 1
#define NUM_BUFS 4
#define BUF_LEN 1024

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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void default_settings() {
  for (uint8_t i = 0; i < NUM_BUFS; i++) {
    strncpy(settings.strings[i], "", BUF_LEN);
  }
  strncpy(settings.strings[0], "Hello, world!", BUF_LEN);
  strncpy(settings.strings[1], "Hello world", BUF_LEN);
  settings.idx = 0;
  settings.num_strings = 2;
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  if (!qr_data) {
    return;
  }
  uint8_t idx = 1;
  for (uint8_t i = 0; i < settings.idx; i++) {
    if (strlen(settings.strings[settings.idx]) > 0) {
      idx++;
    }
  }
  size_t i = 0;
  uint8_t val = 0;
  uint8_t shift = 0;
  uint8_t module_size =
      MIN(width / (qr_code.size + 2), height / (qr_code.size + 2));

#ifdef PBL_ROUND
  module_size = module_size * 707 / 1000;
#endif
  uint8_t offset_x = (width - module_size * qr_code.size) / 2;
  uint8_t offset_y = (height - module_size * qr_code.size) / 2;

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx,
                     GRect((idx - 1) * width / settings.num_strings, 0,
                           width / settings.num_strings, offset_y / 4),
                     0, 0);
  graphics_context_set_stroke_width(ctx, 2);
  for (uint8_t i = 1; i < settings.num_strings; i++) {
    graphics_draw_line(
        ctx, GPoint(i * width / settings.num_strings, 0),
        GPoint(i * width / settings.num_strings, offset_y / 4 - 2));
  }

  for (uint8_t y = 0; y < qr_code.size; y++) {
    for (uint8_t x = 0; x < qr_code.size; x++) {
      if (shift == 0) {
        val = qr_code.modules[i++];
        shift = 8;
      }
      graphics_context_set_fill_color(
          ctx, ((val >> --shift) & 1) ? GColorBlack : GColorWhite);
      graphics_fill_rect(ctx,
                         GRect(x * module_size + offset_x,
                               y * module_size + offset_y, module_size,
                               module_size),
                         0, 0);
    }
  }
}

static void calc_qr() {
  uint8_t version = qr_get_version(strlen(settings.strings[settings.idx]));
  free(qr_data);
  qr_data = (uint8_t *)calloc(qrcode_getBufferSize(version), 1);
  qrcode_initText(&qr_code, qr_data, version, 0,
                  settings.strings[settings.idx]);
  if (s_layer) {
    layer_mark_dirty(s_layer);
  }
}

static void next(uint8_t jump) {
  if (settings.num_strings > 1) {
    settings.idx += jump;
    settings.idx %= NUM_BUFS;
    while (strlen(settings.strings[settings.idx]) == 0) {
      settings.idx += jump;
      settings.idx %= NUM_BUFS;
    }
    calc_qr();
  }
}

static void down_click(ClickRecognizerRef recognizer, void *context) {
  next((uint8_t)BUF_LEN - 1);
}
static void up_click(ClickRecognizerRef recognizer, void *context) { next(1); }

static void config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
}

static void main_window_load(Window *window) {
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
  free(qr_data);
}

static void load_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  calc_qr();
}

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
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
