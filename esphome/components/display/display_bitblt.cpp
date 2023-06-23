#include "display.h"

#include <utility>
#include "esphome/core/application.h"
#include "esphome/core/color.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace display {

static const char *const TAG = "display";

template<typename DestPixelFormat, typename SrcPixelFormat, typename Fn>
static bool display_convert_draw(int x, int y, int width, int height, const SrcPixelFormat *src, int data_length, Color color_on, Color color_off, Fn draw_pixels) {
  auto dest = new DestPixelFormat[width];
  bool done = true;
  DestPixelFormat dest_on, dest_off;

  // does not support packed pixels
  static_assert(DestPixelFormat::PIXELS == 1);

  if (SrcPixelFormat::COLOR_KEY) {
    dest_on = from_color<DestPixelFormat>(color_on);
    dest_off = from_color<DestPixelFormat>(color_off);
  }

  for (int j = 0; j < height && done; j++) {
    for (auto p = dest, end = dest+width; p < end; ) {
      auto &src_p = *src++;

      for (int i = 0; i < SrcPixelFormat::PIXELS && p < end; i++) {
        if (SrcPixelFormat::COLOR_KEY) {
          *p++ = src_p.is_on(i) ? dest_on : dest_off;
        } else {
          *p++ = from_pixel_format<DestPixelFormat, SrcPixelFormat>(src_p, i);
        }
      }
    }

    done = draw_pixels(x, y + j,
      width, 1,
      (const uint8_t*)dest, width * DestPixelFormat::BYTES);
  }

  delete [] dest;
  return done;
}

template<typename DestPixelFormat, typename SrcPixelFormat, typename Fn>
static bool display_convert_direct(int x, int y, int width, int height, const SrcPixelFormat *src, int data_length, Color color_on, Color color_off, Fn get_pixels) {
  DestPixelFormat dest_on, dest_off;

  // does not support packed pixels
  static_assert(DestPixelFormat::PIXELS == 1);

  if (SrcPixelFormat::COLOR_KEY) {
    dest_on = from_color<DestPixelFormat>(color_on);
    dest_off = from_color<DestPixelFormat>(color_off);
  }

  for (int j = 0; j < height; j++) {
    auto dest = (DestPixelFormat*)get_pixels(y + j);
    if (!dest)
      return false;

    for (auto p = dest + x, end = p + width; p < end; ) {
      const auto &src_p = *src++;

      for (int i = 0; i < SrcPixelFormat::PIXELS && p < end; i++) {
        if (src_p.is_transparent(i)) {
          p++;
          continue;
        }

        if (SrcPixelFormat::COLOR_KEY) {
          *p++ = src_p.is_on(i) ? dest_on : dest_off;
        } else {
          *p++ = from_pixel_format<DestPixelFormat, SrcPixelFormat>(src_p, i);
        }
      }
    }
  }

  return true;
}

bool HOT Display::draw_pixels_at(int x, int y, int width, int height, const uint8_t *data, int data_length, PixelFormat data_format, Color color_on, Color color_off) {
  ESP_LOGV(TAG, "DrawFormat: %dx%d/%dx%d, size=%d, format=%d=>%d",
    x, y, width, height,
    data_length, data_format, this->get_native_pixel_format());

  if (this->get_native_pixel_format() == data_format) {
    return this->draw_pixels_(x, y, width, height, data, data_length);
  }

  auto get_pixels = [this](int y) {
    return this->get_native_pixels_(y);
  };
  auto draw_pixels = [this](int x, int y, int w, int h, const uint8_t *data, int data_length) {
    return this->draw_pixels_(x, y, w, h, data, data_length);
  };

  #define CONVERT_IGNORE_FORMAT(format) \
    case PixelFormat::format: \
      break

  #define CONVERT_DEST_FORMAT(dest_format) \
    case PixelFormat::dest_format: \
      if (display_convert_direct<Pixel##dest_format>(x, y, width, height, src_data, data_length, color_on, color_off, get_pixels)) \
        return true; \
      if (display_convert_draw<Pixel##dest_format>(x, y, width, height, src_data, data_length, color_on, color_off, draw_pixels)) \
        return true; \
      break

  #define CONVERT_SRC_FORMAT(src_format) \
    case PixelFormat::src_format: \
      { \
        auto src_data = (const Pixel##src_format*)data; \
        switch (this->get_native_pixel_format()) { \
          EXPORT_DEST_PIXEL_FORMAT(CONVERT_DEST_FORMAT, CONVERT_IGNORE_FORMAT); \
        } \
      } \
      break

  switch (data_format) {
    EXPORT_SRC_PIXEL_FORMAT(CONVERT_SRC_FORMAT, CONVERT_IGNORE_FORMAT);
  }

  return false;
}

template<typename Format, typename Fn>
static bool display_filled_rectangle_alloc(int x, int y, int width, int height, Color color, Fn draw_pixels) {
  auto pixel_color = from_color<Format>(color);
  auto dest = new Format[width];
  bool done = true;

  // does not support packed pixels
  static_assert(Format::PIXELS == 1);

  for (auto p = dest, end = dest + width; p < end; )
    *p++ = pixel_color;

  for (int j = 0; j < height && done; j++) {
    done = draw_pixels(
      x, y + j,
      width, 1,
      (const uint8_t*)dest, width * Format::BYTES
    );
  }

  delete [] dest;
  return done;
}

template<typename Format, typename Fn>
static bool display_filled_rectangle_direct(int x, int y, int width, int height, Color color, Fn get_pixels) {
  auto pixel_color = from_color<Format>(color);

  // does not support packed pixels
  static_assert(Format::PIXELS == 1);

  for (int j = 0; j < height; j++) {
    auto dest = (Format*)get_pixels(y + j);
    if (!dest)
      return false;

    for (auto p = dest + x, end = p + width; p < end; )
      *p++ = pixel_color;
  }

  return true;
}

bool Display::filled_rectangle_(int x, int y, int width, int height, Color color) {
  auto get_pixels = [this](int y) {
    return this->get_native_pixels_(y);
  };
  auto draw_pixels = [this](int x, int y, int w, int h, const uint8_t *data, int data_length) {
    return this->draw_pixels_(x, y, w, h, data, data_length);
  };

  #define FILLED_RECT_FORMAT(format) \
    case PixelFormat::format: \
      if (display_filled_rectangle_direct<Pixel##format>(x, y, width, height, color, get_pixels)) \
        return true; \
      if (display_filled_rectangle_alloc<Pixel##format>(x, y, width, height, color, draw_pixels)) \
        return true; \
      break

  #define FILLED_RECT_IGNORE(format) \
    case PixelFormat::format: \
      break

  switch (this->get_native_pixel_format()) {
    EXPORT_DEST_PIXEL_FORMAT(FILLED_RECT_FORMAT, FILLED_RECT_IGNORE);
  }

  return false;
}

} // display
} // esphome
