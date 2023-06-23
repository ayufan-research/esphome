#include "buffer.h"

#include <utility>
#include "esphome/core/application.h"
#include "esphome/core/color.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace display_buffer {

static const char *const TAG = "display";

template<typename PixelFormat>
void Buffer<PixelFormat>::setup() {
  ExternalRAMAllocator<PixelFormat> allocator(ExternalRAMAllocator<PixelFormat>::ALLOW_FAILURE);

  this->buffer_length_ = PixelFormat::bytes_stride(this->width_, this->height_);
  this->buffer_ = allocator.allocate(PixelFormat::stride(this->width_, this->height_));

  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate buffer for framebuffer!");
    this->mark_failed();
    return;
  }
}

template<typename PixelFormat>
void Buffer<PixelFormat>::dump_config() {
  LOG_DISPLAY("", "Buffer", this);
  ESP_LOGD(TAG, "  Format: %d", PixelFormat::FORMAT);
  ESP_LOGD(TAG, "  Height: %d", this->height_);
  ESP_LOGD(TAG, "  Width: %d", this->width_);
}

template<typename PixelFormat>
display::DisplayType Buffer<PixelFormat>::get_display_type() {
  if (PixelFormat::R || PixelFormat::G || PixelFormat::B) {
    return display::DISPLAY_TYPE_COLOR;
  } else if (PixelFormat::W > 1) {
    return display::DISPLAY_TYPE_GRAYSCALE;
  } else {
    return display::DISPLAY_TYPE_BINARY;
  }
}

template<typename PixelFormat>
void Buffer<PixelFormat>::update() {
  this->do_update_();
}

template<typename PixelFormat>
void HOT Buffer<PixelFormat>::draw_pixel_at(int x, int y, Color color) {
  if (!this->clip(x, y))
    return;

  auto dest = this->get_native_pixels_(x, y);
  if (!dest)
    return;

  *dest = display::from_color<PixelFormat>(color);
  App.feed_wdt();
}

template<typename PixelFormat>
uint8_t *Buffer<PixelFormat>::get_native_pixels_(int y) {
  if (y < 0 || y >= this->height_)
    return nullptr;
  return (uint8_t *)this->get_native_pixels_(0, y);
}

template<typename PixelFormat>
PixelFormat *Buffer<PixelFormat>::get_native_pixels_(int x, int y) {
  if (!this->buffer_)
    return nullptr;

  return offset_buffer(this->buffer_, x, y, this->width_);
}

template<typename PixelFormat>
bool Buffer<PixelFormat>::draw_pixels_(int x_at, int y_at, int w, int h, const uint8_t *data, int data_length) {
  int min_x, max_x;
  if (!this->clamp_x(x_at, w, min_x, max_x))
    return true;

  int min_y, max_y;
  if (!this->clamp_x(y_at, h, min_y, max_y))
    return true;

  for (int y = min_y; y < max_y; y++) {
    auto dest = this->get_native_pixels_(min_x, y);
    if (!dest)
      return false;

    auto src = offset_buffer((const PixelFormat*)data, min_x - x_at, y - y_at, w);
    memcpy(dest, src, PixelFormat::bytes_stride(max_x - min_x));
  }
 
  return true;
}

template<typename PixelFormat>
void Buffer<PixelFormat>::draw(display::Display *display) {
  display->draw_pixels_at(
    0, 0, this->width_, this->height_,
    (const uint8_t *)this->buffer_, this->buffer_length_,
    PixelFormat::FORMAT);
}

#define EXPORT_FRAMEBUFFER_TEMPLATE(name) \
  template class Buffer<display::Pixel##name>
#define IGNORE_FRAMEBUFFER_TEMPLATE(name)

EXPORT_DEST_PIXEL_FORMAT(EXPORT_FRAMEBUFFER_TEMPLATE, IGNORE_FRAMEBUFFER_TEMPLATE);

}  // namespace display_buffer
}  // namespace esphome
