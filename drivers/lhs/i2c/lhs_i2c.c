#include <lhs_i2c.h>
#include <common.h>
#include <asm/io.h>


/*
 * I2C Registers offsets
 */
#define I2C_GLB 0x0
#define I2C_SCL_H 0x4
#define I2C_SCL_L 0x8
#define I2C_DATA1 0x10
#define I2C_TXF 0x20
#define I2C_RXF 0x24
#define I2C_CMD_BASE 0x30
#define I2C_LOOP1 0xb0
#define I2C_DST1 0xb4
#define I2C_TX_WATER 0xc8
#define I2C_RX_WATER 0xcc
#define I2C_CTRL1 0xd0
#define I2C_CTRL2 0xd4
#define I2C_STAT 0xd8
#define I2C_INTR_RAW 0xe0
#define I2C_INTR_EN 0xe4
#define I2C_INTR_STAT 0xe8

/*
 * I2C Global Config Register -- I2C_GLB
 *  */
#define GLB_EN_MASK BIT(0)
#define GLB_SDA_HOLD_MASK 0xffff00
#define GLB_SDA_HOLD_SHIFT 8

/*
 * I2C Timing CMD Register -- I2C_CMD_BASE + n * 4 (n = 0, 1, 2, ... 31)
 *  */
#define CMD_EXIT 0x0
#define CMD_TX_S 0x1
#define CMD_TX_D1_2 0x4
#define CMD_TX_D1_1 0x5
#define CMD_TX_FIFO 0x9
#define CMD_RX_FIFO 0x12
#define CMD_RX_ACK 0x13
#define CMD_IGN_ACK 0x15
#define CMD_TX_ACK 0x16
#define CMD_TX_NACK 0x17
#define CMD_JMP1 0x18
#define CMD_UP_TXF 0x1d
#define CMD_TX_RS 0x1e
#define CMD_TX_P 0x1f

/*
 * I2C Control Register 1 -- I2C_CTRL1
 */
#define CTRL1_CMD_START_MASK BIT(0)

/*
 * I2C Status Register -- I2C_STAT
 */
#define STAT_RXF_NOE_MASK BIT(16) /* RX FIFO not empty flag */
#define STAT_TXF_NOF_MASK BIT(19) /* TX FIFO not full flag */

/*
 * I2C Interrupt status and mask Register --
 * I2C_INTR_RAW, I2C_STAT, I2C_INTR_STAT
 */
#define INTR_ABORT_MASK (BIT(0) | BIT(11))
#define INTR_RX_MASK BIT(2)
#define INTR_TX_MASK BIT(4)
#define INTR_CMD_DONE_MASK BIT(12)
#define INTR_USE_MASK (INTR_ABORT_MASK | INTR_RX_MASK | INTR_TX_MASK | INTR_CMD_DONE_MASK)
#define INTR_ALL_MASK 0xffffffff
#define I2C_TXF_DEPTH 64
#define I2C_RXF_DEPTH 64
#define I2C_TXF_WATER 32
#define I2C_RXF_WATER 32
/* for i2c rescue */
#define CHECK_SDA_IN_SHIFT 16
#define GPIO_MODE_SHIFT 8
#define FORCE_SCL_OEN_SHIFT 4
#define FORCE_SDA_OEN_SHIFT 0


struct platform_i2c {
	unsigned int msg_buf_ptr;
	int status;
#define I2C_WAIT_RESPOND (1 << 0)
};

struct i2c_platform_data {
	unsigned int freq;
	unsigned int clk;
};

struct i2c_msg {
	unsigned short addr; /* slave address */
	unsigned short flags;
#define I2C_M_TEN 0x0010
#define I2C_M_RD 0x0001
#define I2C_M_STOP 0x8000
#define I2C_M_NOSTART 0x4000
#define I2C_M_REV_DIR_ADDR 0x2000
#define I2C_M_IGNORE_NAK 0x1000
#define I2C_M_NO_RD_ACK 0x0800
#define I2C_M_RECV_LEN 0x0400
#define I2C_M_16BIT_DATA 0x0008
#define I2C_M_16BIT_REG 0x0002
	unsigned short len; /* msg length */
	unsigned char *buf; /* pointer to msg data */
};

struct i2c_driver_data {
	unsigned int reg_base;
	unsigned int freq;
	unsigned int irq;
	unsigned int clk;
	struct i2c_msg *msgs;
	unsigned int msg_num;
	unsigned int msg_idx;
	unsigned int lock;
	void *private;
};

