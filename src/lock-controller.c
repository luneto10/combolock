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

static volatile lock_mode_t mode;
static volatile uint8_t entry[3];
static volatile bool entry_in_progress;

static volatile uint8_t digit_index;
static volatile uint8_t current_digit;
static volatile uint8_t bad_tries;
static volatile uint8_t pass_count[3];

static volatile char new_combo[3];
static volatile char confirm_combo[3];
static volatile uint8_t change_phase;
static volatile uint8_t change_index;
static volatile uint8_t starter_flag;

static bool is_attempt_correct(void);
static void handle_attempt(void);
static void display_entry(void);
static void display_three(volatile char *vals, uint8_t count);
static void reset_entry();

uint8_t const *get_combination() {
    return combination;
}

void force_combination_reset() {
    combination[0] = 5;
    combination[1] = 10;
    combination[2] = 15;
}

void initialize_lock_controller() {

    // Set password to 5, 10, 15 only when the program boots up.
    if (starter_flag < 1) {
        force_combination_reset();
        starter_flag++;
    }

    mode = LOCKED;
    bad_tries = 0;

    change_phase = 0;
    change_index = 0;

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

    if (starter_flag < 1) {
        initialize_lock_controller();
    }

    switch (mode) {
    case LOCKED: {
        // Display locked if no attempts have been made
        if (bad_tries < 1) {
            display_string(1, "LOCKED");
            display_string(2, "");
        }
        // Handle dial rotations for each digit
        if (dir != STATIONARY) {
            switch (current_digit) {
            case 0:
                if (dir == CLOCKWISE) {
                    entry[0] = (entry[0] + 1) % 16;
                    if (entry[0] == combination[0])
                        pass_count[0]++;
                } else if (dir == COUNTERCLOCKWISE) {
                    // Move on to digit #2
                    current_digit = 1;
                    entry[1] = entry[0];
                    pass_count[1] = 0;
                }
                break;

            case 1:
                if (dir == COUNTERCLOCKWISE) {
                    entry[1] = (entry[1] + 15) % 16;
                    if (entry[1] == combination[1])
                        pass_count[1]++;
                } else if (dir == CLOCKWISE) {
                    // Move on to digit #3
                    current_digit = 2;
                    entry[2] = entry[1];
                    pass_count[2] = 0;
                }
                break;

            case 2:
                if (dir == CLOCKWISE) {
                    entry[2] = (entry[2] + 1) % 16;
                    if (entry[2] == combination[2])
                        pass_count[2]++;
                } else if (dir == COUNTERCLOCKWISE) {
                    // Back to start
                    reset_entry();
                }
                break;
            }

            // Refresh display for 1, 2 or 3 digits
            digit_index = current_digit + 1;
            display_entry();
        }

        // If we've settled on the 3rd digit, wait for "enter"
        if (current_digit == 2 && cowpi_left_button_is_pressed()) {
            handle_attempt();
        }

        break;
    }

    case UNLOCKED: {
        rotate_full_counterclockwise();
        cowpi_deluminate_left_led();
        cowpi_illuminate_right_led();
        display_string(1, "OPEN");

        // Both buttons down: relock
        if (cowpi_left_button_is_pressed() && cowpi_right_button_is_pressed()) {
            initialize_lock_controller();
        }

        if (cowpi_left_switch_is_in_right_position() && cowpi_right_button_is_pressed()) {
            mode = CHANGING;
            display_string(2, "");
            display_string(4, "__-__-__");
        }

        break;
    }

    case ALARMED: {
        // Static across calls to keep track of blink timing & state
        static uint16_t elapsed_ms = 0;
        static bool leds_on = true;

        // Make sure the display shows ALERT!
        display_string(1, "ALERT!");

        // Accumulate our 100 ms ticks
        elapsed_ms += 100;
        if (elapsed_ms >= 250) {
            elapsed_ms = 0;
            leds_on = !leds_on;
            if (leds_on) {
                cowpi_illuminate_left_led();
                cowpi_illuminate_right_led();
            } else {
                cowpi_deluminate_left_led();
                cowpi_deluminate_right_led();
            }
        }
        break;
    }

    case CHANGING: {
        static char last_key = '\0';
        char raw = cowpi_get_keypress();
        bool new_k = (raw != '\0' && last_key == '\0');
        last_key = raw;

        // Map raw key to numeric 0–15
        uint8_t key = 0xFF;
        if (new_k) {
            if (raw >= '0' && raw <= '9') {
                key = raw - '0';
            } else if (raw >= 'A' && raw <= 'F') {
                key = 10 + (raw - 'A');
            }
        }

        // Finish change when slide‐switch -> left
        if (cowpi_left_switch_is_in_left_position()) {

            bool incomplete = (change_phase == 0) || (change_phase == 1 && change_index < 3);
            bool match = true;
            for (int i = 0; i < 3; i++) {
                if ((new_combo[i] != confirm_combo[i]) && match) {
                    match = false;
                }
            }

            if (incomplete || !match) {
                display_string(2, "NO CHANGE");
            } else {
                for (int i = 0; i < 3; i++) {
                    combination[i] = new_combo[i];
                }
                display_string(2, "CHANGED");
            }

            for (volatile uint32_t i = 0; i < 10000; ++i) {
                // Busy wait
            }

            mode = UNLOCKED;
            change_phase = change_index = 0;
            last_key = '\0';

            break;
        }

        // Still CHANGING: collect new or confirm
        if (change_phase == 0) {
            // ENTER NEW
            if (change_index == 0) {
                display_string(1, "ENTER NEW COMBO");
                for (int i = 0; i < 3; i++)
                    new_combo[i] = 0xFF;
            }
            if (key <= 15 && change_index < 3) {
                new_combo[change_index++] = key;
                display_three(new_combo, change_index);
            }
            if (change_index == 3) {
                change_phase = 1;
                change_index = 0;
                display_string(1, "RE-ENTER COMBO");
            }
        } else {
            // CONFIRM
            if (change_index == 0) {
                for (int i = 0; i < 3; i++)
                    confirm_combo[i] = 0xFF;
            }
            if (key <= 15 && change_index < 3) {
                confirm_combo[change_index++] = key;
                display_three(confirm_combo, change_index);
            }
        }
        break;
    }
    }
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

static void display_three(volatile char *vals, uint8_t count) {
    char buf[9];

    for (int i = 0; i < 3; i++) {
        if (i < count) {
            uint8_t v = vals[i];
            if (v > 15)
                v = 15;
            buf[i * 3 + 0] = '0' + (v / 10);
            buf[i * 3 + 1] = '0' + (v % 10);
        } else {
            buf[i * 3 + 0] = '_';
            buf[i * 3 + 1] = '_';
        }

        buf[i * 3 + 2] = (i < 2 ? '-' : '\0');
    }

    buf[8] = '\0';

    display_string(4, buf);
}

static void reset_entry() {
    entry[0] = entry[1] = entry[2] = 0;
    pass_count[0] = pass_count[1] = pass_count[2] = 0;
    current_digit = 0;
    digit_index = 0;
    entry_in_progress = true;
    display_entry();
}

static bool is_attempt_correct(void) {
    // return (pass_count[0] >= 3 &&
    //         pass_count[1] >= 2 &&
    //         pass_count[2] == 1) &&
    //        (entry[0] == combination[0] &&
    //         entry[1] == combination[1] &&
    //         entry[2] == combination[2]);
    return true;
}

static void handle_attempt(void) {
    if (is_attempt_correct()) {
        mode = UNLOCKED;
    } else {
        bad_tries++;
        char buf[21];
        snprintf(buf, sizeof(buf), "BAD ATTEMPT #%u", bad_tries);
        display_string(1, buf);

        // Blink LED the number of bad attempts
        for (uint8_t i = 0; i < bad_tries && bad_tries < 3; i++) {
            cowpi_illuminate_left_led();
            cowpi_illuminate_right_led();
            busy_wait_ms(250);
            cowpi_deluminate_left_led();
            cowpi_deluminate_right_led();
            busy_wait_ms(250);
        }

        if (bad_tries >= 3) {
            mode = ALARMED;
            cowpi_illuminate_left_led();
            cowpi_illuminate_right_led();
        }
        reset_entry();
    }
}
