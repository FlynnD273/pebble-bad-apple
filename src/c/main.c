#include <pebble.h>

#define FRAME_WIDTH 144
#define FRAME_HEIGHT 108
#define NUM_PIXELS FRAME_WIDTH *FRAME_HEIGHT

#define FPS 12
#define NUM_FRAMES 546

static Window *s_main_window;
static Layer *s_layer;
static uint frame = 0;
static ResHandle frames_resource;
static size_t frames_res_size;
static size_t index_offset = 0;
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

uint16_t get_value(uint8_t *buffer, size_t *index) {
  uint8_t val1 = buffer[*index];
  if (val1 < 0xff) {
    (*index)++;
    return val1;
  }
  uint8_t msb = buffer[*index + 1];
  uint8_t lsb = buffer[*index + 2];
  (*index) += 3;
  return (msb << 8) | lsb;
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  size_t num_bytes = FRAME_WIDTH * FRAME_HEIGHT;
  if (num_bytes + index_offset > frames_res_size) {
    num_bytes = frames_res_size - index_offset - 1;
  }
  uint8_t *buffer = (uint8_t *)malloc(num_bytes);
  resource_load_byte_range(frames_resource, index_offset, buffer, num_bytes);
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  GRect bounds = layer_get_bounds(layer);
  GPoint offset = GPoint((bounds.size.w - FRAME_WIDTH) / 2,
                         (bounds.size.h - FRAME_HEIGHT) / 2);

  bool is_black = true;
  size_t curr_index = 0;
  uint16_t curr_count = 0;
  uint16_t curr_run_count = get_value(buffer, &curr_index);

  for (int y = 0; y < FRAME_HEIGHT; y++) {
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y + offset.y);
    for (int x = 0; x < FRAME_WIDTH; x++) {
      while (curr_count >= curr_run_count) {
        curr_count = 0;
        is_black = !is_black;
        curr_run_count = get_value(buffer, &curr_index);
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
  index_offset += curr_index;
  free(buffer);

  graphics_release_frame_buffer(ctx, fb);
  frame++;
}

static void new_frame(void *data) {
  if (frame >= NUM_FRAMES) {
    memset(&prev_frame, 0, FRAME_WIDTH * FRAME_HEIGHT);
    frame = 0;
    index_offset = 0;
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
