#pragma once
#include "util/printf.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <terminal/vga.h>
#include <util/io.h>
#include <util/util.h>

// Scrollback buffer configuration
#define SCROLLBACK_LINES 100 // Number of lines to keep in history
#define SCROLLBACK_SIZE (SCROLLBACK_LINES * VGA_WIDTH)

typedef struct {
  // Current display state
  uint16_t *framebuffer;
  size_t cursor_x;
  size_t cursor_y;
  uint8_t color;

  // Scrollback buffer
  uint16_t *scrollback_buffer;
  size_t scrollback_head;  // Index of oldest line in circular buffer
  size_t scrollback_count; // Number of lines currently in scrollback
  size_t view_offset;      // Current viewing offset (0 = bottom/newest)

  // State flags
  bool cursor_visible;
  bool in_scrollback_mode;
} terminal_t;

// Global terminal instance
static terminal_t term;

// Static helper functions
static void update_hardware_cursor(void);
static void scroll_display(void);
static void save_line_to_scrollback(size_t line_num);
static void refresh_from_scrollback(void);

void terminal_clear(void);

void terminal_init(void) {
  // Initialize framebuffer
  term.framebuffer = (uint16_t *)VGA_MEMORY;
  term.cursor_x = 0;
  term.cursor_y = 0;
  term.color = vga_make_color(VGA_LIGHT_GREY, VGA_BLACK);
  term.cursor_visible = true;
  term.in_scrollback_mode = false;

  // Allocate scrollback buffer (you'll need to implement kmalloc)
  // For now, we'll use a static buffer
  static uint16_t scrollback_storage[SCROLLBACK_SIZE];
  term.scrollback_buffer = scrollback_storage;
  term.scrollback_head = 0;
  term.scrollback_count = 0;
  term.view_offset = 0;

  // Clear the screen
  terminal_clear();
  update_hardware_cursor();
  print_greeting();
}

void terminal_clear(void) {
  for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    term.framebuffer[i] = vga_make_entry(' ', term.color);
  }
  term.cursor_x = 0;
  term.cursor_y = 0;
  update_hardware_cursor();
}

static void update_hardware_cursor(void) {
  if (!term.cursor_visible || term.in_scrollback_mode) {
    // Hide cursor by moving it off-screen
    uint16_t pos = VGA_WIDTH * VGA_HEIGHT;
    outb(VGA_CTRL_PORT, VGA_CURSOR_HIGH);
    outb(VGA_DATA_PORT, (pos >> 8) & 0xFF);
    outb(VGA_CTRL_PORT, VGA_CURSOR_LOW);
    outb(VGA_DATA_PORT, pos & 0xFF);
  } else {
    uint16_t pos = term.cursor_y * VGA_WIDTH + term.cursor_x;
    outb(VGA_CTRL_PORT, VGA_CURSOR_HIGH);
    outb(VGA_DATA_PORT, (pos >> 8) & 0xFF);
    outb(VGA_CTRL_PORT, VGA_CURSOR_LOW);
    outb(VGA_DATA_PORT, pos & 0xFF);
  }
}

static void save_line_to_scrollback(size_t line_num) {
  if (line_num >= VGA_HEIGHT)
    return;

  // Calculate position in circular buffer
  size_t buffer_line =
      (term.scrollback_head + term.scrollback_count) % SCROLLBACK_LINES;

  // Copy line to scrollback buffer
  uint16_t *src = &term.framebuffer[line_num * VGA_WIDTH];
  uint16_t *dst = &term.scrollback_buffer[buffer_line * VGA_WIDTH];
  memcpy(dst, src, VGA_WIDTH * sizeof(uint16_t));

  // Update scrollback metadata
  if (term.scrollback_count < SCROLLBACK_LINES) {
    term.scrollback_count++;
  } else {
    term.scrollback_head = (term.scrollback_head + 1) % SCROLLBACK_LINES;
  }
}

static void scroll_display(void) {
  // Save the top line to scrollback before scrolling
  save_line_to_scrollback(0);

  // Shift all lines up by one
  for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
    memcpy(&term.framebuffer[y * VGA_WIDTH],
           &term.framebuffer[(y + 1) * VGA_WIDTH],
           VGA_WIDTH * sizeof(uint16_t));
  }

  // Clear the last line
  for (size_t x = 0; x < VGA_WIDTH; x++) {
    term.framebuffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
        vga_make_entry(' ', term.color);
  }
}

