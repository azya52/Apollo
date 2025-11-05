#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"

#define GPIO_IN_CLOCK 7
#define GPIO_DATA_MASK 0x7F
#define CLOCKS_PER_PACKET 120
#define PACKETS_SKIP 30

extern const uint8_t _bad_apollo_data[];
extern const uint8_t _bad_apollo_data_end[];

uint32_t last_clock = 0;
uint32_t prev_diff = 0;
uint32_t clock_count = 0;
uint32_t clock_count_part = 0;
const uint8_t *data = NULL;
uint8_t data_unpacked = 0;
uint32_t data_mod_8 = 0;
bool transmitting = false;

void transmit_data() {
    if (data_mod_8 == 8) {
        gpio_put_masked(0x7F, data_unpacked);
        data_mod_8 = 0;
    } else if (data < _bad_apollo_data_end) {
        gpio_put_masked(GPIO_DATA_MASK, *data);
        data_unpacked = (data_unpacked << 1) | (*data >> 7);
        data++;
        data_mod_8++;
    } else {
        data = _bad_apollo_data;
        data_mod_8 = 0;
    }
}

void start_transmit_data() {
    data = _bad_apollo_data;
    data_mod_8 = 0;
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    gpio_set_dir_out_masked(GPIO_DATA_MASK);
    transmitting = true;
    transmit_data();
}

void clock_callback(uint gpio, uint32_t events) {
    if (gpio == GPIO_IN_CLOCK && (events & GPIO_IRQ_EDGE_RISE)) {
        if (transmitting) {
            transmit_data();
        } else {
            uint32_t current = timer_hw->timerawl;
            if (clock_count != 0) {
                uint32_t diff = current - last_clock;
                if (abs((int32_t)diff - (int32_t)prev_diff) > prev_diff) {
                    clock_count = 1;
                } else {
                    if (clock_count == CLOCKS_PER_PACKET) {
                        clock_count = 0;
                        clock_count_part++;
                        if (clock_count_part > PACKETS_SKIP) {
                            clock_count_part = 0;
                            start_transmit_data();
                        }
                    }
                }
                prev_diff = diff;
            }
            last_clock = current;
            clock_count++;
        }
    }
}

int pico_init(void) {   
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_init(GPIO_IN_CLOCK);
    gpio_set_dir(GPIO_IN_CLOCK, GPIO_IN);
    gpio_pull_down(GPIO_IN_CLOCK);
    gpio_init_mask(GPIO_DATA_MASK);
    gpio_set_dir_in_masked(GPIO_DATA_MASK);

    gpio_set_irq_enabled_with_callback(
        GPIO_IN_CLOCK,
        GPIO_IRQ_EDGE_RISE,
        true,
        &clock_callback
    );

    return PICO_OK;
}

int main() {
    int rc = pico_init();
    hard_assert(rc == PICO_OK);

    while (true) {
        __wfi();
    }
}
