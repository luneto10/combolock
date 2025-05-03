/**************************************************************************/
/**
 *
 * @file lock-controller.c
 *
 * @author Luciano Carvalho
 * @author Lucas Coelho
 *
 * @brief Code to implement the "combination lock" mode.
 *
 ******************************************************************************/

/*
 * ComboLock GroupLab assignment and starter code (c) 2022-24 Christopher A. Bohn
 * ComboLock solution (c) the above-named students
 */

// clang-format off
#include <CowPi.h>
#include "display.h"
#include "lock-controller.h"
#include "rotary-encoder.h"
#include "servomotor.h"
// clang-format on



static uint8_t combination[3] __attribute__((section(".uninitialized_ram.")));

typedef enum { LOCKED, UNLOCKED, ALARMED, CHANGING } lock_mode_t;

static lock_mode_t mode;
static uint8_t entry[3];
static uint8_t digit_index;
static uint8_t bad_tries;

uint8_t const *get_combination() {
    return combination;
}

void reset_entry() {
    entry[0] = 0;
    entry[1] = 0;
    entry[2] = 0;
    display_entry();
}

static void display_entry(void) {
    char buf[9] = { ' ', ' ', '-', ' ', ' ', '-', ' ', ' ', '\0' };
    char const digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    for (int i = 0; i < 3 && i < digit_index; i++)
    {
        uint8_t v = entry[i];
        buf[3 * i] = digits[(v / 10)];
        buf[3 * i + 1] = digits[v % 10];
    }
    display_string(4, buf);
}

void force_combination_reset() {
    combination[0] = 5;
    combination[1] = 10;
    combination[2] = 15;
}

void initialize_lock_controller() {
    mode = LOCKED;
    bad_tries = 0;

    reset_entry();

    cowpi_illuminate_left_led();
    cowpi_deluminate_right_led();

    rotate_full_clockwise(); 

    if (combination[0] > 15 || combination[1] > 15 || combination[2] > 15) {
        force_combination_reset();
    }

}

void control_lock() {
    ;
}