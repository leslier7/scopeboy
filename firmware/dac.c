/*
    DAC code
    By Robbie Leslie 2025

    Two DAC5571 8-bit I2C DACs on a shared I2C1 bus:
    SDA  -> GPIO 20
    SCL  -> GPIO 21
    TRIG DAC   I2C addr 0x4D  (A0=1, A1=0)
    OFFSET DAC I2C addr 0x4C  (A0=0, A1=0)
*/

#include "dac.h"
#include "pico/stdlib.h"
#include <math.h>

// DAC5571 write: 2 bytes after address
//   byte 0: [PD1][PD0][0][0][D7][D6][D5][D4]
//   byte 1: [D3][D2][D1][D0][0][0][0][0]
static int dac5571_write(uint8_t addr, uint8_t raw) {
    uint8_t buf[2] = {
        (raw >> 4) & 0x0F,
        (raw << 4) & 0xF0
    };
    return i2c_write_blocking(DAC_I2C_PORT, addr, buf, 2, false);
}

void initDac() {
    i2c_init(DAC_I2C_PORT, 400000);
    gpio_set_function(DAC_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DAC_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DAC_SDA_PIN);
    gpio_pull_up(DAC_SCL_PIN);
}

int setVoltage(int channel, float voltage) {
    if (voltage > 3.3f || voltage < 0.0f) return -1;

    // DAC5571: VOUT = (D / 256) * VDD, VDD = 3.3V
    uint8_t raw = (uint8_t)fminf(voltage / 3.3f * 256.0f, 255.0f);

    switch (channel) {
        case CHAN_TRIG:
            return dac5571_write(DAC_ADDR_TRIG, raw);
        case CHAN_OFFSET:
            return dac5571_write(DAC_ADDR_OFFSET, raw);
        default:
            return -1;
    }
}
