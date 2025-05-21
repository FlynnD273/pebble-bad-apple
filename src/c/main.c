#include <pebble.h>

#define FRAME_WIDTH 144
#define FRAME_HEIGHT 108
#define NUM_PIXELS FRAME_WIDTH *FRAME_HEIGHT

#define FPS 12
#define NUM_FRAMES 740

static Window *s_main_window;
static Layer *s_layer;
static uint frame = 0;
static ResHandle frames_resource;
static size_t frames_res_size;
static size_t half_nybble_index = 0;
static bool prev_frame[FRAME_WIDTH * FRAME_HEIGHT];

static void byte_set_bit(uint8_t *byte, uint8_t bit, uint8_t value) {
  *byte ^= (-value ^ *byte) & (1 << bit);
}

static void set_pixel_color(GBitmapDataRowInfo info, uint16_t x, GColor color) {
#if defined(PBL_COLOR)
  // Write the pixel's byte color
  memset(&info.data[x], color.argb, 1);
#elif defined(PBL_BW)
  // Find the correct byte, then set the appropriate bit
  uint8_t byte = x / 8;
  uint8_t bit = x % 8;
  byte_set_bit(&info.data[byte], bit, gcolor_equal(color, GColorWhite) ? 1 : 0);
#endif
}

uint8_t get_half_nybble(uint8_t *buffer, size_t *index) {
  uint8_t val = buffer[(*index) / 4] >> (6 - ((*index) % 4) * 2);
  (*index)++;
  return val & 0b11;
}

uint8_t get_nybble(uint8_t *buffer, size_t *index) {
  uint8_t high_bits = get_half_nybble(buffer, index);
  uint8_t low_bits = get_half_nybble(buffer, index);
  return high_bits << 2 | low_bits;
}

uint8_t get_byte(uint8_t *buffer, size_t *index) {
  uint8_t high_bits = get_nybble(buffer, index);
  uint8_t low_bits = get_nybble(buffer, index);
  return high_bits << 4 | low_bits;
}

uint16_t get_short(uint8_t *buffer, size_t *index) {
  uint8_t high_bits = get_byte(buffer, index);
  uint8_t low_bits = get_byte(buffer, index);
  return high_bits << 8 | low_bits;
}

uint16_t get_value(uint8_t *buffer, size_t *index) {
  uint8_t format = get_half_nybble(buffer, index);
  switch (format) {
  case 0:
    return get_half_nybble(buffer, index);
  case 1:
    return get_nybble(buffer, index);
  case 2:
    return get_byte(buffer, index);
  case 3:
    return get_short(buffer, index);
  }
  return 0;
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  size_t num_bytes = FRAME_WIDTH * FRAME_HEIGHT;
  if (num_bytes + half_nybble_index / 4 > frames_res_size) {
    num_bytes = frames_res_size - half_nybble_index / 4 - 1;
  }
  uint8_t *buffer = (uint8_t *)malloc(num_bytes);
  resource_load_byte_range(frames_resource, half_nybble_index / 4, buffer,
                           num_bytes);
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  GRect bounds = layer_get_bounds(layer);
  GPoint offset = GPoint((bounds.size.w - FRAME_WIDTH) / 2,
                         (bounds.size.h - FRAME_HEIGHT) / 2);

  bool is_black = true;
  uint8_t starting_index = half_nybble_index % 4;
  size_t curr_half_nybble_index = starting_index;
  uint16_t curr_count = 0;
  uint16_t curr_run_count = get_value(buffer, &curr_half_nybble_index);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Run count: %d", curr_run_count);

  for (int y = 0; y < FRAME_HEIGHT; y++) {
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y + offset.y);
    for (int x = 0; x < FRAME_WIDTH; x++) {
      while (curr_count >= curr_run_count) {
        curr_count = 0;
        is_black = !is_black;
        curr_run_count = get_value(buffer, &curr_half_nybble_index);
      }
      if (!is_black) {
        // Why can't I XOR here??
        prev_frame[y * FRAME_WIDTH + x] = !prev_frame[y * FRAME_WIDTH + x];
      }
      if (prev_frame[y * FRAME_WIDTH + x]) {
        set_pixel_color(info, x + offset.x, GColorWhite);
      }
      curr_count++;
    }
  }
  half_nybble_index += curr_half_nybble_index - starting_index;
  free(buffer);

  graphics_release_frame_buffer(ctx, fb);
  frame++;
}

static void new_frame(void *data) {
  if (frame >= NUM_FRAMES) {
    memset(&prev_frame, 0, FRAME_WIDTH * FRAME_HEIGHT);
    frame = 0;
    half_nybble_index = 0;
  }
  app_timer_register(1000 / FPS, new_frame, NULL);
  layer_mark_dirty(s_layer);
}

static void main_window_load(Window *window) {
  frames_resource = resource_get_handle(RESOURCE_ID_FRAMES);
  frames_res_size = resource_size(frames_resource);
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_bounds(window_layer);
  s_layer = layer_create(frame);

  layer_add_child(window_layer, s_layer);
  layer_set_update_proc(s_layer, frame_redraw);
  layer_mark_dirty(s_layer);
  new_frame(NULL);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_layer);
  window_destroy(s_main_window);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  window_stack_push(s_main_window, true);
}

int main(void) {
  init();
  app_event_loop();
}
