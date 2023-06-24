#pragma once

#include <cstdarg>
#include <vector>

#include "display_color_utils.h"

namespace esphome {
namespace display {

enum class PixelFormat {
  Unknown,
  A1,
  W1,
  W4,
  W8,
  W8_KEY,
  RGB332,
  RGB565,
  RGB565_BE,
  RGB888,
  RGBA4444,
  RGBA8888
};

#define EXPORT_SRC_PIXEL_FORMAT(MACRO, IGNORE_MACRO, ...) \
  IGNORE_MACRO(Unknown, ##__VA_ARGS__); \
  MACRO(A1, ##__VA_ARGS__); \
  MACRO(W1, ##__VA_ARGS__); \
  MACRO(W4, ##__VA_ARGS__); \
  MACRO(W8, ##__VA_ARGS__); \
  MACRO(W8_KEY, ##__VA_ARGS__); \
  MACRO(RGB332, ##__VA_ARGS__); \
  MACRO(RGB565, ##__VA_ARGS__); \
  MACRO(RGB565_BE, ##__VA_ARGS__); \
  MACRO(RGB888, ##__VA_ARGS__); \
  MACRO(RGBA4444, ##__VA_ARGS__); \
  MACRO(RGBA8888, ##__VA_ARGS__);

#define EXPORT_DEST_PIXEL_FORMAT(MACRO, IGNORE_MACRO, ...) \
  IGNORE_MACRO(Unknown, ##__VA_ARGS__); \
  IGNORE_MACRO(A1, ##__VA_ARGS__); \
  IGNORE_MACRO(W1, ##__VA_ARGS__); \
  MACRO(W4, ##__VA_ARGS__); \
  MACRO(W8, ##__VA_ARGS__); \
  IGNORE_MACRO(W8_KEY, ##__VA_ARGS__); \
  MACRO(RGB332, ##__VA_ARGS__); \
  MACRO(RGB565, ##__VA_ARGS__); \
  MACRO(RGB565_BE, ##__VA_ARGS__); \
  MACRO(RGB888, ##__VA_ARGS__); \
  MACRO(RGBA4444, ##__VA_ARGS__); \
  MACRO(RGBA8888, ##__VA_ARGS__);

template<int In, int Out>
inline uint8_t shift_bits(uint8_t src) {
  if (!In || !Out) {
    return 0;
  } else if (In < Out) {
    // expand: In=5 => Out=8
    // src << (8-5) + src >> (5 - (8-5))
    // src << (8-5) + src >> (5 - 8 + 5)
    uint8_t out = src << (Out-In);

    for (int i = 2; i < 8; i++) {
      if (Out-i*In >= 0)
        out += src << (Out-i*In);
      else if (Out-i*In >= -In)
        out += src >> -(Out-i*In);
      else
        break;
    }

    return out;
  } else if (In > Out) {
    // reduce: In=8 => Out=5
    // (src + (1<<(8-5) - 1)) >> (8-5)
    return src >> (In-Out);
  } else {
    return src;
  }
}

inline uint16_t swap16(uint16_t x) {
  return (x << 8) | (x >> 8);
}

template<PixelFormat FF, int RR, int GG, int BB, int AA, int WW, int NN, int PP = 1, bool CCOLOR_KEY = 0>
struct PixelDetails {
  static const PixelFormat FORMAT = FF;
  static const int R = RR;
  static const int G = GG;
  static const int B = BB;
  static const int A = AA;
  static const int W = WW;
  static const int BYTES = NN;
  static const int PIXELS = PP;
  static const bool COLOR_KEY = CCOLOR_KEY;

  static inline int pixel_index(int x) {
    return x % PIXELS;
  }
  static inline int pixel_offset(int x) {
    return x / PIXELS * PIXELS;
  }
  static inline int offset(int x) {
    return x / PIXELS;
  }
  static inline int stride(int width) {
    return (width + PIXELS - 1) / PIXELS;
  }
  static inline int stride(int width, int height) {
    return stride(width) * height;
  }
  static inline int bytes_stride(int width) {
    return stride(width) * BYTES;
  }
  static inline int bytes_stride(int width, int height) {
    return stride(width, height) * BYTES;
  }
};

struct PixelRGB332 : PixelDetails<PixelFormat::RGB332, 3,3,2,0,0,1> {
  uint8_t raw_8;

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return false;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    this->raw_8 = r << (3+2) | g << (2) | b;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = this->raw_8 >> (3+2);
    g = (this->raw_8 >> (2)) & ((1<<3) - 1);
    b = (this->raw_8 >> 0) & ((1<<2) - 1);
    w = 0; a = 0;
  }
} PACKED;

template<PixelFormat Format, bool BigEndian>
struct PixelRGB565_Endiness : PixelDetails<Format,5,6,5,0,0,2> {
  uint16_t raw_16;

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return false;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    uint16_t value = r << (6+5) | g << (5) | b;
    this->raw_16 = BigEndian ? swap16(value) : value;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    uint16_t value = BigEndian ? swap16(this->raw_16) : this->raw_16;
    r = value >> (6+5);
    g = (value >> (5)) & ((1<<6) - 1);
    b = (value >> 0) & ((1<<5) - 1);
    w = 0; a = 0;
  }
} PACKED;

typedef PixelRGB565_Endiness<PixelFormat::RGB565, false> PixelRGB565;
typedef PixelRGB565_Endiness<PixelFormat::RGB565_BE, true> PixelRGB565_BE;

struct PixelRGB888 : PixelDetails<PixelFormat::RGB888,8,8,8,0,0,3> {
  uint8_t raw_8[3];

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return false;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    this->raw_8[0] = r;
    this->raw_8[1] = g;
    this->raw_8[2] = b;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = this->raw_8[0];
    g = this->raw_8[1];
    b = this->raw_8[2];
    a = w = 0;
  }
} PACKED;

struct PixelRGBA4444 : PixelDetails<PixelFormat::RGBA4444,4,4,4,4,0,2> {
  uint8_t raw_8[2];

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return (this->raw_8[1] & 0xF) < 0x8;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    this->raw_8[0] = r << 4 | g;
    this->raw_8[1] = b << 4 | a;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = this->raw_8[0] >> 4;
    g = this->raw_8[0] & 0xF;
    b = this->raw_8[1] >> 4;
    a = this->raw_8[1] & 0xF;
    w = 0;
  }
} PACKED;

struct PixelRGBA8888 : PixelDetails<PixelFormat::RGBA8888,8,8,8,8,0,4> {
  uint8_t raw_8[4];

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return this->raw_8[3] < 0x80;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    this->raw_8[0] = r;
    this->raw_8[1] = g;
    this->raw_8[2] = b;
    this->raw_8[3] = a;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = this->raw_8[0];
    g = this->raw_8[1];
    b = this->raw_8[2];
    a = this->raw_8[3];
    w = 0;
  }
} PACKED;

struct PixelW1 : PixelDetails<PixelFormat::W1,0,0,0,0,1,1,8,true> {
  uint8_t raw_8;

  inline bool is_on(int pixel = 0) const {
    return (this->raw_8 & (1<<(7-pixel))) ? true : false;
  }
  inline bool is_transparent(int pixel = 0) const {
    return false;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    const int mask = 1<<(7-pixel);
    this->raw_8 &= ~mask;
    if (w)
      this->raw_8 |= mask;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = 0; g = 0; b = 0; a = 0;
    w = (this->raw_8 & (1<<(7-pixel))) ? 1 : 0;
  }
} PACKED;

struct PixelA1 : PixelDetails<PixelFormat::A1,0,0,0,1,0,1,8,true> {
  uint8_t raw_8;

  inline bool is_on(int pixel = 0) const {
    return (this->raw_8 & (1<<(7-pixel))) ? true : false;
  }
  inline bool is_transparent(int pixel = 0) const {
    return (this->raw_8 & (1<<(7-pixel))) ? false : true;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    const int mask = 1<<(7-pixel);
    this->raw_8 &= ~mask;
    if (a)
      this->raw_8 |= mask;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = 0; g = 0; b = 0; w = 0;
    a = (this->raw_8 & (1<<(7-pixel))) ? 1 : 0;
  }
} PACKED;

struct PixelW4 : PixelDetails<PixelFormat::W4,0,0,0,0,4,1,2> {
  uint8_t raw_8;

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return false;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    if (pixel) {
      this->raw_8 &= ~0xF0;
      this->raw_8 |= w << 4;
    } else {
      this->raw_8 &= ~0x0F;
      this->raw_8 |= w;
    }
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = 0; g = 0; b = 0; a = 0;
    if (pixel)
      w = this->raw_8 >> 4;
    else
      w = this->raw_8 & 0xF;
  }
} PACKED;

template<PixelFormat Format, bool Key>
struct PixelW8_Keyed : PixelDetails<Format,0,0,0,0,8,1> {
  uint8_t raw_8;

  inline bool is_on(int pixel = 0) const {
    return true;
  }
  inline bool is_transparent(int pixel = 0) const {
    return Key ? this->raw_8 == 1 : false;
  }
  inline void encode(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t w, int pixel = 0) {
    this->raw_8 = w;
  }
  inline void decode(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a, uint8_t &w, int pixel = 0) const {
    r = 0; g = 0; b = 0; a = 0;
    w = this->raw_8;
  }
} PACKED;

typedef PixelW8_Keyed<PixelFormat::W8, false> PixelW8;
typedef PixelW8_Keyed<PixelFormat::W8_KEY, true> PixelW8_KEY;

template<typename Out>
static inline Out from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) {
  Out out;
  const uint8_t approx_w = (r >> 2) + (g >> 1) + (b >> 2);
  out.encode(
    shift_bits<8, Out::R>(r),
    shift_bits<8, Out::G>(g),
    shift_bits<8, Out::B>(b),
    shift_bits<8, Out::A>(a),
    shift_bits<8, Out::W>(approx_w)
  );
  return out;
}

template<typename Out>
static inline Out from_w(uint8_t w, uint8_t a = 0xFF) {
  Out out;
  out.encode(
    shift_bits<8, Out::R>(w),
    shift_bits<8, Out::G>(w),
    shift_bits<8, Out::B>(w),
    shift_bits<8, Out::A>(a),
    shift_bits<8, Out::W>(w)
  );
  return out;
}

template<typename Out>
static inline Out from_color(Out &out, const Color &in, int out_pixel = 0) {
  const uint8_t approx_w = (in.r >> 2) + (in.g >> 1) + (in.b >> 2);
  out.encode(
    shift_bits<8, Out::R>(in.r),
    shift_bits<8, Out::G>(in.g),
    shift_bits<8, Out::B>(in.b),
    shift_bits<8, Out::A>(in.w),
    shift_bits<8, Out::W>(approx_w),
    out_pixel
  );
  return out;
}

template<typename Out>
static inline Out from_color(const Color &in, bool expand = true) {
  Out out;
  from_color(out, in);
  if (expand) {
    for (int i = 1; i < Out::PIXELS; i++) {
      from_pixel_format(out, i, out, 0);
    }
  }
  return out;
}

template<typename In>
static inline Color to_color(const In &in, int pixel = 0) {
  uint8_t r, g, b, a, w;
  in.decode(r, g, b, a, w, pixel);

  return Color(
    In::R ? r : In::W ? w : a,
    In::G ? g : In::W ? w : a,
    In::B ? b : In::W ? w : a,
    In::W ? w : In::A ? a : 0xFF
  );
}

template<typename Out>
static inline Out *offset_buffer(Out *buffer, int x, int y, int width) {
  return &buffer[y * Out::stride(width) + Out::offset(x)];
}

template<typename Out>
static inline Out *offset_buffer(Out *buffer, int x) {
  return &buffer[Out::offset(x)];
}

template<typename Out>
static inline Out *offset_end_buffer(Out *buffer, int x) {
  return &buffer[Out::stride(x)];
}

template<typename Out, typename In>
static inline Out &from_pixel_format(Out &out, int out_pixel, const In &in, int in_pixel = 0) {
  uint8_t r, g, b, a, w;
  in.decode(r, g, b, a, w, in_pixel);

  uint8_t approx_w = shift_bits<In::R, 6>(r) + shift_bits<In::G, 7>(g) + shift_bits<In::B, 6>(b);

  out.encode(
    In::R ? shift_bits<In::R, Out::R>(r) : In::W ? shift_bits<In::W, Out::R>(w) : shift_bits<In::A, Out::R>(a),
    In::G ? shift_bits<In::G, Out::G>(g) : In::W ? shift_bits<In::W, Out::G>(w) : shift_bits<In::A, Out::G>(a),
    In::B ? shift_bits<In::B, Out::B>(b) : In::W ? shift_bits<In::W, Out::B>(w) : shift_bits<In::A, Out::B>(a),
    In::A ? shift_bits<In::A, Out::A>(a) : shift_bits<In::W, Out::A>(w),
    In::W ? shift_bits<In::W, Out::W>(w) : shift_bits<8, Out::W>(approx_w),
    out_pixel
  );
  return out;
}

template<typename Out, typename In>
static inline Out from_pixel_format(const In &in, int in_pixel = 0) {
  Out out;
  return from_pixel_format(out, 0, in, in_pixel);
}

template<typename Out>
static inline Out &copy_pixel(Out &out, const Out &in, int start_pixel = 0, int end_pixel = Out::PIXELS) {
  for (int i = start_pixel; i < end_pixel; i++) {
    from_pixel_format(out, i, in, i);
  }
  return out;
}

template<typename SrcPixelFormat, typename DestPixelFormat, bool Transparency>
void bitblt(DestPixelFormat *dest, int dest_x, const SrcPixelFormat *src, int src_x, int width, DestPixelFormat color_on, DestPixelFormat color_off);

template<typename PixelFormat>
void fill(PixelFormat *dest, int x, int width, const PixelFormat &color);

} // display
} // esphome
