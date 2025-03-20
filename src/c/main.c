#include "qr.h"
#include <pebble.h>

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  char *string1;
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;
static Layer *s_layer;
static int width;
static int height;

static int qr_size;

static uint8_t *cells;

static void default_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Defaults");
  settings.string1 = (char *)malloc(13);
  strcpy(settings.string1, "Hello, world");
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "REDRAW");
  size_t i = 0;
  uint8_t val = 0;
  uint8_t shift = 0;
  uint8_t cell_size = MIN(width / (qr_size + 2), height / (qr_size + 2));
  uint8_t offset_x = (width - cell_size * qr_size) / 2;
  uint8_t offset_y = (height - cell_size * qr_size) / 2;
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(offset_x - cell_size, offset_y - cell_size,
                                (qr_size + 2) * cell_size,
                                (qr_size + 2) * cell_size));
  for (int y = 0; y < qr_size; y++) {
    for (int x = 0; x < qr_size; x++) {
      if (shift == 0) {
        val = cells[i++];
        shift = 8;
      }
      graphics_context_set_fill_color(
          ctx, ((val >> --shift) & 1) ? GColorBlack : GColorWhite);
      graphics_fill_rect(ctx,
                         GRect(x * cell_size + offset_x,
                               y * cell_size + offset_y, cell_size, cell_size),
                         0, 0);
    }
  }
}

static void new_frame(void *data) {
  qr_size = gen_qr("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
                   &cells);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Return: %i", qr_size);
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
    settings.string1 = string1_t->value->cstring;
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
