#ifndef LHS_I2C_H
#define LHS_I2C_H

#include <compiler.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct i2c_client {
	unsigned char i2c_num;
	unsigned short dev_addr;
	unsigned long int reg_addr;
	unsigned int reg_width;
};

int hal_i2c_init(unsigned char i2c_num);
int hal_i2c_recv(const struct i2c_client *client, unsigned int *buf,
	unsigned int count);
int hal_i2c_send(unsigned char i2c_num, unsigned short dev_addr, const char *buf,
	unsigned int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HAL_I2C_H */