#define CLK_LIMIT_DEFAULT 400000
#define write_reg_bit(value, offset, addr) ({	 \
		unsigned long t, mask;           \
		mask = 1 << (offset);            \
		t = readl(addr);            \
		t &= ~mask;                      \
		t |= (value << (offset)) & mask; \
		writel(t, addr);           \
		})

#define I2C_WAIT_TIMEOUT (100 * 3)
#define I2C_TIMEOUT_COUNT 0x10000
#define I2C_BUF_SIZE 8
#define I2C_INTERRUPT_NUM 0

#if (defined CONFIG_TARGET_XM72050300) || (defined CONFIG_TARGET_XM72020300) || \
       (defined CONFIG_TARGET_XM72050200) || (defined CONFIG_TARGET_XM72050500) || \
       (defined CONFIG_TARGET_XM76050100) || (defined CONFIG_TARGET_XM720XXX)

#define PERI_CRG110 0x01b8
#define I2C_CRG_REG_BASE (CRG_REG_BASE + PERI_CRG110)

#define I2C0_REG_BASE 0x12060000
#define I2C1_REG_BASE 0x12061000
#define I2C2_REG_BASE 0x12062000
#define I2C_NUM 3

#define get_bus_clk() 50000000
#define get_host_clock(i2c_num) ({ get_bus_clk(); })

static struct platform_i2c g_i2c_platform_data[I2C_NUM] = {0};
static int g_i2c_host_cfg[I2C_NUM] = {0, 1, 2}; /* 0,1,2: i2c index */
static struct i2c_driver_data g_i2c_data[I2C_NUM] = {
        {I2C0_REG_BASE, CLK_LIMIT_DEFAULT, I2C_INTERRUPT_NUM},
        {I2C1_REG_BASE, CLK_LIMIT_DEFAULT, I2C_INTERRUPT_NUM},
        {I2C2_REG_BASE, CLK_LIMIT_DEFAULT, I2C_INTERRUPT_NUM},
};

#endif


static void i2c_disable(const struct i2c_driver_data *i2c);
static void i2c_cfg_irq(const struct i2c_driver_data *i2c, unsigned int flag);
static unsigned int i2c_clr_irq(const struct i2c_driver_data *i2c);

static void i2c_rescue(const struct i2c_driver_data *i2c)
{
	i2c_disable(i2c);
	i2c_cfg_irq(i2c, 0);
	i2c_clr_irq(i2c);

	unsigned int val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
			(0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->reg_base + I2C_CTRL2);

	unsigned int time_cnt = 0;
	do {
		for (int index = 0; index < 9; index++) { /* shift:9 */
			val = (0x1 << GPIO_MODE_SHIFT) | 0x1;
			writel(val, i2c->reg_base + I2C_CTRL2);
			udelay(5); /* delay: 5 us */
			val = (0x1 << GPIO_MODE_SHIFT) |
				(0x1 << FORCE_SCL_OEN_SHIFT) |
				(0x1 << FORCE_SDA_OEN_SHIFT);
			writel(val, i2c->reg_base + I2C_CTRL2);
			udelay(5); /* delay: 5 us */
		}

		time_cnt++;
		if (time_cnt > I2C_WAIT_TIMEOUT) {
			goto disable_rescue;
		}

		val = readl(i2c->reg_base + I2C_CTRL2);
	} while (!(val & (0x1 << CHECK_SDA_IN_SHIFT)));

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
		(0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->reg_base + I2C_CTRL2);
	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT);
	writel(val, i2c->reg_base + I2C_CTRL2);
	udelay(10); /* delay: 10 us */
	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
		(0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->reg_base + I2C_CTRL2);

disable_rescue:
	val = (0x1 << FORCE_SCL_OEN_SHIFT) | 0x1;
	writel(val, i2c->reg_base + I2C_CTRL2);
}

static void i2c_disable(const struct i2c_driver_data *i2c)
{
	unsigned int val = readl(i2c->reg_base + I2C_GLB);
	val &= ~GLB_EN_MASK;
	writel(val, i2c->reg_base + I2C_GLB);
}

static void i2c_disable_irq(const struct i2c_driver_data *i2c, unsigned int flag)
{
	unsigned int val = readl(i2c->reg_base + I2C_INTR_EN);
	val &= ~flag;
	writel(val, i2c->reg_base + I2C_INTR_EN);
}

static unsigned int i2c_clr_irq(const struct i2c_driver_data *i2c)
{
	unsigned int val = readl(i2c->reg_base + I2C_INTR_STAT);
	writel(INTR_ALL_MASK, i2c->reg_base + I2C_INTR_RAW);

	return val;
}

