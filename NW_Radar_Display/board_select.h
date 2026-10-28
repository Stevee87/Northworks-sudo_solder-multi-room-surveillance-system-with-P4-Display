#pragma once

// Arduino's M5Tab5 target selects this adapter automatically. For the
// Generic ESP32-P4 Dev Module selects this project's Elecrow V1.0 adapter.
// It is not a universal configuration for other ESP32-P4 boards.
#if defined(ARDUINO_ESP32P4_DEV) && !defined(BOARD_M5STACK_TAB5)
#ifndef BOARD_ELECROW_P4
#define BOARD_ELECROW_P4
#endif
#endif
#if defined(ARDUINO_M5STACK_TAB5) && !defined(BOARD_ELECROW_P4)
#ifndef BOARD_M5STACK_TAB5
#define BOARD_M5STACK_TAB5
#endif
#endif
#if defined(BOARD_M5STACK_TAB5) == defined(BOARD_ELECROW_P4)
#error "Select exactly one board: BOARD_M5STACK_TAB5 or BOARD_ELECROW_P4"
#endif
