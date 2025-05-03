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

typedef enum { LOCKED,
               UNLOCKED,
               ALARMED,
               CHANGING } lock_mode_t;

static lock_mode_t mode;
static uint8_t entry[3];
static uint8_t digit_index;
static uint8_t bad_tries;

static uint8_t current_digit;
static uint8_t pass_count[3];
static bool entry_in_progress;

uint8_t const *get_combination() {
    return combination;
}

static void display_entry(void) {
    char buf[9] = {' ', ' ', '-', ' ', ' ', '-', ' ', ' ', '\0'};
    char const digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    for (int i = 0; i < 3 && i < digit_index; i++) {
        uint8_t v = entry[i];
        buf[3 * i] = digits[(v / 10)];
        buf[3 * i + 1] = digits[v % 10];
    }
    display_string(4, buf);
}

void reset_entry() {
    entry[0] = entry[1] = entry[2] = 0;
    current_digit = 0;
    pass_count[0] = pass_count[1] = pass_count[2] = 0;
    entry_in_progress = true;
    display_entry();
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
    direction_t dir = get_direction();
    static bool left_prev = false;
    bool left_now = cowpi_left_button_is_pressed();

    static int digit_index = 0;
    static int pass_count[3] = {0, 0, 0};

    switch (mode) {
    case LOCKED:
        if (dir != STATIONARY) {
            int old = entry[digit_index];
            switch (digit_index) {
            case 0: // DIAL 1
                if (dir == CLOCKWISE) {
                    entry[0] = (old + 1) % 16;
                    if (old == 15)
                        pass_count[0]++;
                } else { // COUNTERCLOCKWISE
                    digit_index = 1;
                }
                break;

            case 1: // DIAL 2
                if (dir == COUNTERCLOCKWISE) {
                    entry[1] = (old + 15) % 16;
                    if (old == 0)
                        pass_count[1]++;
                } else { // CLOCKWISE
                    digit_index = 2;
                }
                break;

            case 2: // DIAL 3
                if (dir == CLOCKWISE) {
                    entry[2] = (old + 1) % 16;
                    if (old == 15)
                        pass_count[2]++;
                } else { // COUNTERCLOCKWISE → abort
                    reset_entry();
                    for (int i = 0; i < 3; i++)
                        pass_count[i] = 0;
                    digit_index = 0;
                }
                break;
            }
            display_entry();
        }

        // LEFT-BUTTON EVALUATION
        if (left_now && !left_prev && digit_index == 2) {
            bool code_match =
                entry[0] == combination[0] &&
                entry[1] == combination[1] &&
                entry[2] == combination[2];
            bool pass_rules =
                pass_count[0] >= 3 &&
                pass_count[1] >= 2 &&
                pass_count[2] >= 1;

            if (code_match && pass_rules) {
                // …UNLOCK transition, LEDs & servo…
            } else {
                // …bad-try logic, reset entry & pass_count, ALARMED if ≥3 tries…
            }
        }
        left_prev = left_now;
        break;

    case UNLOCKED:
        // …
        break;
    case ALARMED:
        // …
        break;
    case CHANGING:
        // …
        break;
    }
}