static void i2c_set_freq(struct i2c_driver_data *i2c)
{
	unsigned int val;
	unsigned int freq = i2c->freq;
	unsigned int clk_rate = i2c->clk;
	unsigned int max_freq = clk_rate >> 1;

	if (freq > max_freq) {
		i2c->freq = max_freq;
		freq = i2c->freq;
	}

	if (freq <= 100000) { /* 100000:100KHz */
		val = clk_rate / (freq * 2); /* 1/2:0.5 */
		writel(val, i2c->reg_base + I2C_SCL_H);
		writel(val, i2c->reg_base + I2C_SCL_L);
	} else {
		val = (clk_rate * 36) / (freq * 100); /* 36/100:0.36 */
		writel(val, i2c->reg_base + I2C_SCL_H);
		val = (clk_rate * 64) / (freq * 100); /* 64/100:0.64 */
		writel(val, i2c->reg_base + I2C_SCL_L);
	}
	val = readl(i2c->reg_base + I2C_GLB);
	val &= ~GLB_SDA_HOLD_MASK;
	val |= ((0xa << GLB_SDA_HOLD_SHIFT) & GLB_SDA_HOLD_MASK);
	writel(val, i2c->reg_base + I2C_GLB);
}

/*
 * set i2c controller TX and RX FIFO water
 */
static void i2c_set_water(const struct i2c_driver_data *i2c)
{
	writel(I2C_TXF_WATER, i2c->reg_base + I2C_TX_WATER);
	writel(I2C_RXF_WATER, i2c->reg_base + I2C_RX_WATER);
}

static void i2c_enable_clk(unsigned char i2c_num)
{
	const unsigned int clk_start_bit = 11 + i2c_num; /* 11: i2c clk start bit */
	const unsigned int rst_start_bit = 19 + i2c_num; /* 19: i2c rst start bit */
	const unsigned int enable_clck = 1;
	const unsigned int enable_rst = 0;
	write_reg_bit(enable_clck, clk_start_bit, (uintptr_t)I2C_CRG_REG_BASE);
	write_reg_bit(enable_rst, rst_start_bit, (uintptr_t)I2C_CRG_REG_BASE);
}

/*
 * initialise the controller, set i2c bus interface freq
 */
static void i2c_init_cfg(const struct i2c_driver_data *i2c, unsigned char i2c_num)
{
	i2c_enable_clk(i2c_num);
	i2c_disable(i2c);
	i2c_disable_irq(i2c, INTR_ALL_MASK);
	i2c_set_freq((struct i2c_driver_data *)i2c);
	i2c_set_water(i2c);
}

static void i2c_cmdreg_set(const struct i2c_driver_data *i2c, unsigned int cmd,
	unsigned int *offset)
{
	writel(cmd, i2c->reg_base + I2C_CMD_BASE + (*offset) * 4); /* 4: bytes */
	(*offset)++;
}

static void i2c_cfg_cmd(const struct i2c_driver_data *i2c)
{
	struct i2c_msg *msg = i2c->msgs;
	unsigned int offset = 0;

	if (i2c->msg_idx == 0)
		i2c_cmdreg_set(i2c, CMD_TX_S, &offset);
	else
		i2c_cmdreg_set(i2c, CMD_TX_RS, &offset);

	if (msg->flags & I2C_M_TEN) {
		if (i2c->msg_idx == 0) {
			i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
			i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
		} else {
			i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
		}
	} else {
		i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
	}

	if (msg->flags & I2C_M_IGNORE_NAK)
		i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
	else
		i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);

	if (msg->flags & I2C_M_RD) {
		if (msg->len >= 2) { /* msg len:2 */
			writel(offset, i2c->reg_base + I2C_DST1);
			writel(msg->len - 2, i2c->reg_base + I2C_LOOP1); /* 2: max len */
			i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
			i2c_cmdreg_set(i2c, CMD_TX_ACK, &offset);
			i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
		}
		i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
		i2c_cmdreg_set(i2c, CMD_TX_NACK, &offset);
	} else {
		writel(offset, i2c->reg_base + I2C_DST1);
		writel(msg->len - 1, i2c->reg_base + I2C_LOOP1);
		i2c_cmdreg_set(i2c, CMD_UP_TXF, &offset);
		i2c_cmdreg_set(i2c, CMD_TX_FIFO, &offset);

		if (msg->flags & I2C_M_IGNORE_NAK) {
			i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
		}
		else {
			i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);
		}
		i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
	}

	if ((i2c->msg_idx == (i2c->msg_num - 1)) || (msg->flags & I2C_M_STOP)) {
		i2c_cmdreg_set(i2c, CMD_TX_P, &offset);
	}

	i2c_cmdreg_set(i2c, CMD_EXIT, &offset);
}

