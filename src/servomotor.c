/**************************************************************************/
/**
 *
 * @file servomotor.c
 *
 * @author Luciano Carvalho
 * @author Lucas Coelho
 *
 * @brief Code to control a servomotor.
 *
 ******************************************************************************/

/*
 * ComboLock GroupLab assignment and starter code (c) 2022-24 Christopher A. Bohn
 * ComboLock solution (c) the above-named students
 */

// clang-format off
#include <CowPi.h>
#include "servomotor.h"
#include "interrupt_support.h"
// clang-format on

#define SERVO_PIN (22)
#define PULSE_INCREMENT_uS (100)
#define SIGNAL_PERIOD_uS (20000)

static volatile int pulse_width_us;
static volatile cowpi_ioport_t *ioport = (cowpi_ioport_t *)(0xD0000000);

static void handle_timer_interrupt();

void initialize_servo() {
    cowpi_set_output_pins(1u << SERVO_PIN);
    center_servo();
    register_periodic_timer_ISR(0, PULSE_INCREMENT_uS, handle_timer_interrupt);
}

char *test_servo(char *buffer) {
    // Requirement 2: center on left button press
    if (cowpi_left_button_is_pressed()) {
        center_servo();
        sprintf(buffer, "SERVO: center");
    }
    // Requirement 3a: if button not pressed and left switch is left -> full CW
    else if (cowpi_left_switch_is_in_left_position()) {
        rotate_full_clockwise();
        sprintf(buffer, "SERVO: left");
    }
    // Requirement 3b: if button not pressed and left switch is right -> full CCW
    else {
        rotate_full_counterclockwise();
        sprintf(buffer, "SERVO: right");
    }
    return buffer;
}

void center_servo() {
    pulse_width_us = 1500;
}

void rotate_full_clockwise() {
    pulse_width_us = 2500;
}

void rotate_full_counterclockwise() {
    pulse_width_us = 500;
}

static void handle_timer_interrupt() {
    static int time_to_rise = 0;
    static int time_to_fall = 0;

    if (time_to_rise <= 0) {
        // start pulse
        ioport->output |= (1u << SERVO_PIN);
        time_to_rise += SIGNAL_PERIOD_uS;
        time_to_fall = pulse_width_us;
    }
    if (time_to_fall <= 0) {
        // end pulse
        ioport->output &= ~(1u << SERVO_PIN);
    }

    time_to_rise -= PULSE_INCREMENT_uS;
    time_to_fall -= PULSE_INCREMENT_uS;
}