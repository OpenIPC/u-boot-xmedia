/******************************************************************************
* File:             param_tags.c
*
* Author:           Lynn
* Created:          12/08/22
* Description:      PDM Tag data lib, this depends on the ATAGs function of u-boot
*****************************************************************************/

#include <common.h>
#include <linux/string.h>

#define CONFIG_BOOT_PARAMS_SIZE             1024
#define CONFIG_BOOT_PARAMS_ITEM_NUM         20

#undef pr_err
#define pr_err(_V)    printf _V

struct tags_item_t {
	char name[8];
	unsigned short offset;
	unsigned short length;
};

struct tags_data_t {
	int version;
	unsigned int item_num;
	unsigned int buflen;
	char buf[4];
};

struct tags_data_ctrl_t {
	char buf[CONFIG_BOOT_PARAMS_SIZE];
	struct tags_item_t item[CONFIG_BOOT_PARAMS_ITEM_NUM];
	unsigned int item_num;
	unsigned int buflen;
};

/*****************************************************************************/
static struct tags_data_ctrl_t td_ctrl = {
	.item_num = 0,
	.buflen = 0,
};

void set_param_data(const char *name, const char *buf, int buflen)
{
	struct tags_item_t *item = NULL;

	/* parameter vaild check */
	if (!name || !*name || !buf) {
		pr_err(("%s: bad parameter.\n", __FILE__));
		BUG();
	}

	if (td_ctrl.item_num >= CONFIG_BOOT_PARAMS_ITEM_NUM) {
		pr_err(("%s: boot has not enough parameter item room.\n",
			__FILE__));
		BUG();
	}

	if ((buflen + td_ctrl.buflen) > CONFIG_BOOT_PARAMS_SIZE) {
		pr_err(("%s: boot has not enough parameter buffer.\n",
			__FILE__));
		BUG();
	}

	for (item = td_ctrl.item;
	     item < &td_ctrl.item[td_ctrl.item_num]; item++) {

		if (!strncmp(item->name, name, 8)) {
			pr_err(("%s: parameter \"%s\" has exist.\n",
				__FILE__, name));
			BUG();
		}
	}

	item = &td_ctrl.item[td_ctrl.item_num++];

	strncpy(item->name, name, sizeof(item->name));
	if (item->name[sizeof(item->name)-1]) {
		pr_err(("%s: parameter name \"%s\" too longer.\n",
		        __FILE__, name));
		BUG();
	}
	item->name[sizeof(item->name)-1] = '\0';

	item->offset = td_ctrl.buflen;
	item->length = buflen;
	td_ctrl.buflen += item->length;

	memcpy(td_ctrl.buf + item->offset, buf, item->length);
}

int get_param_tag_data(char *tagbuf)
{
	struct tags_data_t *data = (struct tags_data_t *)tagbuf;

	if (!td_ctrl.item_num)
		return 0;

	data->version  = 0x00100000;
	data->item_num = td_ctrl.item_num;
	data->buflen   = (sizeof(struct tags_item_t) * data->item_num);
	memcpy(data->buf, td_ctrl.item, data->buflen);
	memcpy(data->buf + data->buflen, td_ctrl.buf, td_ctrl.buflen);

	data->buflen += td_ctrl.buflen;

	/* more than 4 bytes */
	return ((sizeof(struct tags_data_t) + data->buflen + 0x03) & ~0x03);
}
