// Code by Robbie Leslie

#include "adc.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/structs/sio.h"
#include "dac.h"

//TODO need to update this to use paralel GPIO pins from the new ADC

// Analog mux gain select pins
#define SEL_0 5
#define SEL_1 6

// First GPIO of the 8-bit parallel data bus (D0=GPIO32 … D7=GPIO39)
#define D_BUS_FIRST_PIN 32

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

    // Configure D0-D7 (GPIO32-GPIO39) as high-impedance digital inputs
    for (int i = 0; i < 8; i++) {
        uint pin = D_BUS_FIRST_PIN + i;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_disable_pulls(pin);
    }

    // DMA pacing timer 0: rate = sys_clk * (X/Y)
    // Choose X=1, Y = sys_clk / SAMPLE_RATE_HZ so the timer fires at SAMPLE_RATE_HZ
    uint32_t y = clock_get_hz(clk_sys) / SAMPLE_RATE_HZ;
    if (y > 0xFFFF) y = 0xFFFF;
    dma_hw->timer[0] = (1u << 16) | (uint16_t)y;

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

    // Data channel: byte reads from GPIO_HI_IN[7:0] (= GPIO32-GPIO39) → capture_buf
    // Paced by DMA timer 0 so each transfer happens at SAMPLE_RATE_HZ
    dma_channel_config cfg = dma_channel_get_default_config(data_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_DMA_TIMER0);
    channel_config_set_chain_to(&cfg, ctrl_chan);

    dma_channel_configure(
        data_chan,
        &cfg,
        capture_buf,
        (volatile uint8_t *)&sio_hw->gpio_hi_in,  // low byte = GPIO32-GPIO39
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
