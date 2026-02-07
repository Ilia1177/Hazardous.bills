#include <png.h>
#include <fstream>
#include "PrintFact.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <stdio.h>

size_t utf8_char_count(const std::string& s) {
    size_t count = 0;
    size_t i = 0;

    while (i < s.size()) {
        unsigned char c = s[i];
        if (c < 0x80)          i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else                         i += 1;
        count++;
    }
    return count;
}

unsigned char map_unicode_to_font(char32_t c32)
{
    // 1) ASCII (exact match)
    if (c32 <= 0x7F)
        return static_cast<unsigned char>(c32);

    // 2) Block elements
    switch (c32) {
        case 0x2588: return 0xDB; // █ full block
        case 0x2593: return 0xB2; // ▓
        case 0x2592: return 0xB1; // ▒
        case 0x2591: return 0xB0; // ░
    }

    // 3) Box drawing (single line)
    switch (c32) {
        case 0x2500: return 0xC4; // ─
        case 0x2502: return 0xB3; // │
        case 0x250C: return 0xDA; // ┌
        case 0x2510: return 0xBF; // ┐
        case 0x2514: return 0xC0; // └
        case 0x2518: return 0xD9; // ┘
        case 0x251C: return 0xC3; // ├
        case 0x2524: return 0xB4; // ┤
        case 0x252C: return 0xC2; // ┬
        case 0x2534: return 0xC1; // ┴
        case 0x253C: return 0xC5; // ┼
    }

    // 4) Box drawing (double line)
    switch (c32) {
        case 0x2550: return 0xCD; // ═
        case 0x2551: return 0xBA; // ║
        case 0x2554: return 0xC9; // ╔
        case 0x2557: return 0xBB; // ╗
        case 0x255A: return 0xC8; // ╚
        case 0x255D: return 0xBC; // ╝
        case 0x2560: return 0xCC; // ╠
        case 0x2563: return 0xB9; // ╣
        case 0x2566: return 0xCB; // ╦
        case 0x2569: return 0xCA; // ╩
        case 0x256C: return 0xCE; // ╬
    }
	
    switch (c32) {
        case 0x2550: return 0xCD; // ═
        case 0x2551: return 0xBA; // ║
        case 0x2554: return 0xC9; // ╔
        case 0x2557: return 0xBB; // ╗
        case 0x255A: return 0xC8; // ╚
        case 0x255D: return 0xBC; // ╝
        case 0x2560: return 0xCC; // ╠
        case 0x2563: return 0xB9; // ╣
        case 0x2566: return 0xCB; // ╦
        case 0x2569: return 0xCA; // ╩
        case 0x256C: return 0xCE; // ╬
    }

	switch (c32) {
		// Block Elements
		case 0x2588: return 0xDB; // █ FULL BLOCK
		case 0x2584: return 0xDC; // ▄ LOWER HALF BLOCK
		case 0x258C: return 0xDD; // ▌ LEFT HALF BLOCK
		case 0x2590: return 0xDE; // ▐ RIGHT HALF BLOCK
		case 0x2580: return 0xDF; // ▀ UPPER HALF BLOCK
		
		// Quarter blocks (not in standard CP437, may not render correctly)
		case 0x259D: return 0xDF; // ▝ QUADRANT UPPER RIGHT (approximate)
		case 0x2599: return 0xDB; // ▙ QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT (approximate)
		case 0x259B: return 0xDB; // ▛ QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT (approximate)
		case 0x259C: return 0xDC; // ▜ QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT (approximate)
		case 0x259E: return 0xDF; // ▞ QUADRANT UPPER RIGHT AND LOWER LEFT (approximate)
	}

	    // 6) Accented Latin letters (limited CP437 support)
    switch (c32) {
        case U'ç': return 0x87; // ç
        case U'ü': return 0x81; // ü
        case U'é': return 0x82; // é
        case U'â': return 0x83; // â
        case U'ä': return 0x84; // ä
        case U'à': return 0x85; // à
        case U'å': return 0x86; // å
        case U'ê': return 0x88; // ê
        case U'ë': return 0x89; // ë
        case U'è': return 0x8A; // è
        case U'ï': return 0x8B; // ï
        case U'î': return 0x8C; // î
        case U'ì': return 0x8D; // ì
        case U'Ä': return 0x8E; // Ä
        case U'Å': return 0x8F; // Å
        case U'É': return 0x90; // É
        case U'æ': return 0x91; // æ
        case U'Æ': return 0x92; // Æ
        case U'ô': return 0x93; // ô
        case U'ö': return 0x94; // ö
        case U'ò': return 0x95; // ò
        case U'û': return 0x96; // û
        case U'ù': return 0x97; // ù
        case U'ÿ': return 0x98; // ÿ
        case U'Ö': return 0x99; // Ö
        case U'Ü': return 0x9A; // Ü
        case U'ø': return 0x9B; // ø
        case U'£': return 0x9C; // £ // has been modified in euro sign
        case U'€': return 0x9C; // € 
        case U'Ø': return 0x9D; // Ø
        case U'×': return 0x9E; // ×
        case U'ƒ': return 0x9F; // ƒ
    }

    // 5) Fallback (unknown glyph)
    return 0x3F; // '?'
}

