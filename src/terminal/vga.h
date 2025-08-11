#pragma once
#include <stdint.h>

// VGA hardware constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY_PHYS 0xB8000
#define VGA_MEMORY (0xB8000 + 0xC0000000) // Assuming higher-half kernel

// VGA I/O ports for cursor control
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5
#define VGA_CURSOR_HIGH 14
#define VGA_CURSOR_LOW 15

// VGA color constants
typedef enum {
  VGA_BLACK = 0,
  VGA_BLUE = 1,
  VGA_GREEN = 2,
  VGA_CYAN = 3,
  VGA_RED = 4,
  VGA_MAGENTA = 5,
  VGA_BROWN = 6,
  VGA_LIGHT_GREY = 7,
  VGA_DARK_GREY = 8,
  VGA_LIGHT_BLUE = 9,
  VGA_LIGHT_GREEN = 10,
  VGA_LIGHT_CYAN = 11,
  VGA_LIGHT_RED = 12,
  VGA_LIGHT_MAGENTA = 13,
  VGA_YELLOW = 14,
  VGA_WHITE = 15
} vga_color_t;

// VGA character cell structure
typedef struct {
  uint8_t character;
  uint8_t attribute;
} __attribute__((packed)) vga_cell_t;

static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg) {
  return (bg << 4) | fg;
}

static inline uint16_t vga_make_entry(char c, uint8_t color) {
  return (uint16_t)c | ((uint16_t)color << 8);
}