static void i2c_enable(const struct i2c_driver_data *i2c)
{
	unsigned int val = readl(i2c->reg_base + I2C_GLB);
	val |= GLB_EN_MASK;
	writel(val, i2c->reg_base + I2C_GLB);
}

/*
 * config i2c slave addr
 */
static void i2c_set_addr(const struct i2c_driver_data *i2c)
{
	struct i2c_msg *msg = i2c->msgs;
	unsigned int addr;

	if (msg->flags & I2C_M_TEN) {
		/* first byte is 11110XX0 where XX is upper 2 bits */
		addr = ((msg->addr & 0x300) << 1) | 0xf000;
		if (msg->flags & I2C_M_RD) {
			addr |= 1 << 8; /* shift:8 */
		}

		/* second byte is the remaining 8 bits */
		addr |= msg->addr & 0xff;
	} else {
		addr = (msg->addr & 0x7f) << 1;
		if (msg->flags & I2C_M_RD) {
			addr |= 1;
		}
	}

	writel(addr, i2c->reg_base + I2C_DATA1);
}

/*
 * start command sequence
 */
static void i2c_start_cmd(const struct i2c_driver_data *i2c)
{
	unsigned int val = readl(i2c->reg_base + I2C_CTRL1);
	val |= CTRL1_CMD_START_MASK;
	writel(val, i2c->reg_base + I2C_CTRL1);
}

static int i2c_wait_rx_noempty(const struct i2c_driver_data *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->reg_base + I2C_STAT);
		if (val & STAT_RXF_NOE_MASK) {
			return 0;
		}
		udelay(50); /* delay:50 us */
		time_cnt++;
	} while (time_cnt < I2C_TIMEOUT_COUNT);

	i2c_rescue(i2c);

	return -EIO;
}

static int i2c_wait_tx_nofull(const struct i2c_driver_data *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->reg_base + I2C_STAT);
		if (val & STAT_TXF_NOF_MASK) {
			return 0;
		}
		udelay(50); /* delay:50 us */
		time_cnt++;
	} while (time_cnt < I2C_TIMEOUT_COUNT);

	i2c_rescue(i2c);

	return -EIO;
}


static int i2c_wait_idle(const struct i2c_driver_data *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->reg_base + I2C_INTR_RAW);
		if (val & (INTR_ABORT_MASK)) {
			printf("i2c wait idle EIO,val==0x%x\n",val);
			return -EIO;
		}

		if (val & INTR_CMD_DONE_MASK) {
			return 0;
		}
		udelay(50); /* delay:50 us */
		time_cnt++;
	} while (time_cnt < I2C_WAIT_TIMEOUT);

	i2c_rescue(i2c);

	return -EIO;
}

static int i2c_polling_xfer_one_msg(const struct i2c_driver_data *i2c)
{
	int status;
	unsigned int val;
	struct i2c_msg *msg = i2c->msgs;
	unsigned int msg_buf_ptr = 0;

	i2c_enable(i2c);
	i2c_clr_irq(i2c);
	i2c_set_addr(i2c);
	i2c_cfg_cmd(i2c);
	i2c_start_cmd(i2c);

	if (msg->flags & I2C_M_RD) {
		while (msg_buf_ptr < msg->len) {
			status = i2c_wait_rx_noempty(i2c);
			if (status) {
				goto end;
			}
			val = readl(i2c->reg_base + I2C_RXF);
			msg->buf[msg_buf_ptr] = val;
			msg_buf_ptr++;
		}
	} else {
		while (msg_buf_ptr < msg->len) {
			status = i2c_wait_tx_nofull(i2c);
			if (status) {
				goto end;
			}
			val = msg->buf[msg_buf_ptr];
			writel(val, i2c->reg_base + I2C_TXF);
			msg_buf_ptr++;
		}
	}

	status = i2c_wait_idle(i2c);
end:
	i2c_disable(i2c);

	return status;
}

static void i2c_cfg_irq(const struct i2c_driver_data *i2c, unsigned int flag)
{
	writel(flag, i2c->reg_base + I2C_INTR_EN);
}

