#include "ThermalPrinter.hpp"
#include <vector>
#include <png.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cerrno>
#include <iostream>
#include <termios.h>  // For tcdrain()
					  //
std::vector<unsigned char> CMD_HT        = {0x09};              // HT
std::vector<unsigned char> CMD_LF        = {0x0A};              // LF
std::vector<unsigned char> CMD_CR        = {0x0D};              // CR
std::vector<unsigned char> CMD_INIT      = {0x1B, 0x40};        // ESC @

std::vector<unsigned char> CMD_FEED_DOTS = {0x1B, 0x4A, 0x00};   // ESC J n
std::vector<unsigned char> CMD_FEED_LINES= {0x1B, 0x64, 0x01};  // ESC d n

std::vector<unsigned char> CMD_LINE_SPACE_DEFAULT = {0x1B, 0x32};      // ESC 2
std::vector<unsigned char> CMD_LINE_SPACE_SET     = {0x1B, 0x33, 0x00}; // ESC 3 n

std::vector<unsigned char> CMD_ALIGN_LEFT   = {0x1B, 0x61, 0x00}; // ESC a 0
std::vector<unsigned char> CMD_ALIGN_CENTER = {0x1B, 0x61, 0x01}; // ESC a 1
std::vector<unsigned char> CMD_ALIGN_RIGHT  = {0x1B, 0x61, 0x02}; // ESC a 2

std::vector<unsigned char> CMD_ABS_POS = {0x1B, 0x24, 0x00, 0x00}; // ESC $ nL nH

std::vector<unsigned char> CMD_FONT_A = {0x1B, 0x4D, 0x00}; // ESC M 0
std::vector<unsigned char> CMD_FONT_B = {0x1B, 0x4D, 0x01}; // ESC M 1

std::vector<unsigned char> CMD_BOLD_ON  = {0x1B, 0x45, 0x01}; // ESC E 1
std::vector<unsigned char> CMD_BOLD_OFF = {0x1B, 0x45, 0x00}; // ESC E 0

std::vector<unsigned char> CMD_DOUBLE_STRIKE_ON  = {0x1B, 0x47, 0x01}; // ESC G 1
std::vector<unsigned char> CMD_DOUBLE_STRIKE_OFF = {0x1B, 0x47, 0x00}; // ESC G 0

std::vector<unsigned char> CMD_ROTATE_ON  = {0x1B, 0x56, 0x01}; // ESC V 1
std::vector<unsigned char> CMD_ROTATE_OFF = {0x1B, 0x56, 0x00}; // ESC V 0

std::vector<unsigned char> CMD_UNDERLINE_OFF = {0x1B, 0x2D, 0x00}; // ESC - 0
std::vector<unsigned char> CMD_UNDERLINE_1   = {0x1B, 0x2D, 0x01}; // ESC - 1
std::vector<unsigned char> CMD_UNDERLINE_2   = {0x1B, 0x2D, 0x02}; // ESC - 2

std::vector<unsigned char> CMD_CHAR_SPACING = {0x1B, 0x20, 0x00}; // ESC SP n

std::vector<unsigned char> CMD_BIT_IMAGE   = {0x1B, 0x2A}; // ESC *
std::vector<unsigned char> CMD_DL_IMAGE    = {0x1D, 0x2A}; // GS *
std::vector<unsigned char> CMD_PRINT_DLIMG = {0x1D, 0x2F}; // GS /
std::vector<unsigned char> CMD_PRINT_LINE  = {0x1D, 0x22}; // GS "
														   //
std::vector<unsigned char> CMD_BAR_HRI_POS = {0x1D, 0x48, 0x00}; // GS H n
std::vector<unsigned char> CMD_BAR_HEIGHT  = {0x1D, 0x68, 0x00}; // GS h n
std::vector<unsigned char> CMD_BAR_WIDTH   = {0x1D, 0x77, 0x00}; // GS w n
std::vector<unsigned char> CMD_BAR_FONT    = {0x1D, 0x66, 0x00}; // GS f n
std::vector<unsigned char> CMD_BAR_PRINT   = {0x1D, 0x6B};      // GS k


