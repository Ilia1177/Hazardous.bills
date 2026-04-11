#include "Font.hpp"
// ---------------------------------------------------------------------------
// make_png
//
//   fileStream – the raw text content you want rendered (not a filename).
//   font       – visual options (see Font struct above).
//   outputPath – where to write the PNG file (default: "output.png").
//
// Throws std::runtime_error on failure.
// ---------------------------------------------------------------------------
void make_png(std::string       fileStream,
              Font              font,
              std::string_view  outputPath)
{
    // ── 1. Create a throw-away 1×1 surface to measure text ─────────────────
    detail::Surface measure_surf{
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1)
    };
    detail::Cr measure_cr{ cairo_create(measure_surf.get()) };

    // ── 2. Build Pango layout for measurement ───────────────────────────────
    detail::Layout layout{
        pango_cairo_create_layout(measure_cr.get())
    };

    // Build a Pango font description string, e.g. "Monospace Bold Italic 14"
    std::string font_desc_str = font.family;
    if (font.bold)   font_desc_str += " Bold";
    if (font.italic) font_desc_str += " Italic";
    font_desc_str += std::format(" {}", static_cast<int>(font.size));

    PangoFontDescription* fd = pango_font_description_from_string(font_desc_str.c_str());
    if (!fd)
        throw std::runtime_error("make_png: invalid font description: " + font_desc_str);

    pango_layout_set_font_description(layout.get(), fd);
    pango_font_description_free(fd);

    pango_layout_set_text(layout.get(), fileStream.c_str(),
                          static_cast<int>(fileStream.size()));

    // ── 3. Measure the rendered text ────────────────────────────────────────
    int text_w_px = 0, text_h_px = 0;
    pango_layout_get_pixel_size(layout.get(), &text_w_px, &text_h_px);

    const int img_w = text_w_px + 2 * font.padding;
    const int img_h = text_h_px + 2 * font.padding;

    if (img_w <= 0 || img_h <= 0)
        throw std::runtime_error("make_png: computed image dimensions are non-positive");

    // ── 4. Create the real surface at the correct size ───────────────────────
    detail::Surface surface{
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, img_w, img_h)
    };
    if (cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS)
        throw std::runtime_error("make_png: failed to create Cairo surface");

    detail::Cr cr{ cairo_create(surface.get()) };

    // ── 5. Draw background ───────────────────────────────────────────────────
    cairo_set_source_rgba(cr.get(), font.bg_r, font.bg_g, font.bg_b, font.bg_a);
    cairo_paint(cr.get());

    // ── 6. Re-create Pango layout on the real context ───────────────────────
    detail::Layout real_layout{
        pango_cairo_create_layout(cr.get())
    };

    PangoFontDescription* fd2 = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(real_layout.get(), fd2);
    pango_font_description_free(fd2);

    pango_layout_set_text(real_layout.get(), fileStream.c_str(),
                          static_cast<int>(fileStream.size()));

    // ── 7. Render text ───────────────────────────────────────────────────────
    cairo_translate(cr.get(), font.padding, font.padding);
    cairo_set_source_rgba(cr.get(), font.fg_r, font.fg_g, font.fg_b, font.fg_a);
    pango_cairo_show_layout(cr.get(), real_layout.get());

    // ── 8. Write PNG ─────────────────────────────────────────────────────────
    const std::string path_str{ outputPath };
    cairo_status_t status =
        cairo_surface_write_to_png(surface.get(), path_str.c_str());

    if (status != CAIRO_STATUS_SUCCESS)
        throw std::runtime_error(
            std::format("make_png: cairo_surface_write_to_png failed ({})",
                        cairo_status_to_string(status)));
}

// ---------------------------------------------------------------------------
// Example usage
// ---------------------------------------------------------------------------
// #include <fstream>
// #include <sstream>
// #include <iostream>
//
// int main(int argc, char* argv[])
// {
//     const char* txt_path = (argc > 1) ? argv[1] : "input.txt";
//     const char* png_path = (argc > 2) ? argv[2] : "output.png";
//
//     // Read the .txt file into a std::string
//     std::ifstream ifs(txt_path);
//     if (!ifs)
//         throw std::runtime_error(std::string("Cannot open: ") + txt_path);
//
//     std::ostringstream oss;
//     oss << ifs.rdbuf();
//
//     Font font;
//     font.family  = "Monospace";
//     font.size    = 13.0;
//     font.bold    = false;
//     font.fg_r = 0.10; font.fg_g = 0.10; font.fg_b = 0.10;  // near-black text
//     font.bg_r = 0.97; font.bg_g = 0.97; font.bg_b = 0.95;  // off-white background
//     font.padding = 24;
//
//     make_png(oss.str(), font, png_path);
//     std::cout << "Written to: " << png_path << '\n';
// }
//
