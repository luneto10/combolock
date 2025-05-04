/**************************************************************************/
/**
 *
 * @file rotary-encoder.c
 *
 * @author Luciano Carvalho
 * @author Lucas Coelho
 *
 * @brief Code to determine the direction that a rotary encoder is turning.
 *
 ******************************************************************************/

/*
 * ComboLock GroupLab assignment and starter code (c) 2022-24 Christopher A. Bohn
 * ComboLock solution (c) the above-named students
 */

// clang-format off
#include <CowPi.h>
#include "interrupt_support.h"
#include "rotary-encoder.h"
#include "display.h"
// clang-format on

#define A_WIPER_PIN (16)
#define B_WIPER_PIN (A_WIPER_PIN + 1)

typedef enum {
    HIGH_HIGH,
    HIGH_LOW,
    LOW_LOW,
    LOW_HIGH,
    UNKNOWN
} rotation_state_t;

char debug_buffer[22];
const char *states[] = {"L_L", "L_H", "H_L", "H_H", "U_K"};
volatile cowpi_ioport_t *ioport = (cowpi_ioport_t *)(0xD0000000);
static rotation_state_t volatile state;
static direction_t volatile direction = STATIONARY;
static int volatile clockwise_count = 0;
static int volatile counterclockwise_count = 0;

static void handle_quadrature_interrupt();

void initialize_rotary_encoder() {
    cowpi_set_pullup_input_pins((1 << A_WIPER_PIN) | (1 << B_WIPER_PIN));

    state = get_quadrature();
    direction = STATIONARY;

    clockwise_count = 0;
    counterclockwise_count = 0;

    register_pin_ISR((1 << A_WIPER_PIN) | (1 << B_WIPER_PIN), handle_quadrature_interrupt);
}

uint8_t get_quadrature() {
    uint8_t A = (ioport->input >> A_WIPER_PIN) & 0x1;
    uint8_t B = (ioport->input >> B_WIPER_PIN) & 0x1;
    return (B << 1) | (A << 0);
}

char *count_rotations(char *buffer) {
    sprintf(buffer, "CW:%2d CCW:%2d", clockwise_count, counterclockwise_count);
    return buffer;
}

direction_t get_direction() {
    direction_t direction_copy = direction;
    direction = STATIONARY;
    return direction_copy;
}

static void handle_quadrature_interrupt() {
    static rotation_state_t last_state = HIGH_HIGH; // Initialize to a valid state
    uint8_t quadrature = get_quadrature();
    rotation_state_t next_state = state; // Initialize next_state to current state

    switch (quadrature) {
    case 0b00: // LOW_LOW
        if (state == HIGH_LOW && last_state == HIGH_HIGH) {
            clockwise_count++;
            direction = CLOCKWISE;
        } else if (state == LOW_HIGH && last_state == HIGH_HIGH) {
            counterclockwise_count++;
            direction = COUNTERCLOCKWISE;
        }
        next_state = LOW_LOW;
        break;

    case 0b01: // LOW_HIGH
        next_state = LOW_HIGH;
        break;

    case 0b10: // HIGH_LOW
        next_state = HIGH_LOW;
        break;

    case 0b11: // HIGH_HIGH
        next_state = HIGH_HIGH;
        break;
    }

    last_state = state;
    state = next_state;
}