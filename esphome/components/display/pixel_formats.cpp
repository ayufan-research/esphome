#include "pixel_formats.h"

#include <utility>

namespace esphome {
namespace display {

template<typename SrcPixelFormat, typename DestPixelFormat>
void bitblt(DestPixelFormat *dest, int dest_x, const SrcPixelFormat *src, int src_x, int width)
{

}

template<typename PixelFormat>
void fill(PixelFormat *dest, int x, int width, const PixelFormat &color)
{
  auto destp = offset_buffer(dest, x);
  auto endp = offset_end_buffer(dest, x + width);
  if (destp == endp)
    return;

  // handle packed pixels
  if (PixelFormat::PIXELS > 1) {
    auto start_offset = PixelFormat::pixel_offset(x);

    // copy start pixels
    if (start_offset > 0) {
      copy_pixel(*destp, color, start_offset);
      destp++;
    }

    // copy end pixels
    auto end_offset = PixelFormat::pixel_offset(x + width);
    if (end_offset > 0 && destp < endp) {
      --endp;
      copy_pixel(*endp, color, 0, end_offset);
    }
  }

  for (auto p = destp; p < endp; p++)
    *p = color;
}

}  // namespace display
}  // namespace esphome