static int i2c_xfer(unsigned char i2c_num, const struct i2c_msg *msgs, int num)
{
	int status = 0;

	if (msgs == NULL) {
		printf("[error] msg pointer is null.\n");
		return -EIO;
	}

	struct i2c_driver_data *i2c = &g_i2c_data[i2c_num];
	struct platform_i2c *hpi = &g_i2c_platform_data[i2c_num];

	i2c->clk = get_host_clock(0);
	i2c->private = (void *)(hpi);

	i2c->msgs = (struct i2c_msg *)msgs;
	i2c->msg_num = (unsigned int)num;
	i2c->msg_idx = 0;

	while (i2c->msg_idx < i2c->msg_num) {
		status = i2c_polling_xfer_one_msg(i2c);
 		if (status) {
			break;
 		}

		i2c->msgs++;
		i2c->msg_idx++;
	}

	if (!status || i2c->msg_idx > 0) {
		status = i2c->msg_idx;
	}

	return status;
}

static int i2c_check_enable(unsigned char i2c_num)
{
	if (i2c_num >= I2C_NUM) {
		return -1;
	}

	return g_i2c_host_cfg[i2c_num];
}

static int hal_i2c_recv_inner(const struct i2c_client *client, unsigned int *buf,
	unsigned int count)
{
	struct i2c_msg msg[I2C_BUF_SIZE] = {0};
	unsigned char recv_buf[I2C_BUF_SIZE] = {0};

	if (client->reg_width == 2) { /* reg_width:2 */
		recv_buf[0] = (client->reg_addr >> 8) & 0xff; /* shift:8 */
		recv_buf[1] = client->reg_addr & 0xff;
	} else {
		recv_buf[0] = client->reg_addr & 0xff;
	}

	msg[0].addr = client->dev_addr;
	msg[0].flags = 0;
	msg[0].len = client->reg_width;
	msg[0].buf = recv_buf;

	msg[1].addr = client->dev_addr;
	msg[1].flags = 0;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = count;
	msg[1].buf = recv_buf;

	int ret = i2c_xfer(client->i2c_num, msg, 2); /* msg num:2 */
	if (ret < 0) {
		return -1;
	}

	if (count == 2) { /* data_width:2 */
		*buf = recv_buf[0] | (recv_buf[1] << 8); /* shift:8 */
	} else {
		*buf = recv_buf[0];
	}

	return 0;
}

static int hal_i2c_send_inner(unsigned char i2c_num, unsigned short dev_addr, const char *buf,
	unsigned int count)
{
	struct i2c_msg msg = {0};
	msg.addr = dev_addr;
	msg.flags = 0;
	msg.len = count;
	msg.buf = (unsigned char *)buf;
	int ret = i2c_xfer(i2c_num, &msg, 1);
	return (ret == 1) ? count : ret;
}

static int hal_i2c_init_inner(unsigned char i2c_num)
{
	struct i2c_driver_data *i2c = NULL;
	struct platform_i2c *hpi = NULL;

	int ret = i2c_check_enable(i2c_num);
	if (ret < 0) {
		return -1;
	}

	i2c = &g_i2c_data[i2c_num];
	hpi = &g_i2c_platform_data[i2c_num];
	i2c->clk = get_host_clock(i2c_num);
	i2c->private = (void *)(hpi);
	i2c->irq = 0;
	i2c_init_cfg(i2c, i2c_num);

	return 0;
}

int hal_i2c_recv(const struct i2c_client *client, unsigned int *buf,
	unsigned int count)
{
	if ((client == NULL) || (buf == NULL)) {
		printf("[error] i2c recv param is null.\n");
		return -1;
	}

	if (client->i2c_num >= I2C_NUM) {
		printf("[error] i2c num(%u) is invalid.\n", client->i2c_num);
		return -1;
	}

	if (count > I2C_BUF_SIZE) {
		printf("[error] buf count(%u) should lees than(%d).\n", count, I2C_BUF_SIZE);
		return -1;
	}

	return hal_i2c_recv_inner(client, buf, count);
}

int hal_i2c_send(unsigned char i2c_num, unsigned short dev_addr, const char *buf,
	unsigned int count)
{
	if (i2c_num >= I2C_NUM) {
		printf("[error] i2c num(%u) is invalid.\n", i2c_num);
		return -1;
	}

	if (buf == NULL) {
		printf("[error] i2c send param is null.\n");
		return -1;
	}

	if (count > I2C_BUF_SIZE) {
		printf("[error] buf count(%u) should lees than(%d).\n", count, I2C_BUF_SIZE);
		return -1;
	}

	return hal_i2c_send_inner(i2c_num, dev_addr, buf, count);
}

int hal_i2c_init(unsigned char i2c_num)
{
	if (i2c_num >= I2C_NUM) {
		printf("[error] i2c num(%u) is invalid.\n", i2c_num);
		return -1;
	}

	return hal_i2c_init_inner(i2c_num);
}
