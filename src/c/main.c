#include "qrcode.h"

#include "qr.h"
#include <pebble.h>

#define SETTINGS_KEY 1
#define BUF_LEN 1024

typedef struct ClaySettings {
  char string1[BUF_LEN];
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;
static Layer *s_layer;
static int width;
static int height;

static QRCode qr_code;
static uint8_t *qr_data;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void default_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Defaults");
  strncpy(settings.string1, "Hello, world!", BUF_LEN);
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  size_t i = 0;
  uint8_t val = 0;
  uint8_t shift = 0;
  uint8_t module_size =
      MIN(width / (qr_code.size + 2), height / (qr_code.size + 2));
  uint8_t offset_x = (width - module_size * qr_code.size) / 2;
  uint8_t offset_y = (height - module_size * qr_code.size) / 2;
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(offset_x - module_size, offset_y - module_size,
                                (qr_code.size + 2) * module_size,
                                (qr_code.size + 2) * module_size));

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

static void new_frame(void *data) {
  uint8_t version = qr_get_version(strlen(settings.string1));
  qr_data = (uint8_t *)calloc(qrcode_getBufferSize(version), 1);
  qrcode_initText(&qr_code, qr_data, version, 0, settings.string1);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "QR size: %i", qr_code.size);
  layer_mark_dirty(s_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_bounds(window_layer);
  s_layer = layer_create(frame);
  width = frame.size.w;
  height = frame.size.h;

  layer_add_child(window_layer, s_layer);
  layer_set_update_proc(s_layer, frame_redraw);
  app_timer_register(500, new_frame, NULL);
  layer_mark_dirty(s_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_layer);
  window_destroy(s_main_window);
  /* free(cells); */
}

static void load_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *string1_t = dict_find(iter, MESSAGE_KEY_STRING_1);
  if (string1_t) {
    strncpy(settings.string1, string1_t->value->cstring, BUF_LEN);
  }
  save_settings();
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
  app_message_open(128, 128);
}

int main(void) {
  init();
  app_event_loop();
}
