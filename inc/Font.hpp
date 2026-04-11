#ifndef FONT_HPP
# define FONT_HPP
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <string>
#include <string_view>
#include <stdexcept>
#include <cmath>
#include <format>

// ---------------------------------------------------------------------------
// Font descriptor – pass by value; all fields are optional with sensible defaults.
// ---------------------------------------------------------------------------
struct Font {
    std::string family  = "Monospace";   // e.g. "DejaVu Sans", "Liberation Serif"
    double      size    = 14.0;          // points
    bool        bold    = false;
    bool        italic  = false;

    // Foreground / background colours (RGBA, 0.0–1.0)
    double fg_r = 0.0, fg_g = 0.0, fg_b = 0.0, fg_a = 1.0;   // black text
    double bg_r = 1.0, bg_g = 1.0, bg_b = 1.0, bg_a = 1.0;   // white background

    // Padding around the text block (pixels)
    int padding = 20;
};

// ---------------------------------------------------------------------------
// RAII wrappers so we never leak Cairo/Pango objects on throw.
// ---------------------------------------------------------------------------
namespace detail {

struct SurfaceDeleter {
    void operator()(cairo_surface_t* s) const noexcept { if (s) cairo_surface_destroy(s); }
};
struct CrDeleter {
    void operator()(cairo_t* cr) const noexcept { if (cr) cairo_destroy(cr); }
};
struct LayoutDeleter {
    void operator()(PangoLayout* l) const noexcept { if (l) g_object_unref(l); }
};

using Surface = std::unique_ptr<cairo_surface_t, SurfaceDeleter>;
using Cr      = std::unique_ptr<cairo_t,         CrDeleter>;
using Layout  = std::unique_ptr<PangoLayout,     LayoutDeleter>;

} // namespace detail
  //
void make_png(std::string fileStream, Font font, std::string_view  outputPath = "output.png");

#endif
