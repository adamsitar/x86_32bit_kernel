#pragma once
#include <terminal/terminal.h>

/* Helper to print a signed integer. */
static inline void print_int(int num) {
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
static inline void print_uint(unsigned int num) {
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
static inline void print_hex(unsigned int num) {
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
/* Helper to print a pointer as 0x followed by lowercase hex. */
static inline void print_pointer(void *ptr) {
  terminal_writestring("0x");

  uintptr_t num = (uintptr_t)ptr;
  if (num == 0) {
    terminal_putchar('0');
    return;
  }

  char buf[32];
  size_t idx = 0;
  while (num > 0) {
    uintptr_t digit = num % 16;
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
      case 'p': {
        void *ptr = va_arg(parameters, void *);
        print_pointer(ptr);
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
  );
}