// UTF-8 decoding
std::vector<char32_t> utf8_to_codepoints(const std::string& s) {
    std::vector<char32_t> result;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = s[i];
        if (c < 128) {
            result.push_back(c);
            i++;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
            char32_t cp = ((c & 0x1F) << 6) | (s[i+1] & 0x3F);
            result.push_back(cp);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
            char32_t cp = ((c & 0x0F) << 12) |
                          ((s[i+1] & 0x3F) << 6) |
                          (s[i+2] & 0x3F);
            result.push_back(cp);
            i += 3;
        } else {
            result.push_back('?');
            i++;
        }
    }
    return result;
}

unsigned char map_unicode_to_font2(char32_t c32) {
    if (c32 < 256) return static_cast<unsigned char>(c32);
    switch (c32) {
        case 0x2588: return 0xDB; // map '█' to font index
        // add other special mappings if needed
        default: return '?';
    }
}

	// Decode UTF-8 to single-byte values for your font table
	std::vector<unsigned char> utf8_to_bytes(const std::string& s) {
		std::vector<unsigned char> result;
		size_t i = 0;

		while (i < s.size()) {
			unsigned char c = s[i];
			if (c < 128) {
				// ASCII
				result.push_back(c);
				i++;
			} else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
				// 2-byte UTF-8
				unsigned char byte = 0xFF & (((c & 0x1F) << 6) | (s[i+1] & 0x3F));
				result.push_back(byte);
				i += 2;
			} else {
				unsigned char byte = map_unicode_to_font(s[i]);
				// unsupported multi-byte character
				// result.push_back(static_cast<unsigned char>(0xDB));
				result.push_back(byte);
				i++;
			}
		}

		return result;
	}

static const unsigned char font8x16[256*16] = {
    #include "../inc/font8x16.inc"
};
#include <string>

// lines[row] is a UTF-8 std::string
	static void renderLineToImage(const std::string& line, size_t row,
						   unsigned char* image, int width,
						   int charWidth, int charHeight)
	{
		// Decode UTF-8 to codepoints
		// std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
		// std::u32string u32 = conv.from_bytes(line);

		auto bytes = utf8_to_bytes(line);
		int col = 0; // column counter in pixels
					 //
		auto codepoints = utf8_to_codepoints(line);
		for (char32_t cp : codepoints) {
			unsigned char c = map_unicode_to_font(cp);

			// render character
			for (int y = 0; y < charHeight; y++) {
				unsigned char bits = font8x16[c * charHeight + y];
				for (int x = 0; x < charWidth; x++) {
					if (bits & (1 << (7 - x))) {
						int px = col * charWidth + x;
						int py = row * charHeight + y;
						image[py * width + px] = 0; // black
					}
				}
			}

			col++; // increment once per character, not per byte
		}

}

void txtToPng(const std::string& txtFile, const std::string& pngFile)
{
	int charWidth = 8;
	int charHeight = 16;

    // Read text file
    std::ifstream in(txtFile);
    if (!in) {
        std::cerr << "Cannot open text file\n";
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    size_t maxLen = 0;

    while (std::getline(in, line)) {
        lines.push_back(line);
		size_t visualLen = utf8_char_count(line);
        if (visualLen > maxLen) 
            maxLen = visualLen; 
    }

    if (lines.empty()) return;

    int width  = maxLen * charWidth;
    int height = lines.size() * charHeight;

    // Create grayscale image (white background)
    std::vector<unsigned char> image(width * height, 255);
	unsigned char* ptr = image.data();

	for (size_t i = 0; i < lines.size(); i++) {
		renderLineToImage(lines[i], i, ptr, width, charWidth, charHeight);
	}

    // Write PNG
    FILE* fp = fopen(pngFile.c_str(), "wb");
    if (!fp) return;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, fp);

    png_set_IHDR(
        png, info,
        width, height,
        8,
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );

    png_write_info(png, info);

    std::vector<png_bytep> rows(height);
    for (int y = 0; y < height; y++)
        rows[y] = (png_bytep)&image[y * width];

    png_write_image(png, rows.data());
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