bool isPrinterReady(int fd) {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;  // Immediate return
    
    int ready = select(fd + 1, NULL, &writefds, NULL, &timeout);
    
    return (ready > 0);  // true if ready to accept more data
}

// Default constructor
ThermalPrinter::ThermalPrinter(const std::string& path): _devicePath(path) {

	if (!open_device(path)) {
		throw std::runtime_error("Cannot open device.");
	}
	// Initialize printer
	std::vector<unsigned char> init = {0x1B, 0x40};
	write(_fd, init.data(), init.size());
	usleep(50000);
}

// Destructor
ThermalPrinter::~ThermalPrinter(void) {
	if (_fd >= 0) {
		::close(_fd);
	}
}

ThermalPrinter::ThermalPrinter(void): _fd(-1) {
	return;
}

bool ThermalPrinter::init(const std::string& path) {
	if (!open_device(path))
		return false;
	writeCommand(CMD_INIT);
	std::cout << "Printer initialized !\n";
	return true;
}

void ThermalPrinter::align(AlignDef pos) {
	if (pos == LEFT) {
		writeCommand(CMD_ALIGN_LEFT);
	} else if (pos == RIGHT) {
		writeCommand(CMD_ALIGN_RIGHT);
	} else if (pos == CENTER) {
		writeCommand(CMD_ALIGN_CENTER);
	}
}

void ThermalPrinter::close_device() {
	    if (_fd < 0)
			return;
        std::vector<unsigned char> feed = {0x1B, 0x64, 0x05};  // Feed 5 lines
        writeCommand(feed);
        usleep(300000);  // Wait 300ms
						 //
        std::cout << "Closing printer gracefully..." << std::endl;

        tcflush(_fd, TCOFLUSH);
		reset();
        // 1. Drain output buffer (wait for transmission)
        tcdrain(_fd);
        
        // 2. Feed some paper to finish cleanly

		struct termios tio;
		if (tcgetattr(_fd, &tio) == 0) {
			cfmakeraw(&tio);
			tcsetattr(_fd, TCSANOW, &tio);
		}
        // 3. Reset printer to defaults
        usleep(100000);
        
        // 4. Close file descriptor
        ::close(_fd);
        _fd = -1;
        
        std::cout << "✓ Printer closed" << std::endl;
}

bool ThermalPrinter::is_open() {
	if (_fd < 0) {
		return false;
	}
	return true;
}

void ThermalPrinter::reset() {
    // ESC @ (0x1B 0x40) - Initialize printer
    std::vector<unsigned char> cmd = {0x1B, 0x40};
    writeCommand(cmd);
    usleep(50000);  // Wait 50ms for reset to complete
}

// void ThermalPrinter::writeCommand(const std::vector<unsigned char>& cmd, int timeout_sec)
// {
// 	if (!check_connection()) {
// 		return;
// 	}
//     if (_fd < 0) {
//         std::cout << "ERROR: fd is invalid: " << _fd << std::endl;
//         return;
//     }
//     if (cmd.empty()) {
//         std::cout << "ERROR: cmd is empty" << std::endl;
//         return;
//     }
//
//     std::cout << "fd = " << _fd << ", cmd size = " << cmd.size() << std::endl;
//
//     // Check if fd is still valid
//     int flags = fcntl(_fd, F_GETFL);
//     if (flags == -1) {
//         std::cout << "ERROR: fcntl failed: " << strerror(errno) << std::endl;
//         _fd = -1;
//         return;
//     }
//     std::cout << "File flags: " << flags << " (O_NONBLOCK=" << (flags & O_NONBLOCK) << ")" << std::endl;
//
//     // Try select first
//     std::cout << "Calling select()..." << std::endl;
//     fd_set wfds;
//     FD_ZERO(&wfds);
//     FD_SET(_fd, &wfds);
//
//     struct timeval tv;
//     tv.tv_sec = timeout_sec;
//     tv.tv_usec = 0;
//
//     int ret = select(_fd + 1, nullptr, &wfds, nullptr, &tv);
//     std::cout << "select() returned: " << ret << std::endl;
//
//     if (ret == -1) {
//         std::cerr << "select() error: " << strerror(errno) << std::endl;
//         return;
//     } else if (ret == 0) {
//         std::cerr << "select() timeout" << std::endl;
//         return;
//     }
//
//     if (!FD_ISSET(_fd, &wfds)) {
//         std::cerr << "fd not ready after select()" << std::endl;
//         return;
//     }
//
//     std::cout << "Calling write()..." << std::endl;
//     std::cout.flush();  // Force output before potentially blocking
//
//     ssize_t n = write(_fd, cmd.data(), cmd.size());
//
//     std::cout << "write() returned: " << n << std::endl;
//
//     if (n < 0) {
//         std::cerr << "write() error: " << strerror(errno) << " (errno=" << errno << ")" << std::endl;
//     } else {
//         std::cout << "Successfully wrote " << n << " bytes" << std::endl;
//     }
//
//     std::cout << "=== writeCommand END ===" << std::endl;
// }

