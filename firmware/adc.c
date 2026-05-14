// Code by Robbie Leslie

#include "adc.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "adc8060.pio.h"
#include "dac.h"

// Analog mux gain select pins
#define SEL_0 5
#define SEL_1 6

// First GPIO of the 8-bit parallel data bus (D0=GPIO32 … D7=GPIO39)
#define D_BUS_FIRST_PIN 32

// CLK output to ADC8060
#define ADC_CLK_PIN 9

uint8_t capture_buf[CAPTURE_DEPTH];
uint8_t frame_buf[CAPTURE_DEPTH];

volatile bool trigger_fired = false;
volatile bool capture_ready = false;
volatile bool trigger_armed = true;

// Pointer reset value for the DMA control channel
uint8_t *dma_address_pointer = &capture_buf[0];

void init_adc_capture() {
    // Set up analog mux gain select pins
    gpio_init(SEL_0);
    gpio_set_dir(SEL_0, GPIO_OUT);
    gpio_init(SEL_1);
    gpio_set_dir(SEL_1, GPIO_OUT);

    // PIO1 — TFTMaster already owns pio0
    // Generates CLK on GPIO9 and captures D0-D7 (GPIO32-39) on each rising edge
    PIO pio = pio1;
    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &adc8060_capture_program);
    adc8060_capture_program_init(pio, sm, offset, ADC_CLK_PIN, D_BUS_FIRST_PIN, SAMPLE_RATE_HZ);

    uint ctrl_chan = dma_claim_unused_channel(true);
    uint data_chan = dma_claim_unused_channel(true);

    // Control channel: resets the data channel's write address so capture loops
    dma_channel_config c = dma_channel_get_default_config(ctrl_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
    channel_config_set_chain_to(&c, data_chan);

    dma_channel_configure(
        ctrl_chan,
        &c,
        &dma_hw->ch[data_chan].write_addr,
        &dma_address_pointer,
        1,
        false
    );

    // Data channel: 8-bit reads from PIO RX FIFO → capture_buf, paced by PIO DREQ
    dma_channel_config cfg = dma_channel_get_default_config(data_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, false));
    channel_config_set_chain_to(&cfg, ctrl_chan);

    dma_channel_configure(
        data_chan,
        &cfg,
        capture_buf,
        (volatile uint8_t *)&pio->rxf[sm],
        CAPTURE_DEPTH,
        true
    );

    dma_start_channel_mask(1u << ctrl_chan);
}

float adc_to_volt(uint8_t adc_val) {
    return (adc_val / 255.0f) * 3.3f;
}

void set_gain(gain_mode_t gain) {
    switch (gain) {
        case GAIN_LOW:
            gpio_put(SEL_0, false);
            gpio_put(SEL_1, false);
            break;
        case GAIN_MEDIUM:
            gpio_put(SEL_0, false);
            gpio_put(SEL_1, true);
            break;
        case GAIN_HIGH:
            gpio_put(SEL_0, true);
            gpio_put(SEL_1, false);
            break;
        default:
            gpio_put(SEL_0, false);
            gpio_put(SEL_1, false);
            break;
    }
}