void terminal_scrollbottom(void);
void terminal_putchar(char c) {
  // Exit scrollback mode when new output arrives
  if (term.in_scrollback_mode) {
    terminal_scrollbottom();
  }

  switch (c) {
  case '\n':
    term.cursor_x = 0;
    term.cursor_y++;
    break;

  case '\r':
    term.cursor_x = 0;
    break;

  case '\t':
    term.cursor_x = (term.cursor_x + 8) & ~7;
    break;

  case '\b':
    if (term.cursor_x > 0) {
      term.cursor_x--;
      term.framebuffer[term.cursor_y * VGA_WIDTH + term.cursor_x] =
          vga_make_entry(' ', term.color);
    }
    break;

  default:
    if (c >= 32 && c < 127) { // Printable ASCII
      term.framebuffer[term.cursor_y * VGA_WIDTH + term.cursor_x] =
          vga_make_entry(c, term.color);
      term.cursor_x++;
    }
    break;
  }

  // Handle line wrap
  if (term.cursor_x >= VGA_WIDTH) {
    term.cursor_x = 0;
    term.cursor_y++;
  }

  // Handle screen scroll
  if (term.cursor_y >= VGA_HEIGHT) {
    scroll_display();
    term.cursor_y = VGA_HEIGHT - 1;
  }

  update_hardware_cursor();
}

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    terminal_putchar(data[i]);
  }
}

void terminal_writestring(const char *str) {
  while (*str) {
    terminal_putchar(*str++);
  }
}

void terminal_setcolor(vga_color_t fg, vga_color_t bg) {
  term.color = vga_make_color(fg, bg);
}

static void refresh_from_scrollback(void) {
  // Clear screen first
  for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    term.framebuffer[i] = vga_make_entry(' ', term.color);
  }

  // Determine which lines to display
  size_t lines_to_show = VGA_HEIGHT;
  size_t start_line = 0;

  if (term.scrollback_count > 0) {
    // Calculate starting position based on view_offset
    if (term.view_offset < VGA_HEIGHT) {
      // Showing some current screen content
      lines_to_show = VGA_HEIGHT - term.view_offset;
      start_line = term.scrollback_count - lines_to_show;
    } else {
      // Showing only scrollback content
      start_line = term.scrollback_count - term.view_offset;
      lines_to_show = VGA_HEIGHT;
    }

    // Copy lines from scrollback to display
    for (size_t i = 0; i < lines_to_show && i < term.scrollback_count; i++) {
      size_t scrollback_line =
          (term.scrollback_head + start_line + i) % SCROLLBACK_LINES;
      memcpy(&term.framebuffer[i * VGA_WIDTH],
             &term.scrollback_buffer[scrollback_line * VGA_WIDTH],
             VGA_WIDTH * sizeof(uint16_t));
    }
  }
}

void terminal_scrollup(size_t lines) {
  size_t max_offset = term.scrollback_count;

  term.view_offset += lines;
  if (term.view_offset > max_offset) {
    term.view_offset = max_offset;
  }

  term.in_scrollback_mode = (term.view_offset > 0);
  refresh_from_scrollback();
  update_hardware_cursor();
}

void terminal_scrolldown(size_t lines) {
  if (term.view_offset >= lines) {
    term.view_offset -= lines;
  } else {
    term.view_offset = 0;
  }

  term.in_scrollback_mode = (term.view_offset > 0);
  if (!term.in_scrollback_mode) {
    // Restore current screen content
    terminal_clear(); // You'd want to restore the actual current content here
  } else {
    refresh_from_scrollback();
  }
  update_hardware_cursor();
}

void terminal_scrolltop(void) {
  term.view_offset = term.scrollback_count;
  term.in_scrollback_mode = true;
  refresh_from_scrollback();
  update_hardware_cursor();
}

void terminal_scrollbottom(void) {
  term.view_offset = 0;
  term.in_scrollback_mode = false;
  // Restore current screen content
  terminal_clear(); // You'd want to restore the actual current content here
  update_hardware_cursor();
}

bool terminal_in_scrollback(void) { return term.in_scrollback_mode; }

// Additional utility functions
void terminal_setcursor(size_t x, size_t y) {
  if (x < VGA_WIDTH && y < VGA_HEIGHT) {
    term.cursor_x = x;
    term.cursor_y = y;
    update_hardware_cursor();
  }
}

void terminal_getcursor(size_t *x, size_t *y) {
  if (x)
    *x = term.cursor_x;
  if (y)
    *y = term.cursor_y;
}

void terminal_showcursor(bool show) {
  term.cursor_visible = show;
  update_hardware_cursor();
}