// bool ThermalPrinter::check_connection() {
//     int bytes_waiting;
// 	if (_fd < 0) {
// 		return false;
// 	}
//     if (ioctl(_fd, TIOCOUTQ, &bytes_waiting) < 0 || bytes_waiting > 4096) {
//         std::cerr << "Connection dead or stuck" << std::endl;
//         ::close(_fd);
//         _fd = -1;
//         return false;
//     }
// 	return (true);
// }

// void ThermalPrinter::writeCommand(const std::vector<unsigned char>& cmd, int timeout_sec)
// {
//     if (_fd < 0 || cmd.empty())
// 		return;
//     // Check if device is still valid
//     struct stat st;
//     if (fstat(_fd, &st) < 0) {
//         std::cerr << "Device descriptor invalid: " << strerror(errno) << "\n";
//         _fd = -1;
//         return;
//     }
//
// 	std::cout << "Write command ...\n";
// 	fd_set wfds;
// 	FD_ZERO(&wfds);
// 	FD_SET(_fd, &wfds);
//
// 	struct timeval tv;
// 	tv.tv_sec = timeout_sec;
// 	tv.tv_usec = 0;
//
// 	int ret = select(_fd + 1, nullptr, &wfds, nullptr, &tv);
// 	if (ret == -1) {
// 		std::cerr << "select() error on printer: " << strerror(errno) << "\n";
// 		return;
// 	} else if (ret == 0) {
// 		std::cerr << "Printer write timeout\n";
// 		return;  // Timeout, stop trying
// 	}
//
// 	if (FD_ISSET(_fd, &wfds)) {
// 		ssize_t n = write(_fd, cmd.data(), cmd.size());
// 		if (n < 0) {
// 			if (errno == EAGAIN || errno == EWOULDBLOCK) {
// 				std::cerr << "Printer busy: " << strerror(errno) << "\n";
// 				return;
// 			} else {
// 				std::cerr << "Printer write error: " << strerror(errno) << "\n";
// 				return;
// 			}
// 		}
// 	} else {
// 		std::cerr << "Printer not ready.\n";
// 	}
// 	std::cout << "done command ...\n";
// }


void ThermalPrinter::writeCommand(const std::vector<unsigned char>& cmd, int timeout) {
	(void)timeout;
	if (_fd < 0) return;
	write(_fd, cmd.data(), cmd.size());
}

bool ThermalPrinter::open_device(const std::string& path) {
	_fd = ::open(path.c_str(), O_RDWR);
	if (_fd < 0) {
		return false;
	}
	return true;
}

