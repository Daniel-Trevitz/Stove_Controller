#include <sys/time.h>
#include <driver/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} date_time_t;

void set_hwclock(i2c_port_t i2c_port, date_time_t dt);
void set_hwclock_2(i2c_port_t i2c_port, date_time_t dt);
date_time_t get_hwclock(i2c_port_t i2c_port);

float rx8900_get_temp(i2c_port_t i2c_port);

#ifdef __cplusplus
}
#endif
