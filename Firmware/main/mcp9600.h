#ifndef MCP9600_H
#define MCP9600_H

#include <driver/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void mcp9600_set_port(i2c_port_t i2c_port);
extern void mcp9600_use_type_J();
extern float mcp9600_get_temp_F();

extern void mcp9600_dump_state();
extern void mcp9600_update_temp();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MCP9600_H
