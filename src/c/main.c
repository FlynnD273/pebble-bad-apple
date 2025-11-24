#include <pebble.h>

static uint16_t width;
static uint16_t height;
#define FRAME_WIDTH 144
#define FRAME_HEIGHT 108
#define NUM_PIXELS FRAME_WIDTH *FRAME_HEIGHT

#define FPS 12
#define NUM_FRAMES 2629

static Window *s_main_window;
static Layer *s_layer;
static uint frame = 0;
static ResHandle frames_resource;
static size_t frames_res_size;
static size_t bit_index = 0;
static uint8_t *buffer;

uint8_t get_bit(uint8_t *buffer, size_t *index) {
  uint8_t val = buffer[(*index) / 8] >> (7 - ((*index) % 8));
  (*index)++;
  return val & 1;
}

uint8_t get_half_nybble(uint8_t *buffer, size_t *index) {
  uint8_t high_bits = get_bit(buffer, index);
  uint8_t low_bits = get_bit(buffer, index);
  return high_bits << 1 | low_bits;
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

static void parse_quadtree(GContext *ctx, GRect bounds, uint8_t *buffer,
                           size_t *bit_index) {
  uint8_t should_recurse = get_bit(buffer, bit_index);
  if (should_recurse) {
    parse_quadtree(ctx,
                   GRect(bounds.origin.x, bounds.origin.y,
                         (bounds.size.w + 1) / 2, bounds.size.h / 2),
                   buffer, bit_index);
    parse_quadtree(ctx,
                   GRect(bounds.origin.x + bounds.size.w / 2, bounds.origin.y,
                         (bounds.size.w + 1) / 2, (bounds.size.h + 1) / 2),
                   buffer, bit_index);
    parse_quadtree(ctx,
                   GRect(bounds.origin.x, bounds.origin.y + bounds.size.h / 2,
                         (bounds.size.w + 1) / 2, (bounds.size.h + 1) / 2),
                   buffer, bit_index);
    parse_quadtree(ctx,
                   GRect(bounds.origin.x + bounds.size.w / 2,
                         bounds.origin.y + bounds.size.h / 2,
                         (bounds.size.w + 1) / 2, (bounds.size.h + 1) / 2),
                   buffer, bit_index);
  } else {
    uint8_t is_white = get_bit(buffer, bit_index);
    if (is_white) {
      graphics_fill_rect(ctx, bounds, 0, 0);
    }
  }
}

static void frame_redraw(Layer *layer, GContext *ctx) {
  size_t num_bytes = FRAME_WIDTH * FRAME_HEIGHT;
  if (num_bytes + bit_index / 8 > frames_res_size) {
    num_bytes = frames_res_size - bit_index / 8 - 1;
  }
  resource_load_byte_range(frames_resource, bit_index / 8, buffer, num_bytes);
  GRect bounds = layer_get_bounds(layer);
  GPoint offset =
      GPoint((bounds.size.w - width) / 2, (bounds.size.h - height) / 2);

  uint8_t starting_index = bit_index % 8;
  size_t curr_bit_index = starting_index;

  graphics_context_set_fill_color(ctx, GColorWhite);
  parse_quadtree(ctx, GRect(offset.x, offset.y, width, height), buffer,
                 &curr_bit_index);

  bit_index += curr_bit_index - starting_index;
  frame++;
}

static void new_frame(void *data) {
  if (frame >= NUM_FRAMES) {
    frame = 0;
    bit_index = 0;
  }
  app_timer_register(1000 / FPS, new_frame, NULL);
  layer_mark_dirty(s_layer);
}

static void main_window_load(Window *window) {
  frames_resource = resource_get_handle(RESOURCE_ID_FRAMES);
  frames_res_size = resource_size(frames_resource);
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_bounds(window_layer);
  width = frame.size.w;
  height = width * 3 / 4;
#ifdef PBL_ROUND
  width *= 4;
  width /= 5;
  height *= 4;
  height /= 5;
#endif
  buffer = (uint8_t *)malloc(FRAME_WIDTH * FRAME_HEIGHT);
  s_layer = layer_create(frame);

  layer_add_child(window_layer, s_layer);
  layer_set_update_proc(s_layer, frame_redraw);
  layer_mark_dirty(s_layer);
  new_frame(NULL);
}

static void main_window_unload(Window *window) {
  free(buffer);
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
