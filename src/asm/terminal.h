#pragma once
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

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
  return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | (uint16_t)color << 8;
}

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t *terminal_buffer = (uint16_t *)VGA_MEMORY;

void print_greeting();
void terminal_initialize(void) {
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

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = vga_entry(c, color);
}

/* Added to implement proper scrolling (shifts lines up and clears the bottom).
 */
static void terminal_scroll(void) {
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
inline void terminal_putchar(char c) {
  if (c == '\n') {
    terminal_column = 0;
    terminal_row++;
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

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++)
    terminal_putchar(data[i]);
}

void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}

/* Helper to print a signed integer. */
static void print_int(int num) {
  if (num == 0) {
    terminal_putchar('0');
    return;
  }

  int is_negative = 0;
  if (num < 0) {
    is_negative = 1;
    num = -num; /* Note: Overflow not handled for INT_MIN. */
  }

  char buf[32];
  size_t idx = 0;
  while (num > 0) {
    buf[idx++] = '0' + (num % 10);
    num /= 10;
  }

  if (is_negative) {
    terminal_putchar('-');
  }

  /* Reverse and print. */
  for (int j = idx - 1; j >= 0; j--) {
    terminal_putchar(buf[j]);
  }
}

/* Helper to print an unsigned integer. */
static void print_uint(unsigned int num) {
  if (num == 0) {
    terminal_putchar('0');
    return;
  }

  char buf[32];
  size_t idx = 0;
  while (num > 0) {
    buf[idx++] = '0' + (num % 10);
    num /= 10;
  }

  /* Reverse and print. */
  for (int j = idx - 1; j >= 0; j--) {
    terminal_putchar(buf[j]);
  }
}

/* Helper to print an unsigned integer in lowercase hex (no 0x prefix). */
static void print_hex(unsigned int num) {
  if (num == 0) {
    terminal_putchar('0');
    return;
  }

  char buf[32];
  size_t idx = 0;
  while (num > 0) {
    unsigned int digit = num % 16;
    buf[idx++] = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
    num /= 16;
  }

  /* Reverse and print. */
  for (int j = idx - 1; j >= 0; j--) {
    terminal_putchar(buf[j]);
  }
}

/* Basic printf supporting %c, %s, %d/%i, %u, %x, %%. Handles \n via
 * terminal_putchar. */
void printf(const char *format, ...) {
  va_list parameters;
  va_start(parameters, format);

  size_t i = 0;
  while (format[i] != '\0') {
    if (format[i] != '%') {
      terminal_putchar(format[i]);
    } else {
      /* Skip the '%'. */
      i++;
      switch (format[i]) {
      case 'c': {
        char c = (char)va_arg(parameters, int); /* char promoted to int. */
        terminal_putchar(c);
        break;
      }
      case 's': {
        const char *str = va_arg(parameters, const char *);
        terminal_writestring(str);
        break;
      }
      case 'd':
      case 'i': {
        int num = va_arg(parameters, int);
        print_int(num);
        break;
      }
      case 'u': {
        unsigned int num = va_arg(parameters, unsigned int);
        print_uint(num);
        break;
      }
      case 'x': {
        unsigned int num = va_arg(parameters, unsigned int);
        print_hex(num);
        break;
      }
      case '%': {
        terminal_putchar('%');
        break;
      }
      default: {
        /* Unknown specifier: print '%' and the char. */
        terminal_putchar('%');
        terminal_putchar(format[i]);
        break;
      }
      }
    }
    i++;
  }

  va_end(parameters);
}

void print_greeting() {
  printf(" __           __ __\n"         //
         "|  |--.-----.|  |  |.-----.\n" //
         "|     |  -__||  |  ||  _  |\n" //
         "|__|__|_____||__|__||_____|\n" //
         "                           \n" //
  );
}
