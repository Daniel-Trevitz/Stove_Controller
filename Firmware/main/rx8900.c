#include "rx8900.h"
#include "i2c.h"

#include <errno.h>
#include <string.h>

// https://support.epson.biz/td/api/doc_check.php?dl=app_RX8900SA&lang=en

#define REG_YEAR 6
#define REG_MON  5
#define REG_DAY  4
#define REG_WEEK 3
#define REG_HOUR 2
#define REG_MIN  1
#define REG_SEC  0

// From qemu-common.h
/* Convert a byte between binary and BCD.  */
static inline uint8_t to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}
static inline uint8_t from_bcd(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0f);
}

date_time_t get_hwclock(i2c_port_t i2c_port)
{
    date_time_t dt;
    memset(&dt, 0, sizeof(date_time_t));

    const static int len = 7;
    uint8_t reg[] = { REG_SEC };
    uint8_t data[len];
    int err = i2c_slave_read(i2c_port, 0x32, reg, data, len);

    if (err)
    {
        printf ("error %d on read %d byte", err, len);
        return dt;
    }

    dt.year  = 2000 + from_bcd(data[REG_YEAR]);
    dt.month = from_bcd(data[REG_MON]);
    dt.day   = from_bcd(data[REG_DAY]);
    dt.hour  = from_bcd(data[REG_HOUR]);
    dt.min   = from_bcd(data[REG_MIN]);
    dt.sec   = from_bcd(data[REG_SEC]);

    return dt;
}

void set_hwclock(i2c_port_t i2c_port, date_time_t dt)
{
    const static int len = 3;
    int err = 0;

    {
        const uint8_t reg[] = { REG_SEC };
        uint8_t data[] = {
            to_bcd(dt.sec),
            to_bcd(dt.min),
            to_bcd(dt.hour)
        };

        err = i2c_slave_write(i2c_port, 0x32, reg, data, len);
    }

    if(!err)
    {
        const uint8_t reg[] = { REG_DAY };
        const uint8_t data[] = {
            to_bcd(dt.day),
            to_bcd(dt.month),
            to_bcd(dt.year)
        };

        err = i2c_slave_write(i2c_port, 0x32, reg, data, len);
    }

    if (err)
    {
        printf ("error %d on writing %d byte\n", err, len);
        return;
    }
}


void set_hwclock_2(i2c_port_t i2c_port, date_time_t dt)
{
    const static int len = 3;
    uint8_t reg[] = { REG_SEC };
    uint8_t data[] = {
        to_bcd(dt.sec),
        to_bcd(dt.min),
        to_bcd(dt.hour)
    };
    int err = i2c_slave_write(i2c_port, 0x32, reg, data, len);

    if (err)
    {
        printf ("error %d on writing %d byte\n", err, len);
        return;
    }
}

float rx8900_get_temp(i2c_port_t i2c_port)
{
    const static int len = 1;
    uint8_t reg[] = { 0x17 };
    uint8_t data[len];
    int err = i2c_slave_read(i2c_port, 0x32, reg, data, len);

    if (err)
    {
        printf ("error %d on read %d byte", err, len);
        return 0.0;
    }

    return ((float)(data[0] * 2) - 187.19) / 3.218;
}

