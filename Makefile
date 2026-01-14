NAME 	= 	bill
OS = $(shell uname)

CXXFLAGS	=	-Wall -Wextra -Werror -std=c++17 -g -O0
ASANFLAGS   = -fsanitize=address
ifeq ($(OS), Darwin)
	CXX 		=	g++
else
	CXX 		=	c++
endif

# Homebrew paths for macOS
BREW_PREFIX = $(shell brew --prefix 2>/dev/null)
ifneq ($(BREW_PREFIX),)
    INCS = -I$(BREW_PREFIX)/include
    LDFLAGS += -L$(BREW_PREFIX)/lib
endif

LDFLAGS += -L/usr/local/lib -lhpdf -lpng

SRC_DIR = src/
INC_DIR = inc/
OBJ_DIR	= .objs/
INCS		+= -I$(INC_DIR) -I/usr/local/include
SRCS	= 	main.cpp\
			tools.cpp\
			img.cpp\
			PrintFact.cpp\
			ThermalPrinter.cpp\

SRCS 	:= 	$(addprefix $(SRC_DIR), $(SRCS))
OBJS	=	$(SRCS:$(SRC_DIR)%.cpp=$(OBJ_DIR)%.o)

all 			: $(NAME)

asan: CXXFLAGS += $(ASAN_CXXFLAGS)
asan: re

install: checl_lib
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
	@echo "Detecting system..."
	@if [ -f /etc/debian_version ]; then \
		echo "Debian/Ubuntu detected. Installing libharu..."; \
		sudo apt-get update && sudo apt-get install -y libhpdf-dev; \
	elif [ -f /etc/redhat-release ]; then \
		echo "RedHat/Fedora/CentOS detected. Installing libharu..."; \
		sudo yum install -y libharu-devel || sudo dnf install -y libharu-devel; \
	elif [ -f /etc/arch-release ]; then \
		echo "Arch Linux detected. Installing libharu..."; \
		sudo pacman -S --noconfirm libharu; \
	elif command -v brew >/dev/null 2>&1; then \
		echo "macOS (Homebrew) detected. Installing libharu..."; \
		brew install libharu; \
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
