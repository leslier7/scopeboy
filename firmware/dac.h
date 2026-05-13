#ifndef DAC_H
#define DAC_H

#include "hardware/i2c.h"

#define DAC_I2C_PORT    i2c1
#define DAC_SDA_PIN     20
#define DAC_SCL_PIN     21
#define DAC_ADDR_TRIG   0x4D    // 8-bit 0x9A >> 1
#define DAC_ADDR_OFFSET 0x4C    // 8-bit 0x98 >> 1

enum DAC_Chan {
    CHAN_TRIG,
    CHAN_OFFSET
};

void initDac();

int setVoltage(int channel, float voltage);

#endif
