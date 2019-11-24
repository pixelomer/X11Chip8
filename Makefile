CFLAGS = -lX11 -lpthread -Wno-unused-command-line-argument -Wno-attribute-packed-for-bitfield
OUT_NAME = chip8
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=out/%.o)
XCODE_PATH = /Applications/Xcode.app
MACOS_FLAGS = -I/opt/X11/include -L/opt/X11/lib
MACOS_REQ_FOLDERS = /opt/X11 $(XCODE_PATH)
OS_READABLE = Unknown
ifneq ($(OS),Windows_NT)
ifeq ($(shell uname -s),Darwin)
OS_READABLE = macOS
CFLAGS += $(MACOS_FLAGS)
endif
endif

all: check-depends $(OUT_NAME)

check-depends:
	@echo "==> Checking requirements..."
ifeq ($(OS_READABLE),macOS)
	@for path in $(MACOS_REQ_FOLDERS); do \
		if [ ! -d $${path} ]; then \
			@echo "[!] Missing directory: $${path}"; \
		fi; \
	done;
endif
	@if [ ! -d out ]; then \
		c="mkdir -p out"; \
		printf "==> Creating output folder...\n$${c}\n"; \
		eval $${c}; \
	fi

$(OUT_NAME): $(OBJECTS)
	@echo "==> Producing executable..."
	$(CC) $(CFLAGS) -o "$@" $(OBJECTS)

out/%.o: %.c
	@mkdir -p out
	@echo "==> Compiling $<..."
	$(CC) $(CFLAGS) -c "$<" -o "$@"