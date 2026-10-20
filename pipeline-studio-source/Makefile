CC = cc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
RAYLIB_DIR ?= vendor/raylib-5.5/src
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
PLATFORM_LIBS = -framework Cocoa -framework IOKit -framework OpenGL
else
PLATFORM_LIBS = -lGL -lm -lpthread -ldl -lrt -lX11
endif

.PHONY: all run test clean setup
all: build/pipeline-studio

setup:
	mkdir -p vendor
	curl -fL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz -o vendor/raylib.tar.gz
	tar -xzf vendor/raylib.tar.gz -C vendor
	$(MAKE) -C vendor/raylib-5.5/src PLATFORM=PLATFORM_DESKTOP

build:
	mkdir -p build

build/pipeline-studio: src/main.c src/pipeline.c src/pipeline.h | build
	$(CC) $(CFLAGS) -DPROJECT_ROOT='"$(CURDIR)"' -I"$(RAYLIB_DIR)" src/main.c src/pipeline.c "$(RAYLIB_DIR)/libraylib.a" $(PLATFORM_LIBS) -o $@

run: all
	./build/pipeline-studio

build/test-pipeline: tests/test_pipeline.c src/pipeline.c src/pipeline.h | build
	$(CC) $(CFLAGS) -Isrc tests/test_pipeline.c src/pipeline.c -o $@

test: build/test-pipeline
	./build/test-pipeline

clean:
	rm -rf build
