
#include "PrintFact.hpp"
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <exception>
#include <utility>
#include "tools.h"

namespace fs = std::filesystem;
// Helper function to prompt and read a line
std::string userline(std::string label, bool blocking)
{
	bool fail = false;
	std::string value;
	do {
		if (fail)
			clearInputLine();
		std::cout << label << ": ";
		std::getline(std::cin, value);
		if (!fail) {
			label = "Field needed ! " + label;
			fail = true;
		}
	} while (blocking && value.empty() && std::cin.good() && !g_interrupt);
    return value;
}

size_t dial_menu(const std::vector<std::string>& menu)
{
	int margin = 4;
	if (g_interrupt)
		return 0;
	int choice = -1;
	for (size_t i = 0; i < menu.size(); i++) {
		std::cout << fit(" ", margin) << i << ". " << menu[i] << "\n";
	}
	std::cout << "\n";
	while (!g_interrupt) {
		if (!user_value(fit(" ", margin) + "Select", choice, true))
			return 0;
		if (choice < 0 || static_cast<size_t>(choice) > menu.size() + 1) {
			clearInputLine();
			std::cout << fit(" ", margin) << "Choice not available... ";
		} else {
			eraseLines(menu.size() + 2);
			std::cout << fit(" ", margin) << "* " << menu[choice] << "\n\n";
			break;
		}
	};
	return static_cast<size_t>(choice);
}
std::string end_str(int size, int offset, bool reset)
{
    static bool firstUse = true;
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::string poem;
    static std::ifstream* text = nullptr;  // lazy-initialized

    if (reset) {
        firstUse = true;  // also fix: reset should allow re-init
        delete text;
        text = nullptr;
        poem.clear();
        return "";
    }

    std::string off(" ");
    for (int i = 0; i < offset; i++) {
        off += " ";
    }

    if (firstUse) {
        std::uniform_int_distribution<size_t> distribution(0, 4);
        int r = distribution(generator);
        poem = (r > 0) ? "poem" + std::to_string(r) + ".txt" : "";

        if (!poem.empty()) {
            text = new std::ifstream(poem);
            if (!text->is_open()) {
                delete text;
                text = nullptr;
                poem.clear();
            }
        }
        firstUse = false;
    }

    if (text && text->is_open()) {
        std::string line;
        if (std::getline(*text, line)) {
            return off + line + "\n";
        }
    }

    return off + random_string(size, {" ", "█"}) + "\n";
}

// std::string end_str(int size, int offset, bool reset)
// {
// 	static bool firstUse = true;
//     static std::random_device rd;
//     static std::mt19937 generator(rd());
// 	static std::string poem;
//
// 	if (reset) {
// 		firstUse = false;
// 		return "";
// 	}
// 	std::string off(" ");
//     for (int i = 0; i < offset; i++) {
//         off += " ";
//     }
// 	if (firstUse) {
// 		std::uniform_int_distribution<size_t> distribution(0, 4);
// 		int r = distribution(generator);
// 		if (r > 0) {
// 			poem = "poem" + std::to_string(r) + ".txt";
// 		} else {
// 			poem.clear();
// 		}
// 	}
// 	firstUse = false;
// 	if (!poem.empty()) {
// 		static std::ifstream text(poem);
// 		if (text && text.is_open()) {
// 			std::string line;
// 			std::getline(text, line);
// 			return (off + line + "\n");
// 		}
// 	}
//     return off + random_string(size, {" ", "█"}) + "\n";
// }
//
void clearInputLine()
{
    std::cout << "\033[1A\r\033[2K" << std::flush;
}

void clearScreen()
{
    std::cout << "\033[2J\033[H";
}

// void moveCursor(int row, int col)
// {
//     std::cout << "\033[" << row << ";" << col << "H";
// }

void eraseLines(int lines)
{
    for (int i = 0; i < lines; ++i) {
        std::cout << "\033[1A\r\033[2K";
    }
    std::cout << std::flush;
}

// void moveCursorUp(int lines)
// {
//     std::cout << "\033[" << lines << "A";
// }

void clearLine()
{
	std::cout << "\033[2K";
}

bool user_value(const std::string& str, double& value, bool blocking) {
	std::string line;

	value = 0.0;
	while (!g_interrupt) {
		std::cout << str << ": ";
		if (!getline(std::cin, line)) 
			return false;
		if (g_interrupt)
			return false;
		try {
			value = std::stod(line);
		} catch (std::exception& e) {
			if (blocking) {
				clearInputLine();
				std::cerr << "Error: " << e.what() << ". ";
			}
		}
		if (blocking && value < EPSILON) {
			continue;
		} else if (value < 0.0) {
			continue;
		}
		break;
	} 
	return true;
}

