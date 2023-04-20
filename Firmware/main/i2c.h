#include <driver/gpio.h>
#include <driver/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

void initalize_i2c(i2c_port_t i2c_port, gpio_num_t scl, gpio_num_t sda, uint32_t freq);

int i2c_slave_write (i2c_port_t i2c_port, uint8_t addr, const uint8_t *reg, uint8_t *data, uint32_t len);

int i2c_slave_read (i2c_port_t i2c_port, uint8_t addr, const uint8_t *reg, uint8_t *data, uint32_t len);

void i2c_detect(i2c_port_t i2c_port);

#ifdef __cplusplus
}
#endif
