NAME 	= 	bill
OS = $(shell uname)
ifeq ($(OS), Darwin)
	CXX 		=	g++
else
	CXX 		=	c++
endif

# Homebrew paths for macOS
BREW_PREFIX = $(shell brew --prefix 2>/dev/null)
ifneq ($(BREW_PREFIX),)
    PKG_CONFIG_PATH := $(BREW_PREFIX)/lib/pkgconfig
    export PKG_CONFIG_PATH
    INCS    = -I$(BREW_PREFIX)/include
	INCS += -I$(BREW_PREFIX)/include/cairo
    INCS += -I$(BREW_PREFIX)/include/pango-1.0
    INCS += -I$(BREW_PREFIX)/include/glib-2.0
    INCS += -I$(BREW_PREFIX)/lib/glib-2.0/include
	INCS += -I$(BREW_PREFIX)/include/harfbuzz
    LDFLAGS += -L$(BREW_PREFIX)/lib
endif

PKG_CFLAGS  = $(shell pkg-config --cflags cairo pango pangocairo)
PKG_LIBS    = $(shell pkg-config --libs   cairo pango pangocairo)

CXXFLAGS	=	-Wall -Wextra -Werror -std=c++20 -g -O0 $(INCS) $(PKG_CFLAGS)
ASANFLAGS   = -fsanitize=address

LDFLAGS  += -L/usr/local/lib -lpng -lsqlite3 $(PKG_LIBS) -lcurl

SRC_DIR = src/
INC_DIR = inc/
OBJ_DIR	= .objs/
INCS		+= -I$(INC_DIR) -I/usr/local/include
SRCS	= 	main.cpp\
			PrintFact.cpp\
			Risography.cpp\
			Database.cpp\
			tools.cpp\
			Facture.cpp\
			config.cpp\
			Client.cpp\
			smtp.cpp\

SRCS 	:= 	$(addprefix $(SRC_DIR), $(SRCS))
OBJS	=	$(SRCS:$(SRC_DIR)%.cpp=$(OBJ_DIR)%.o)

all 			: $(NAME)

asan: CXXFLAGS += $(ASAN_CXXFLAGS)
asan: re

install: check_lib
	#brew install libharu
	#brew install libpng

# Check if libharu is installed
check_lib:
	@echo "Checking for libharu..."
	@if ! pkg-config --exists libharu 2>/dev/null && ! ldconfig -p | grep -q libhpdf; then \
		echo "libharu not found. Installing..."; \
		$(MAKE) install_lib; \
	else \
		echo "libharu is already installed."; \
	fi

# Install libharu based on the system
install_lib:
	export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig" && echo "Detecting system..."
	@if [ -f /etc/debian_version ]; then \
		echo "Debian/Ubuntu detected. Installing libharu..."; \
		sudo apt-get update && sudo apt-get install -y libhpdf-dev libcairo2-dev libpango1.0-dev ; \
	elif [ -f /etc/redhat-release ]; then \
		echo "RedHat/Fedora/CentOS detected. Installing libharu..."; \
		sudo yum install -y libharu-devel || sudo dnf install -y libharu-devel libcairo2-dev libpango1.0-dev ;  \
	elif [ -f /etc/arch-release ]; then \
		echo "Arch Linux detected. Installing libharu..."; \
		sudo pacman -S --noconfirm libharu libcairo2-dev libpango1.0-dev ; \
	elif command -v brew >/dev/null 2>&1; then \
		echo "macOS (Homebrew) detected. Installing libharu..."; \
		brew install libharu cairo pango pkg-config pkg-config;\
	else \
		echo "Unsupported system. Please install libharu manually."; \
		exit 1; \
	fi

$(OBJ_DIR)%.o	: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

$(NAME)			: $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $(NAME) $(LDFLAGS)

clean			:
	@echo "Cleaning object..."
	@rm -rf $(OBJ_DIR)

fclean			: clean
	@echo "Cleaning everything..."
	@rm -f $(NAME)

re		: fclean all

.PHONY	: all clean fclean re 
