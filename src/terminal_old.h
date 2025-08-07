#pragma once
#include <io.h>
#include <stdarg.h> /* Added for variadic arguments in printf */
#include <stddef.h>
#include <stdint.h>

/* Hardware text mode color constants. */
enum vga_color {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15,
};

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
// #define VGA_MEMORY 0xB8000
#define VGA_MEMORY 0x000B8000

/* The I/O ports */
#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT 0x3D5

/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND 15

/** fb_move_cursor:
 * Moves the cursor of the framebuffer to the given position
 *
 * @param pos The new position of the cursor
 */
static inline void move_cursor(uint8_t x, uint8_t y) {
  const size_t pos = y * VGA_WIDTH + x;
  outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
  outb(FB_DATA_PORT, ((pos >> 8) & 0x00FF));
  outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
  outb(FB_DATA_PORT, pos & 0x00FF);
}

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
  return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | (uint16_t)color << 8;
}

static inline size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
// this does prevent from raising an exception
// must be 16 bits as the VGA has 2 byte cells
uint16_t *terminal_buffer = (uint16_t *)(VGA_MEMORY + 0xC0000000);

void print_greeting();
void init_terminal(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
  }
  print_greeting();
}

static inline void terminal_setcolor(uint8_t color) { terminal_color = color; }

static inline void terminal_putentryat(char c, uint8_t color, size_t x,
                                       size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  move_cursor(x, y);
  terminal_buffer[index] = vga_entry(c, color);
}

/* Added to implement proper scrolling (shifts lines up and clears the bottom).
 */
static inline void terminal_scroll(void) {
  for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t dest_index = y * VGA_WIDTH + x;
      const size_t src_index = (y + 1) * VGA_WIDTH + x;
      terminal_buffer[dest_index] = terminal_buffer[src_index];
    }
  }
  /* Clear the last row. */
  for (size_t x = 0; x < VGA_WIDTH; x++) {
    const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(' ', terminal_color);
  }
}

/* Modified to handle '\n' specially and trigger scrolling instead of wrapping.
 */
static inline void terminal_putchar(char c) {
  if (c == '\n') {
    terminal_column = 0;
    terminal_row++;
    move_cursor(terminal_column, terminal_row);
    if (terminal_row == VGA_HEIGHT) {
      terminal_scroll();
      terminal_row = VGA_HEIGHT - 1;
    }
    return;
  }

  terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
  terminal_column++;
  if (terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    terminal_row++;
    if (terminal_row == VGA_HEIGHT) {
      terminal_scroll();
      terminal_row = VGA_HEIGHT - 1;
    }
  }
}

static inline void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++)
    terminal_putchar(data[i]);
}

static inline void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}
