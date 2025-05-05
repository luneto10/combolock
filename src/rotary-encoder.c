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

#define A_WIPER_PIN (16u)
#define B_WIPER_PIN (17u)

typedef enum {
    HIGH_HIGH,
    HIGH_LOW,
    LOW_LOW,
    LOW_HIGH,
    UNKNOWN
} rotation_state_t;

char debug_buffer[22];
const char *states[] = {"H_H", "H_L", "L_L", "L_H", "U_K"};
volatile cowpi_ioport_t *ioport = (cowpi_ioport_t *)(0xD0000000);
static rotation_state_t volatile state;
static rotation_state_t volatile last_state;
static direction_t volatile direction = STATIONARY;
static int volatile clockwise_count = 0;
static int volatile counterclockwise_count = 0;

static void handle_quadrature_interrupt();

void initialize_rotary_encoder() {
    cowpi_set_pullup_input_pins((1 << A_WIPER_PIN) | (1 << B_WIPER_PIN));

    state = HIGH_HIGH;
    last_state = state;
    direction = STATIONARY;

    // sprintf(debug_buffer, "%s %s", states[state], states[last_state]);
    // display_string(0, debug_buffer);
    get_quadrature();
    clockwise_count = 0;
    counterclockwise_count = 0;

    register_pin_ISR((1 << A_WIPER_PIN) | (1 << B_WIPER_PIN), handle_quadrature_interrupt);
}

uint8_t get_quadrature() {
    uint32_t quadrature = ioport->input & (3 << 16);
    uint8_t shifted_quadrature = quadrature >> 16;
    sprintf(debug_buffer, "%d", shifted_quadrature);
    display_string(0, debug_buffer);
    return shifted_quadrature;
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
    uint8_t quadrature = get_quadrature();

    // sprintf(debug_buffer, "%s %s %d", states[last_state],states[state], quadrature);
    // display_string(0, debug_buffer);

    switch (state) {
        case HIGH_HIGH:
            if (quadrature == 0b01){
                last_state = state;
                state = LOW_HIGH;
            }
            else if(quadrature == 0b10){
                last_state = state;
                state = HIGH_LOW;
            }
            break;

        case LOW_HIGH:
            if (quadrature == 0b11){
                state = HIGH_HIGH;
                last_state = state;
            }
            else if (quadrature == 0b00 && last_state == HIGH_HIGH){
                counterclockwise_count++;
                direction = COUNTERCLOCKWISE;
                last_state = state;
                state = LOW_LOW;
            }
            break;

        case HIGH_LOW:
            if (quadrature == 0b11){
                state = HIGH_HIGH;
                last_state = state;
            }
            else if (quadrature == 0b00 && last_state == HIGH_HIGH){
                clockwise_count++;
                direction = CLOCKWISE;
                last_state = state;
                state = LOW_LOW;
            }
            break;

        case LOW_LOW:
            if (quadrature == 0b01 && last_state == HIGH_LOW){
                last_state = state;
                state = LOW_HIGH;
            }
            else if (quadrature == 0b10 && last_state == LOW_HIGH){
                last_state = state;
                state = HIGH_LOW;
            }
            break;

        default:
            break;
        }
}