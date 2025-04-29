/**************************************************************************/
/**
 *
 * @file rotary-encoder.c
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
#define PULSE_INCREMENT_uS (INT32_MAX)
#define SIGNAL_PERIOD_uS (INT32_MAX)

static int volatile pulse_width_us;

static void handle_timer_interrupt();

void initialize_servo() {
    cowpi_set_output_pins(1 << SERVO_PIN);
    center_servo();
    //    register_periodic_timer_ISR(0, PULSE_INCREMENT_uS, handle_timer_interrupt);
}

char *test_servo(char *buffer) {
    ;
    return buffer;
}

void center_servo() {
    ;
}

void rotate_full_clockwise() {
    ;
}

void rotate_full_counterclockwise() {
    ;
}

static void handle_timer_interrupt() {
    ;
}
