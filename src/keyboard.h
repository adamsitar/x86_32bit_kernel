#include <io.h>
#include <scancodes.h>
#include <stdbool.h>
#include <terminal.h>

#define KBD_DATA_PORT 0x60
/** read_scan_code:
 * Reads a scan code from the keyboard
 *
 * @return The scan code (NOT an ASCII character!)
 */
unsigned char read_scan_code(void) { return inb(KBD_DATA_PORT); }

void handle_keyboard_interrupt() {
  unsigned char scancode = read_scan_code();
  // TODO map to ASCII or handle press/release
  printf("Keyboard scancode: 0x%X (decimal %d)\n", scancode, scancode);
  if (scancode & 0x80) {
    printf("Key released (code: 0x%X)\n",
           scancode & ~0x80); // Strip release bit
  } else {
    printf("Key pressed (code: 0x%X)\n", scancode);
  }
  // TODO: Add scancode-to-ASCII mapping table here (e.g., for 'A' = 0x1E ->
  // 'a')
}

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT                                                        \
  0x64 // Used to check if data is ready (optional for robustness)

// Scancode constants (Set 2)
#define SCANCODE_RELEASE_BIT 0x80
#define SCANCODE_LEFT_SHIFT 0x2A
#define SCANCODE_RIGHT_SHIFT 0x36
#define SCANCODE_CAPS_LOCK 0x3A
#define SCANCODE_ENTER 0x1C
#define SCANCODE_SPACE 0x39
#define SCANCODE_BACKSPACE 0x0E
#define SCANCODE_ESC 0x01

// Global state (extern for access if needed)
bool shift_pressed = false;
bool caps_lock = false;

// Function to convert scancode to ASCII (considering state)
char scancode_to_ascii(unsigned char scancode) {
  bool effective_shift =
      shift_pressed ^
      caps_lock; // XOR for Caps Lock toggling shift effect on letters
  if (scancode >= 128) {
    return 0; // Invalid or unhandled
  }

  if (effective_shift) {
    return shifted_table[scancode];
  } else {
    return unshifted_table[scancode];
  }
}

// Keyboard IRQ handler (call from exception_handler)
void keyboard_handler(void) {
  unsigned char scancode = read_scan_code();
  bool is_release = (scancode & SCANCODE_RELEASE_BIT) != 0;
  unsigned char base_scancode =
      scancode & ~SCANCODE_RELEASE_BIT; // Strip release bit

  if (is_release) {
    // Handle releases for modifiers
    if (base_scancode == SCANCODE_LEFT_SHIFT ||
        base_scancode == SCANCODE_RIGHT_SHIFT) {
      shift_pressed = false;
    }

    // Ignore other releases for now
  } else {
    // Handle presses
    if (base_scancode == SCANCODE_LEFT_SHIFT ||
        base_scancode == SCANCODE_RIGHT_SHIFT) {
      shift_pressed = true;
    } else if (base_scancode == SCANCODE_CAPS_LOCK) {
      caps_lock = !caps_lock; // Toggle
    } else {
      // Convert to ASCII and handle
      char ascii = scancode_to_ascii(base_scancode);

      if (ascii != 0) {
        // Print the character (or buffer it in a real system)
        printf("%c", ascii);
        // Special handling (optional)
        if (base_scancode == SCANCODE_ENTER) {
          printf("\n"); // Newline for Enter
        } else if (base_scancode == SCANCODE_BACKSPACE) {
          printf("\b"); // Backspace
        }

      } else {
        // Unmapped key (e.g., arrows) - log or ignore
        printf("[Unhandled scancode: 0x%x]\n", scancode);
      }
    }
  }
}
