#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"

#define GPIO_IN_CLOCK 7
#define GPIO_DATA_MASK 0x7F
#define TRANSMIT_TIMEOUT_US 500000
#define DATA_BUFFER_SIZE 1024 * 400
#define CLOCKS_PER_PACKET 120
#define PACKETS_SKIP 30

uint32_t last_clock = 0;
uint32_t prev_diff = 0;
uint32_t clock_count = 0;
uint32_t clock_count_part = 0;
uint32_t data_i = 0;
bool transmitting = false;
alarm_id_t transmit_alarm_id = 0;
uint8_t *data = NULL;
uint32_t data_size = 0;

int64_t transmit_timeout_callback(alarm_id_t id, void *user_data);

void receive_data_usb(uint8_t *buf) {
    int32_t c = getchar_timeout_us(0);
    if (c != PICO_ERROR_TIMEOUT) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        if (data) {
            free(data);
            data = NULL;
        }
        data = (uint8_t*)malloc(DATA_BUFFER_SIZE);
        data_size = 0;
        while ((c != PICO_ERROR_TIMEOUT) && (data_size < DATA_BUFFER_SIZE)) {
            data[data_size++] = (uint8_t)c;
            c = getchar_timeout_us(1000);
        }
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }
}

void end_transmit_data() {
    cancel_alarm(transmit_alarm_id);
    gpio_set_dir_in_masked(GPIO_DATA_MASK);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    transmitting = false;
}

void transmit_data() {
    if (data_i < data_size) {
        gpio_put_masked(GPIO_DATA_MASK, data[data_i++]);
    } else {
        end_transmit_data();
    }
}

void start_transmit_data() {
    data_i = 0;
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    gpio_set_dir_out_masked(GPIO_DATA_MASK);
    transmitting = true;
    transmit_data();
    transmit_alarm_id = add_alarm_in_us(TRANSMIT_TIMEOUT_US, transmit_timeout_callback, NULL, false);
}

int64_t transmit_timeout_callback(alarm_id_t id, void *user_data) {
    if (transmitting) {
        end_transmit_data();
    }
    return 0;
}

void clock_callback(uint gpio, uint32_t events) {
    if ((gpio == GPIO_IN_CLOCK) && (events & GPIO_IRQ_EDGE_RISE)) {
        if (data_size > 0) {
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
}

int pico_init(void) {
    stdio_usb_init();
    
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
        receive_data_usb(data);
        __wfi();
    }
}
