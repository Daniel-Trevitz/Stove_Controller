#include "mcp9600.h"
#include "i2c.h"

#include <float.h>

static const uint8_t dev_id = 0x60;
static i2c_port_t i2c_port;
static bool broken = false;

void mcp9600_set_port(i2c_port_t i)
{
    i2c_port = i;
}

static float compute_temp_C(uint8_t msb, uint8_t lsb)
{
    return ((float)msb)*16.0 + ((float)lsb)/16.0 - 4096.0 * ((float)!!(msb & 0xF0));
}

static float compute_temp_F(uint8_t msb, uint8_t lsb)
{
    return compute_temp_C(msb, lsb) * 1.8 + 32.0;
}

enum Registers_t
{
    HotJuncitonTemp = 0,
    JunctionDeltaTemp = 1,
    ColdJunctionTemp = 2,
    RawDataADC = 3,
    Status = 4,
    ThermocoupleType = 5,
    DeviceConf = 6,

    Alert1Configuration = 8,
    Alert2Configuration = 9,
    Alert3Configuration = 10,
    Alert4Configuration = 11,

    Alert1Hystersis = 12,
    Alert2Hystersis = 13,
    Alert3Hystersis = 14,
    Alert4Hystersis = 15,

    Alert1Limit = 16,
    Alert2Limit = 17,
    Alert3Limit = 18,
    Alert4Limit = 19,

    DeviceRevision = 0x20
} Registers;

#define DEBUG_REGISTERS_8(a) \
    reg = a; \
    i2c_slave_read(i2c_port, dev_id, &reg, data, 1); \
    printf( #a " = %x\n", data[0]);

#define DEBUG_REGISTERS_16(a) \
    reg = a; \
    i2c_slave_read(i2c_port, dev_id, &reg, data, 2); \
    printf( #a " = %f; %x, %x\n", compute_temp_C(data[0], data[1]), data[0], data[1]);

#define DEBUG_REGISTERS_24(a) \
    reg = a; \
    i2c_slave_read(i2c_port, dev_id, &reg, data, 3); \
    printf( #a " = %x %x %x\n", data[0], data[1], data[2]);

void mcp9600_dump_state()
{
    uint8_t data[8];
    uint8_t reg;

    DEBUG_REGISTERS_16(HotJuncitonTemp);
    DEBUG_REGISTERS_16(JunctionDeltaTemp);
    DEBUG_REGISTERS_16(ColdJunctionTemp);
    DEBUG_REGISTERS_24(RawDataADC);
    DEBUG_REGISTERS_8(Status);
    DEBUG_REGISTERS_8(ThermocoupleType);
    DEBUG_REGISTERS_8(DeviceConf);
    DEBUG_REGISTERS_8(Alert1Configuration);
    DEBUG_REGISTERS_8(Alert2Configuration);
    DEBUG_REGISTERS_8(Alert3Configuration);
    DEBUG_REGISTERS_8(Alert4Configuration);
    DEBUG_REGISTERS_8(Alert1Hystersis);
    DEBUG_REGISTERS_8(Alert2Hystersis);
    DEBUG_REGISTERS_8(Alert3Hystersis);
    DEBUG_REGISTERS_8(Alert4Hystersis);
    DEBUG_REGISTERS_16(Alert1Limit);
    DEBUG_REGISTERS_16(Alert2Limit);
    DEBUG_REGISTERS_16(Alert3Limit);
    DEBUG_REGISTERS_16(Alert4Limit);
    DEBUG_REGISTERS_8(DeviceRevision);
}

void mcp9600_update_temp()
{
    uint8_t data[8];
    uint8_t reg;

    DEBUG_REGISTERS_8(Status);
    DEBUG_REGISTERS_16(HotJuncitonTemp);
    DEBUG_REGISTERS_16(JunctionDeltaTemp);
    DEBUG_REGISTERS_16(ColdJunctionTemp);
    DEBUG_REGISTERS_24(RawDataADC);
}

float mcp9600_get_temp_F()
{
    if (broken)
        return FLT_MAX;

    uint8_t data[8];

    uint8_t reg = HotJuncitonTemp;
    if (ESP_OK != i2c_slave_read(i2c_port, dev_id, &reg, data, 2))
    {
        if (!broken)
            printf("Failed to read temperature!\n");
        broken = true;
        return FLT_MAX;
    }

    return compute_temp_F(data[0], data[1]);
}

void mcp9600_use_type_J()
{
    // 0 typ 0 flt
    // 0 001 0 000

    // typ == thermocouple type; 1 = J
    // flt == filter coefficents; 0 == off

    uint8_t data[8];

    uint8_t reg = ThermocoupleType;
    data[0] = 0x10;

    if (ESP_OK != i2c_slave_write(i2c_port, dev_id, &reg, data, 1))
    {
        broken = true;
        printf("Failed to set thermocouple type to type J\n");
    }
}