// bool ThermalPrinter::open_device2(const std::string& path) {
//     if (path.empty()) {
//         return false;
//     }
//     close_device();
//
//     std::cout << "Opening device at: " << path << std::endl;
//
//     // Try to open with retry logic
//     int retry_count = 0;
//     const int max_retries = 3;
//
//     while (retry_count < max_retries) {
//         _fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
//
//         if (_fd >= 0) {
//             break;  // Success
//         }
//
//         if (errno == EBUSY) {
//             std::cerr << "Device busy, retrying in 1 second..." << std::endl;
//             usleep(1000000);  // Wait 1 second
//             retry_count++;
//         } else {
//             std::cerr << "Error opening device: " << strerror(errno) << std::endl;
//             return false;
//         }
//     }
//
//     if (_fd < 0) {
//         std::cerr << "Failed to open device after " << max_retries << " attempts" << std::endl;
//         return false;
//     }
//
//     // Configure terminal settings
//     struct termios tio;
//     if (tcgetattr(_fd, &tio) == 0) {
//         cfmakeraw(&tio);
//         tio.c_cflag |= (CLOCAL | CREAD);
//         tio.c_cflag &= ~CRTSCTS;  // Disable hardware flow control
//         tcflush(_fd, TCIOFLUSH);
//         tcsetattr(_fd, TCSANOW, &tio);
//     }
//
//     // Switch to blocking mode
//     int flags = fcntl(_fd, F_GETFL, 0);
//     fcntl(_fd, F_SETFL, flags & ~O_NONBLOCK);
//
//     _devicePath = path;
//     usleep(100000);  // Give device time to stabilize
//
//     return true;
// }
//
// bool ThermalPrinter::open_device(const std::string& path) {
// 	if (path.empty()) {
// 		return false;
// 	}
// 	close_device();
// 	std::cout << "openning device at: " << path << std::endl;;
// 	_fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
// 	if (_fd < 0) {
//         if (errno == EBUSY || errno == EAGAIN) {
//             std::cerr << "Error: Printer is already in use" << std::endl;
//         } else {
//             std::cerr << "Error: Cannot open device " << path << std::endl;
//             std::cerr << "Reason: " << strerror(errno) << std::endl;
//         }
//         return false;
//     }
// 	// Switch back to blocking mode for normal I/O
//     int flags = fcntl(_fd, F_GETFL, 0);
//     fcntl(_fd, F_SETFL, flags & ~O_NONBLOCK);
// 	_devicePath = path;
// 	usleep(50000);
// 	return true;
// }

void ThermalPrinter::printPNG(const std::string& filename) {
	std::cout << "Printing PNG\n";
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        std::cerr << "Cannot open PNG file\n";
        return;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, fp);
    png_read_info(png, info);

    int width  = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte depth = png_get_bit_depth(png, info);

    if (depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        png_set_rgb_to_gray_fixed(png, 1, -1, -1);
    if (color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png);

	std::cout << "read update PNG\n";
    png_read_update_info(png, info);

	if (width > MAX_PRINT_WIDTH * 8) {
		std::cerr << "Image is too wide (" << width << ").\n";
		return;
	}

    std::vector<png_bytep> rows(height);
    for (int y = 0; y < height; y++)
        rows[y] = (png_bytep)malloc(png_get_rowbytes(png, info));

    png_read_image(png, rows.data());
    fclose(fp);

    int widthBytes = (width + 7) / 8;

    // GS v 0
    std::vector<unsigned char> header = {
        0x1D, 0x76, 0x30, 0x00,
        (unsigned char)(widthBytes & 0xFF),
        (unsigned char)((widthBytes >> 8) & 0xFF),
        (unsigned char)(height & 0xFF),
        (unsigned char)((height >> 8) & 0xFF)
    };

	// writeCommand(header);
    // write(_fd, header.data(), header.size());

	std::vector<unsigned char> imageBuffer(height * widthBytes, 0);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < widthBytes; x++) {
            unsigned char byte = 0;
            for (int b = 0; b < 8; b++) {
                int px = x * 8 + b;
                if (px < width) {
                    unsigned char pixel = rows[y][px];
                    if (pixel < 128)
                        byte |= (1 << (7 - b));
                }
            }
			imageBuffer[y * widthBytes + x] = byte;
        }
        free(rows[y]);
    }

	std::cout << "Write header to printer\n";
	writeCommand(header);
	std::cout << "Write buffer to printer\n";
	writeCommand(imageBuffer);
	// writeCommand(CMD_FEED_LINES);
	std::vector<unsigned char> feed = {0x1B, 0x64, 0xF};  // Feed 5 lines
	writeCommand(feed);
    png_destroy_read_struct(&png, &info, nullptr);
	std::cout << "Done printing\n";
}

