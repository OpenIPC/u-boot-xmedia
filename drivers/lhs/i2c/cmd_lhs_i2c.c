#include <command.h>
#include <lhs_i2c.h>
#include <common.h>
#include <asm/io.h>


#define BUF_LEN 8


extern int hal_i2c_init(unsigned char i2c_num);
extern int hal_i2c_recv(const struct i2c_client *client, unsigned int *buf, unsigned int count);
extern int hal_i2c_send(unsigned char i2c_num, unsigned short dev_addr, const char *buf,unsigned int count);


static int do_i2c_recv(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int va_byte = 0;
	int ret;
     	int i2c_num;
     	int reg_byte;
     	int data_byte;
     	unsigned short device_addr;
     	unsigned int reg;
    	struct i2c_client client = {0};

     	/* parse cmd args */
     	if(argc != 6) {
        	printf("invalid params\n");
                return -1;
     	}


     	i2c_num = (unsigned int)simple_strtoul(argv[1], NULL, 10);
     	device_addr = (unsigned short)simple_strtoul(argv[2], NULL, 16);
     	reg = (unsigned int)simple_strtoul(argv[3], NULL, 16);
     	reg_byte = (unsigned int )simple_strtoul(argv[4], NULL, 10);
     	data_byte = (unsigned int )simple_strtoul(argv[5], NULL, 10);

     	printf("i2c paramaters:i2c_num[%d] device_addr[0x%x] reg[0x%x] reg_byte[%d] data_byte[%d]\n",i2c_num, device_addr, reg, reg_byte, data_byte);

     	client.i2c_num = i2c_num;
     	client.dev_addr = device_addr;
     	client.dev_addr = (client.dev_addr >> 1) & 0xff;
     	client.reg_addr = reg;
     	client.reg_width = reg_byte;

     	ret = hal_i2c_recv(&client, &va_byte, data_byte);
     	if (ret != 0) {
        	printf("[error] hal_i2c_recv err ret: %d\n", ret);
         	return 0;
     	}
     	printf("recv data va_byte[0x%x]\n",va_byte);

     	return 0;
}


static int do_i2c_send(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
     	int i2c_num;
     	int reg_byte;
     	int data_byte;
     	unsigned int idx = 0;
     	int ret;
     	char buf[BUF_LEN] = {0};
     	unsigned short device_addr;
     	unsigned int reg;
     	unsigned int data;

     	/* parse cmd args */
     	if(argc != 7) {
     		printf("invalid params\n");
		return -1;
     	}
     	i2c_num = (unsigned int)simple_strtoul(argv[1], NULL, 10);
     	device_addr = (unsigned short)simple_strtoul(argv[2], NULL, 16);
     	reg = (unsigned int)simple_strtoul(argv[3], NULL, 16);
     	reg_byte = (unsigned int )simple_strtoul(argv[4], NULL, 10);
     	data = (unsigned int)simple_strtoul(argv[5], NULL, 16);
     	data_byte = (unsigned int )simple_strtoul(argv[6], NULL, 10);

     	printf("i2c paramaters:i2c_num[%d] device_addr[0x%x] reg[0x%x] reg_byte[%d] data[0x%x] data_byte[%d]\n",i2c_num, device_addr, reg, reg_byte, data, data_byte);

     	writel(0x1d01, 0x112C0030);
     	writel(0x1d01, 0x112C0034); /* i2c0 */

     	writel(0x1d01, 0x112C0038);
     	writel(0x1d01, 0x112C003C); /* i2c2 */

     	hal_i2c_init(i2c_num);

     	if (reg_byte == 1) { /* 1 byte */
        	buf[idx] = reg & 0xff;
            	idx++;
     	}else if(reg_byte == 2) { /* 2 byte */
        	buf[idx] = (reg >> 8) & 0xff; /* shift 8 */
            	idx++;
            	buf[idx] = reg & 0xff;
            	idx++;
     	}

     	if(data_byte == 1) { /* 1 byte */
            	buf[idx] = data & 0xff;
            	idx++;
     	}
     	else if (data_byte == 2) {   /* 2 byte */
            	buf[idx] = (data >> 8) & 0xff; /* shift 8 */
            	idx++;
            	buf[idx] = data & 0xff;
            	idx++;
     	}

     	unsigned short i2c_addr = (device_addr >> 1);
     	const unsigned int buf_len = reg_byte + data_byte;
     	ret = hal_i2c_send(i2c_num, i2c_addr, buf, buf_len);
     	if (ret <= 0) {
            	printf("[error] hal_i2c_send err ret: %d\n", ret);
                return -1;
     	}
     	if (ret != (reg_byte + data_byte)) {
            	printf("[error] hal_i2c_send error\n");
                return -1;
     	}

     	return 0;
}

/*
 * i2c recv test:for example 7205v200 volt sendor
 * 1.i2c_recv 2 0x94 0x02 1 2
 */
U_BOOT_CMD(i2c_recv, 6, 1, do_i2c_recv,
           "i2c_recv   - i2c recv from device.\n"
           "\t- i2c_recv [args i2c_num, device_addr,reg,reg_length, data_length]",
	   "\nargs: [i2c_num, device_addr,reg,reg_length, data_length]\n"
           "\t-<i2c_num> : 0/1/2\n"
           "\t-<device_addr> : 0x94\n"
           "\t-<reg_addr> : 0xaa\n"
           "\t-<reg_length>: 1\n"
           "\t-<data_length>: 1\n");


/*
 * i2c send test:for example 7205v200 volt sendor
 * 1.i2c_send 2 0x94 0x00 1 0x3fff 2
 * 2.i2c_send 2 0x94 0x05 1 0xa000 2
 */
U_BOOT_CMD(i2c_send, 7, 1, do_i2c_send,
           "i2c_send   - i2c send to device.\n"
           "\t- i2c_send [args: i2c_num, device_addr,reg,reg_length,data, data_length]",
	   "\nargs: [i2c_num, device_addr,reg,reg_length,data, data_length]\n"
           "\t-<i2c_num> : 0/1/2\n"
           "\t-<device_addr> : 0x94\n"
           "\t-<reg_addr> : 0xaa\n"
           "\t-<reg_length>: 1\n"
           "\t-<data> : 0x11\n"
           "\t-<data_length>: 1\n");