bool user_value(const std::string& str, int& value, bool blocking) {
	std::string line;

	value = 0;
	while (!g_interrupt) {
		std::cout << str << ": ";
		if (!getline(std::cin, line)) 
			return false;
		if (g_interrupt)
			return false;
		try {
			value = std::stoi(line);
		} catch (std::exception& e) {
			if (blocking) {
				clearInputLine();
				std::cerr << "Error: " << e.what() << ". ";
				continue;
			}
		}
		if (blocking && value < 0) {
			clearInputLine();
			std::cerr << "Choice should be in menu... (" << value << "). ";
			continue;
		}
		break;
	}
	return true;
}

// bool user_input(const std::string& str, std::string& input, bool blocking = true, size_t maxLength) {
// 	std::string line = "";
//
// 	while (!g_interrupt) {
// 		std::cout << str;
// 		if (!std::getline(std::cin, line))
// 			return false;
// 		if (g_interrupt)
// 			return false;
// 		trim(line);
// 		if (maxLength == 0)
// 			break;
// 		if (line.length() > maxLength) {
// 			clearInputLine();
// 			std::cout << maxLength << " characters max. ";
// 			continue;
// 		} else if (blocking && line.empty()) {
// 			clearInputLine();
// 			std::cout << "Field needed ! ";
// 			continue;
// 		}
// 		break;
// 	};
// 	input = line;
// 	return true;
// }

// std::string get_bill_filename(const std::string& pattern)
// {
// 	std::string empty("");
//
//     if (!fs::exists("factures") && !fs::exists("devis")) {
//         return empty;
//     }
//     for (const auto& entry : fs::directory_iterator("factures")) {
//         std::string filename = entry.path().filename().string();
//         if (filename.find(pattern) != std::string::npos) {
//             return filename;
//         }
//     }
//     for (const auto& entry : fs::directory_iterator("devis")) {
//         std::string filename = entry.path().filename().string();
//         if (filename.find(pattern) != std::string::npos) {
//             return filename;
//         }
//     }
//     for (const auto& entry : fs::directory_iterator("tickets")) {
//         std::string filename = entry.path().filename().string();
//         if (filename.find(pattern) != std::string::npos) {
//             return filename;
//         }
//     }
//     return empty;
// }

// void stream_find_replace(std::stringstream& ss, const std::string& search, const std::string& replace) {
//     std::string content = ss.str();
//     size_t pos = content.find(search);
//
//     if (pos != std::string::npos) {
//         content.replace(pos, search.length(), replace);
//         ss.str(content);
//         ss.clear();
//     }
// }

// Usage
std::string random_string(size_t length, const std::vector<std::string>& charset)
{
    static std::random_device rd;
    static std::mt19937       generator(rd());

    std::uniform_int_distribution<size_t> distribution(0, charset.size() - 1);

    std::string result;

    for (size_t i = 0; i < length; ++i) {
        result += charset[distribution(generator)];
    }

    return result;
}

// std::string sanitize_filename(const std::string& name)
// {
//     std::string result;
//
// 	size_t len = 0;
//
//     for (char c : name) {
// 		if (len > 8) {
// 			break;
// 		}
//         if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
//             result += c;
// 			len++;
//         } else if (!result.empty() && c == ' ') {
// 			continue;
//         }
//     }
//
//     return result;
// }

void trim(std::string& str)
{
	if (str.empty())
		return;
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        str.clear(); // All whitespace
        return;
    }

    str.erase(0, start);
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    str.erase(end + 1);
}

std::stringstream printMap(const std::map<std::string, std::pair<int, double> >& m)
{
    std::stringstream ss;
	std::pair<int, double> p;
    for (const auto& it : m) {
		p = it.second;
        ss << "    - " << std::setw(12) << it.first + " : " << std::setw(2) << p.second
           << std::endl;
    }
    return ss;
}

#include <fstream>
#include <map>

std::map<std::string, std::string> load_config(const std::string& path) {
    std::map<std::string, std::string> config;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '[') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key   = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        // trim spaces
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        config[key] = value;
    }
    return config;
}

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
	std::cout << "making png...\n";
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
