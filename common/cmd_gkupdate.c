#if 1
//#include <environment.h>
#include <common.h>
#include <config.h>
#include <command.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/types.h>
#include <fat.h>
#include <exports.h>
#else
#include <common.h>
#include <environment.h>
#include <command.h>
#include <malloc.h>
#include <image.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <spi_flash.h>
#include <linux/mtd/mtd.h>
#include <fat.h>
#include <mmc.h>
#endif

#define CFG_MEM_ADDR     0x41000000
#define IMAGE_MEM_ADDR   0x41100000

#define UBIFS_UPGRADE_FILE "amg_upgrade.bin"
#define UBIFS_UPGRADE_ALL "amg_all.bin"
#define UBIFS_PART_NAME    "System"


typedef struct {
    unsigned int len;
    unsigned int addr;
}pkt_inf;

typedef struct {
    unsigned int flash_size;
    unsigned int pkt_num;
    unsigned int crc;
    unsigned int update_zone;
    pkt_inf pkt_info[15];
}pkt_cfg;

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

//#define cpu_to_le32(x)        (x)
//#define le32_to_cpu(x)        (x)
#define DO_CRC(x) crc = tab[(crc ^ (x)) & 255] ^ (crc >> 8)

#define tole(x) cpu_to_le32(x)

static const uint32_t crc_table[256] = {
tole(0x00000000L), tole(0x77073096L), tole(0xee0e612cL), tole(0x990951baL),
tole(0x076dc419L), tole(0x706af48fL), tole(0xe963a535L), tole(0x9e6495a3L),
tole(0x0edb8832L), tole(0x79dcb8a4L), tole(0xe0d5e91eL), tole(0x97d2d988L),
tole(0x09b64c2bL), tole(0x7eb17cbdL), tole(0xe7b82d07L), tole(0x90bf1d91L),
tole(0x1db71064L), tole(0x6ab020f2L), tole(0xf3b97148L), tole(0x84be41deL),
tole(0x1adad47dL), tole(0x6ddde4ebL), tole(0xf4d4b551L), tole(0x83d385c7L),
tole(0x136c9856L), tole(0x646ba8c0L), tole(0xfd62f97aL), tole(0x8a65c9ecL),
tole(0x14015c4fL), tole(0x63066cd9L), tole(0xfa0f3d63L), tole(0x8d080df5L),
tole(0x3b6e20c8L), tole(0x4c69105eL), tole(0xd56041e4L), tole(0xa2677172L),
tole(0x3c03e4d1L), tole(0x4b04d447L), tole(0xd20d85fdL), tole(0xa50ab56bL),
tole(0x35b5a8faL), tole(0x42b2986cL), tole(0xdbbbc9d6L), tole(0xacbcf940L),
tole(0x32d86ce3L), tole(0x45df5c75L), tole(0xdcd60dcfL), tole(0xabd13d59L),
tole(0x26d930acL), tole(0x51de003aL), tole(0xc8d75180L), tole(0xbfd06116L),
tole(0x21b4f4b5L), tole(0x56b3c423L), tole(0xcfba9599L), tole(0xb8bda50fL),
tole(0x2802b89eL), tole(0x5f058808L), tole(0xc60cd9b2L), tole(0xb10be924L),
tole(0x2f6f7c87L), tole(0x58684c11L), tole(0xc1611dabL), tole(0xb6662d3dL),
tole(0x76dc4190L), tole(0x01db7106L), tole(0x98d220bcL), tole(0xefd5102aL),
tole(0x71b18589L), tole(0x06b6b51fL), tole(0x9fbfe4a5L), tole(0xe8b8d433L),
tole(0x7807c9a2L), tole(0x0f00f934L), tole(0x9609a88eL), tole(0xe10e9818L),
tole(0x7f6a0dbbL), tole(0x086d3d2dL), tole(0x91646c97L), tole(0xe6635c01L),
tole(0x6b6b51f4L), tole(0x1c6c6162L), tole(0x856530d8L), tole(0xf262004eL),
tole(0x6c0695edL), tole(0x1b01a57bL), tole(0x8208f4c1L), tole(0xf50fc457L),
tole(0x65b0d9c6L), tole(0x12b7e950L), tole(0x8bbeb8eaL), tole(0xfcb9887cL),
tole(0x62dd1ddfL), tole(0x15da2d49L), tole(0x8cd37cf3L), tole(0xfbd44c65L),
tole(0x4db26158L), tole(0x3ab551ceL), tole(0xa3bc0074L), tole(0xd4bb30e2L),
tole(0x4adfa541L), tole(0x3dd895d7L), tole(0xa4d1c46dL), tole(0xd3d6f4fbL),
tole(0x4369e96aL), tole(0x346ed9fcL), tole(0xad678846L), tole(0xda60b8d0L),
tole(0x44042d73L), tole(0x33031de5L), tole(0xaa0a4c5fL), tole(0xdd0d7cc9L),
tole(0x5005713cL), tole(0x270241aaL), tole(0xbe0b1010L), tole(0xc90c2086L),
tole(0x5768b525L), tole(0x206f85b3L), tole(0xb966d409L), tole(0xce61e49fL),
tole(0x5edef90eL), tole(0x29d9c998L), tole(0xb0d09822L), tole(0xc7d7a8b4L),
tole(0x59b33d17L), tole(0x2eb40d81L), tole(0xb7bd5c3bL), tole(0xc0ba6cadL),
tole(0xedb88320L), tole(0x9abfb3b6L), tole(0x03b6e20cL), tole(0x74b1d29aL),
tole(0xead54739L), tole(0x9dd277afL), tole(0x04db2615L), tole(0x73dc1683L),
tole(0xe3630b12L), tole(0x94643b84L), tole(0x0d6d6a3eL), tole(0x7a6a5aa8L),
tole(0xe40ecf0bL), tole(0x9309ff9dL), tole(0x0a00ae27L), tole(0x7d079eb1L),
tole(0xf00f9344L), tole(0x8708a3d2L), tole(0x1e01f268L), tole(0x6906c2feL),
tole(0xf762575dL), tole(0x806567cbL), tole(0x196c3671L), tole(0x6e6b06e7L),
tole(0xfed41b76L), tole(0x89d32be0L), tole(0x10da7a5aL), tole(0x67dd4accL),
tole(0xf9b9df6fL), tole(0x8ebeeff9L), tole(0x17b7be43L), tole(0x60b08ed5L),
tole(0xd6d6a3e8L), tole(0xa1d1937eL), tole(0x38d8c2c4L), tole(0x4fdff252L),
tole(0xd1bb67f1L), tole(0xa6bc5767L), tole(0x3fb506ddL), tole(0x48b2364bL),
tole(0xd80d2bdaL), tole(0xaf0a1b4cL), tole(0x36034af6L), tole(0x41047a60L),
tole(0xdf60efc3L), tole(0xa867df55L), tole(0x316e8eefL), tole(0x4669be79L),
tole(0xcb61b38cL), tole(0xbc66831aL), tole(0x256fd2a0L), tole(0x5268e236L),
tole(0xcc0c7795L), tole(0xbb0b4703L), tole(0x220216b9L), tole(0x5505262fL),
tole(0xc5ba3bbeL), tole(0xb2bd0b28L), tole(0x2bb45a92L), tole(0x5cb36a04L),
tole(0xc2d7ffa7L), tole(0xb5d0cf31L), tole(0x2cd99e8bL), tole(0x5bdeae1dL),
tole(0x9b64c2b0L), tole(0xec63f226L), tole(0x756aa39cL), tole(0x026d930aL),
tole(0x9c0906a9L), tole(0xeb0e363fL), tole(0x72076785L), tole(0x05005713L),
tole(0x95bf4a82L), tole(0xe2b87a14L), tole(0x7bb12baeL), tole(0x0cb61b38L),
tole(0x92d28e9bL), tole(0xe5d5be0dL), tole(0x7cdcefb7L), tole(0x0bdbdf21L),
tole(0x86d3d2d4L), tole(0xf1d4e242L), tole(0x68ddb3f8L), tole(0x1fda836eL),
tole(0x81be16cdL), tole(0xf6b9265bL), tole(0x6fb077e1L), tole(0x18b74777L),
tole(0x88085ae6L), tole(0xff0f6a70L), tole(0x66063bcaL), tole(0x11010b5cL),
tole(0x8f659effL), tole(0xf862ae69L), tole(0x616bffd3L), tole(0x166ccf45L),
tole(0xa00ae278L), tole(0xd70dd2eeL), tole(0x4e048354L), tole(0x3903b3c2L),
tole(0xa7672661L), tole(0xd06016f7L), tole(0x4969474dL), tole(0x3e6e77dbL),
tole(0xaed16a4aL), tole(0xd9d65adcL), tole(0x40df0b66L), tole(0x37d83bf0L),
tole(0xa9bcae53L), tole(0xdebb9ec5L), tole(0x47b2cf7fL), tole(0x30b5ffe9L),
tole(0xbdbdf21cL), tole(0xcabac28aL), tole(0x53b39330L), tole(0x24b4a3a6L),
tole(0xbad03605L), tole(0xcdd70693L), tole(0x54de5729L), tole(0x23d967bfL),
tole(0xb3667a2eL), tole(0xc4614ab8L), tole(0x5d681b02L), tole(0x2a6f2b94L),
tole(0xb40bbe37L), tole(0xc30c8ea1L), tole(0x5a05df1bL), tole(0x2d02ef8dL)
};

extern int do_nand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

extern int do_mmcops (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_fat_ls (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_ubi(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
extern int do_ubifs_ls(cmd_tbl_t *cmdtp, int flag, int argc,
                char * const argv[]);
extern int do_ubifs_load(cmd_tbl_t *cmdtp, int flag, int argc,
				char * const argv[]);
extern int do_ubifs_mount(cmd_tbl_t *cmdtp, int flag, int argc,
				char * const argv[]);


static uint32_t  gk_crc32_no_comp(uint32_t crc, const char *buf, uint32_t len)
{
    const uint32_t *tab = crc_table;
    const uint32_t *b =(const uint32_t *)buf;
    size_t rem_len;

    crc = cpu_to_le32(crc);
    /* Align it */
    if (((long)b) & 3 && len) {
     uint8_t *p = (uint8_t *)b;
     do {
          DO_CRC(*p++);
     } while ((--len) && ((long)p)&3);
     b = (uint32_t *)p;
    }

    rem_len = len & 3;
    len = len >> 2;
    for (--b; len; --len) {
     /* load data 32 bits wide, xor data 32 bits wide. */
     crc ^= *++b; /* use pre increment for speed */
     DO_CRC(0);
     DO_CRC(0);
     DO_CRC(0);
     DO_CRC(0);
    }
    len = rem_len;
    /* And the last few bytes */
    if (len) {
     uint8_t *p = (uint8_t *)(b + 1) - 1;
     do {
          DO_CRC(*++p); /* use pre increment for speed */
     } while (--len);
    }

    return le32_to_cpu(crc);
}

static unsigned int gk_crc32 (unsigned int crc, const char *p, unsigned int len)
{
     return gk_crc32_no_comp(crc ^ 0xffffffffL, p, len) ^ 0xffffffffL;
}

int crc_check(char *buf,unsigned int size,unsigned int pkt_crc)
{
    unsigned int crc;
    crc = gk_crc32(0,buf,size);
    printf("crc_ret = 0x%x  pkt_crc = 0x%x, size:%d\n",crc,pkt_crc, size);
    if(crc != pkt_crc)
        return -1;
    return 0;
}

int get_partition_inf(char *buf,unsigned int size,pkt_cfg *pkt_info_cfg)
{
    char* cfg_buf = buf;
    int flash_size;
    int pkt_addr;

    if(strncmp(cfg_buf,"flash_size",strlen("flash_size")) == 0)
    {
        cfg_buf += strlen("flash_size");
        while(!(*cfg_buf > 0x30 && *cfg_buf < 0x39))
        {
            cfg_buf++;
        }
        flash_size = simple_strtoul(cfg_buf,&cfg_buf,10);
        if(flash_size < 0 || flash_size > 64*1024)
        {
            printf("unvaild flash_size %d,please check \n",flash_size);
            return -1;
        }
        else
        {
            printf("flash_size = %dK buf ddr %p\n",flash_size,buf);
            pkt_info_cfg->flash_size = flash_size * 1024;
        }
    }
    else
    {
        printf("flash_size table can not found,please check image.cfg file\n");
        return -1;
    }

    while(cfg_buf <= (buf + size))
    {
        while(strncmp(cfg_buf,"0x",strlen("0x")) != 0)
        {
            cfg_buf++;
            if(cfg_buf >= (buf + size))
            {
                if(pkt_info_cfg->pkt_num > 1)
                    break;
                else
                    return -1;
            }
        }
        pkt_addr = simple_strtoul(cfg_buf,&cfg_buf,16);
        if(pkt_addr < 0 || pkt_addr > pkt_info_cfg->flash_size)
        {
            printf("unvaild pkt size %d,please check \n",pkt_addr);
            return -1;
        }
        else
        {
            if(pkt_info_cfg != NULL)
            {
                if((pkt_info_cfg->pkt_num > 0 && pkt_addr != 0) || pkt_info_cfg->pkt_num == 0)
                {
                    unsigned int pkt_len = 0;
                    pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num].addr = pkt_addr;
                    if(pkt_info_cfg->pkt_num > 0)
                    {
                        pkt_len  = pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num].addr - pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num - 1].addr;
                        if (pkt_len < 0)
                        {
                            pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num - 1].len = 0;
                            break;
                        }
                        pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num - 1].len = pkt_len;
                    }
                    pkt_info_cfg->pkt_num ++;
                }
            }
        }
    }

    pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num - 1].len = pkt_info_cfg->flash_size - pkt_info_cfg->pkt_info[pkt_info_cfg->pkt_num - 1].addr;

    cfg_buf = buf;
    while(strncmp(cfg_buf,"crc=",strlen("crc=")) != 0)
    {
        if(cfg_buf < (buf + size))
        {
            cfg_buf++;
        }
        else
        {
            printf("crc tab not found\n");
            return -1;
        }
    }
    cfg_buf += strlen("crc=");
    pkt_info_cfg->crc = simple_strtoul(cfg_buf,&cfg_buf,16);

    while(strncmp(cfg_buf,"update_zone=",strlen("update_zone=")) != 0)
    {
        if(cfg_buf < (buf + size))
        {
            cfg_buf++;
        }
        else
        {
            printf("update_zone tab not found\n");
            return -1;
        }
    }
    cfg_buf += strlen("update_zone=");
    pkt_info_cfg->update_zone = simple_strtoul(cfg_buf,&cfg_buf,2);
    if((pkt_info_cfg->update_zone < 0) || (pkt_info_cfg->update_zone > 0xFFFFFFFF))
    {
        printf("update_zone tab is a error num 0x%x please check\n",pkt_info_cfg->update_zone);
        return -1;
    }
    return 0;
}


void gk_update_flash(char *buf,pkt_cfg *pkt_info_cfg,char up_boot)
{
        cmd_tbl_t * cmdtp = NULL;
        int flag =2,argc;
        int i = 0;
        int ret = -1;
        char begin_addr[16],length[16],mem_addr[16];
        argc = 3;

        for(i = 0; i < pkt_info_cfg->pkt_num;i++)
        {
            if((pkt_info_cfg->update_zone >> i) & 0x1)
            {
                sprintf(begin_addr,"0x%x",pkt_info_cfg->pkt_info[i].addr);
                sprintf(length,"0x%x",pkt_info_cfg->pkt_info[i].len);
                char *argv1 []={"nand","erase",begin_addr,length};// erase
                argc = 4;
                if(pkt_info_cfg->pkt_info[i].addr > 0 || up_boot)
                {
                    ret = do_nand(cmdtp, flag, argc,  argv1);
                    if(ret != 0)
                    {
                        printf("erase bank %d addr 0x%x size %d %s\n",i,pkt_info_cfg->pkt_info[i].addr,pkt_info_cfg->pkt_info[i].len,(ret == 0)? "sucess":"failed");
                        return;
                    }

                    sprintf(mem_addr,"0x%x",(unsigned int)(buf) + pkt_info_cfg->pkt_info[i].addr);
                    char *argv2 []={"nand","write.e",mem_addr,begin_addr,length};
                    argc = 5;
                    ret = do_nand(cmdtp, flag, argc,  argv2);
                    if(ret != 0)
                    {
                        printf("write bank %d mem %s addr 0x%x size %d %s\n",i,mem_addr,pkt_info_cfg->pkt_info[i].addr,pkt_info_cfg->pkt_info[i].len,(ret == 0)? "sucess":"failed");
                        return;
                    }
                }
            }
        }

#ifdef CONFIG_SYS_AUTO_UPDATE
        recovery_count = 0;
        gk_set_reserved_update_flag_config(SYS_NEED_BOOT_APP);
        gk_save_reserved_config();
#endif
#if 0
		//用来保存sd卡升级标志
		if((pkt_info_cfg->update_zone >> 2) & 0x1){		
			int cn = pkt_info_cfg->pkt_num-2;//8;

            for (i = 0; i < pkt_info_cfg->pkt_num; i++)
            {
                if (pkt_info_cfg->pkt_info[i].addr == 0xff0000)
                {
                    //
                    if (cn != i - 1)
                    {
                        cn = i - 1;
                        printf("reserve error.\n");
                    }
                    break;
                }
            }

            if (pkt_info_cfg->pkt_info[cn].addr == 0xff0000)
                return;
			sprintf(begin_addr,"0x%x",pkt_info_cfg->pkt_info[cn].addr);
			sprintf(length,"0x010000");
			
			printf("sd flash begin_addr=%s,length=%s,cn=%d\n",begin_addr,length,cn);
			
			char *cmd1 []={"nand","erase",begin_addr,length};// erase
			argc = 4;
			if(pkt_info_cfg->pkt_info[cn].addr > 0 || up_boot)
			{
				ret = do_nand(cmdtp, flag, argc,cmd1);
				if(ret != 0)
				{
					printf("erase bank %d addr 0x%x size %d %s\n",i,pkt_info_cfg->pkt_info[cn].addr,pkt_info_cfg->pkt_info[cn].len,(ret == 0)? "sucess":"failed");
					return;
				}
				char *imageAddr = (char *)IMAGE_MEM_ADDR;
				char sdUpFlagBuf[32]={"sdupfinish"};
				memcpy(imageAddr,sdUpFlagBuf,32);
				sprintf(mem_addr,"0x%x",(unsigned int)(imageAddr));
				char *cmd2 []={"nand","write.e",mem_addr,begin_addr,length};
				argc = 5;
				ret = do_nand(cmdtp, flag, argc,cmd2);
				if(ret != 0)
				{
					printf("write bank %d mem %s addr 0x%x size %d %s\n",i,mem_addr,pkt_info_cfg->pkt_info[cn].addr,pkt_info_cfg->pkt_info[cn].len,(ret == 0)? "sucess":"failed");
					return;
				}
			}
		}
#endif		
        printf("update flash finish,do reset\n");
        flag = 1;
        argc = 1;
        char *argv_reset []={"reset"};
        do_reset(cmdtp, flag, argc, argv_reset);
        return;
}

void gk_read_update_zone(void* data, int addr, int len)
{
    int retVal = 0;
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    char begin_addr[16],length[16],mem_addr[16];
    int mtd_size = len;

    sprintf(mem_addr,"0x%x",(unsigned int)data);
    sprintf(begin_addr,"0x%x",addr);
    sprintf(length,"0x%x",mtd_size);

    memset(data, 0xff, len);
	printf("nand read %s %s %s\n", mem_addr,begin_addr,length);
    char *argv1 []={"nand","read",mem_addr,begin_addr,length};
    argc = 5;
    retVal = do_nand(cmdtp, flag, argc, argv1);
    if(retVal != 0)
    {
        printf("[%s:%d]read flash failed\n",__func__,__LINE__);
    }

}

int gk_get_nand_badblock_cnt(int addr, int len)
{
    int retVal = 0;
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    char begin_addr[16],length[16],mem_addr[16];
    int mtd_size = len;

    sprintf(begin_addr,"0x%x",addr);
    sprintf(length,"0x%x",len);

	printf("nand bad %s %s\n", begin_addr,length);
    char *argv1 []={"nand","bad",begin_addr,length};
    argc = 4;
    retVal = do_nand(cmdtp, flag, argc, argv1);
    if(retVal != 0)
    {
        printf("[%s:%d]read flash failed\n",__func__,__LINE__);
    }

    char *badCntStr = env_get("badcnt");
    int badCnt = 0;
    if (badCntStr != NULL)
    {
        badCnt = simple_strtoul(badCntStr, &badCntStr, 10);
        printf("bad cnt:%d\n", badCnt);
    }
    return badCnt;
}


int gk_read_update_ubi(void* data, int addr, int len)
{
    int retVal = 0;
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    int fileSize = 0;
    char *fileSizeStr = NULL;
    char mem_addr[32];

    char *argv1 []={"ubifsls", UBIFS_UPGRADE_ALL};
    argc = 2;
    retVal = do_ubifs_ls(cmdtp, flag, argc, argv1);
    if(retVal != 0)
    {
        printf("do_ubifs_ls %s failed.\n", UBIFS_UPGRADE_ALL);
        return -1;
    }

    memset(data, 0xFF, len);
    sprintf(mem_addr,"0x%x",(unsigned int)data);

	printf("ubifs load %s\n", mem_addr);
    char *argv2 []={"ubifsload", mem_addr, UBIFS_UPGRADE_ALL};
    argc = 3;
    retVal = do_ubifs_load(cmdtp, flag, argc, argv2);
    if(retVal != 0)
    {
        printf("[%s:%d]do_ubifs_load failed\n",__func__,__LINE__);
    }

    
    fileSizeStr = env_get("filesize");
    if (fileSizeStr != NULL)
    {
        fileSize = simple_strtoul(fileSizeStr, &fileSizeStr, 16);
        printf("ubi file size:%d-%s\n", fileSize, fileSizeStr);
    }

    return 0;
}


int gk_check_partition_crc(char* ptk_buf, pkt_cfg* pkt_info_cfg)
{
    unsigned int uboot_zone_crc = 0;
    unsigned int app_zone_crc = 0;
    char *buffer = (char *)IMAGE_MEM_ADDR + pkt_info_cfg->flash_size;
    int i = 0;

    if ((NULL == ptk_buf) || (NULL == pkt_info_cfg))
    {
        return -1;
    }

    for(i = 0;i < pkt_info_cfg->pkt_num;i++)
    {
        if (((pkt_info_cfg->update_zone >> i) & 0x1))
        {
            uboot_zone_crc = gk_crc32(0, ptk_buf + pkt_info_cfg->pkt_info[i].addr, pkt_info_cfg->pkt_info[i].len);
            gk_read_update_zone(buffer, pkt_info_cfg->pkt_info[i].addr, pkt_info_cfg->pkt_info[i].len);
            app_zone_crc = gk_crc32(0, buffer, pkt_info_cfg->pkt_info[i].len);
            printf("app_zone_crc = 0x%x  uboot_zone_crc = 0x%x   i = %d  update_zone = 0x%x\n",app_zone_crc,uboot_zone_crc,i,pkt_info_cfg->update_zone);
            if (app_zone_crc != uboot_zone_crc)
            {
                break;
            }
        }

        if (i == pkt_info_cfg->pkt_num-1)
        {
            printf("same img,exit update from sd card\n");
            return -1;
        }
    }

    return 0;
}

char *get_board_inf(const char *name, char *buf, unsigned int size)
{
    char* cfg_buf = buf;
    int board_size = 0;
    while(strncmp(cfg_buf, name, strlen(name)) != 0)
    {
        if(cfg_buf < (buf + size))
        {
            cfg_buf++;
        }
        else
        {
            printf("board not found\n");
            return NULL;
        }
    }
    cfg_buf += strlen(name);

    char *board = cfg_buf;
    while(*cfg_buf != 0)
    {
        cfg_buf++;
        board_size++;
    }
    cfg_buf -= board_size;
    memcpy(board, cfg_buf, board_size);
    return board;
}

#define BOARD_TYPE_SIZE  32
#define BOARD_TPYE_NAME  "board="
int gk_check_board(char* ptk_buf, pkt_cfg* pkt_info_cfg, char *cfg_file_buf, int cfg_file_size)
{
    char *buffer = (char *)IMAGE_MEM_ADDR + pkt_info_cfg->flash_size;
    gk_read_update_zone(buffer, pkt_info_cfg->pkt_info[1].addr, pkt_info_cfg->pkt_info[1].len);

    char *uboot_board = get_board_inf(BOARD_TPYE_NAME, buffer,  pkt_info_cfg->pkt_info[1].len);
    //char *pkt_board = get_board_inf(BOARD_TPYE_NAME, ptk_buf + pkt_info_cfg->pkt_info[1].addr,  pkt_info_cfg->pkt_info[1].len);
    //read board from ini file not from the bin file.
    char *pkt_board = get_board_inf(BOARD_TPYE_NAME, cfg_file_buf,  cfg_file_size);
    
    printf(">>>uboot_board=%s, pkt_board=%s\n", uboot_board, pkt_board);

    #ifndef CONFIG_FORCE_UPGRADE
    if((NULL == uboot_board) || (NULL == pkt_board))
    {
        printf("board not found, exit update from sd card, uboot_board=%s, pkt_board=%s\n", uboot_board, pkt_board);
        return -1;
    }
    if(strncmp(uboot_board, pkt_board, strlen(uboot_board)) != 0)
    {
        printf("board type mismatch, exit update from sd card\n");
        return -1;
    }
    #endif
    return 0;
}

int do_gkupdate ( cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    pkt_cfg pkt_info_cfg;
    loff_t cfg_size = 0;
    loff_t ptk_size = 0;
    char   up_uboot = 0;
    char   cmd[3] = {0};

    if(argc > 1)
    {
        sprintf(cmd,argv[1]);
        if(0 == strncmp(cmd,"all",strlen("all")))
        {
            up_uboot = 1;
            printf("Enable update uboot\n");
        }
    }

    cmd_tbl_t * mmc_cmdtp = NULL;
    int mmc_flag = 1;
    int mmc_argc = 2;
    char *mmc_argv []={"mmc","rescan"};
    int i;
    

    if(0 == do_mmcops( mmc_cmdtp,  mmc_flag,  mmc_argc,mmc_argv ))
    {
        char *cfg_file =   {"gkipc_image_sd_update.cfg"};
        char *image_file = {"gkipc_image.bin"};
        char *cfg_file_read = cfg_file; 
#ifdef CONFIG_FORCE_UPGRADE
        char *cfg_file2 =   {"gkipc_image_sd_update2.cfg"};
        char *image_file2 = {"gkipc_image2.bin"};
#endif
        char *image_file_read = image_file;
        cmd_tbl_t * fat_cmdtp = NULL;
        int fat_flag = 1;
        int fat_argc = 3;
        char *fat_argv []={"fatls","mmc","0:0"};
        char *cfg_prt    = (char *)CFG_MEM_ADDR;

        for(i = 0; i < 10; i ++)
        {
            sprintf(fat_argv[2], "0:%d", i);
            if(do_fat_ls(fat_cmdtp,fat_flag,fat_argc,fat_argv) != 0)
            {
                printf("fat ls %s err\n", fat_argv[2]);
                continue;
            }
            
            #ifdef CONFIG_FORCE_UPGRADE
            cfg_file_read = cfg_file2;
            #endif
            cfg_size = file_fat_read(cfg_file_read, cfg_prt, 0);
            if(cfg_size <= 0)
            {
                printf("bad %s,exit update from sd card\n", cfg_file_read);
                continue;
            }
            else
            {
                break;
            }
        }

        *(cfg_prt + cfg_size) = 0;
        printf("read gkipc_image_sd_update.cfg ok\n");
        memset(&pkt_info_cfg,0,sizeof(pkt_cfg));
        if (0 != get_partition_inf(cfg_prt,cfg_size,&pkt_info_cfg))
        {
            printf("exit gkupdate from sd card cfg_size = %lld\n",cfg_size);
            return -1;
        }

        int i = 0;
        for(i = 0;i < pkt_info_cfg.pkt_num;i++)
            printf("bank %d addr %#10x len %9d bytes up_enable%d\n",i,pkt_info_cfg.pkt_info[i].addr,pkt_info_cfg.pkt_info[i].len,((pkt_info_cfg.update_zone >> i) & 0x1));

        char *image_prt = (char *)IMAGE_MEM_ADDR;
        #ifdef CONFIG_FORCE_UPGRADE
        image_file_read = image_file2;
        #endif
        ptk_size = file_fat_read(image_file_read, image_prt, 0);
        if(ptk_size <= 0)
        {
            printf("bad %s,exit update from sd card\n", image_file_read);
            return -1;
        }

        
        if(crc_check(image_prt,ptk_size,pkt_info_cfg.crc) < 0 )
        {
            printf("bad crc check,exit update from sd card\n");
            return -1;
        }
        else
        {
            printf("crc check ok\n");
            if (gk_check_board(image_prt, &pkt_info_cfg, cfg_prt, cfg_size) < 0)
            {
                return -1;
            }
            #ifndef CONFIG_FORCE_UPGRADE
            if (gk_check_partition_crc(image_prt, &pkt_info_cfg) < 0)
            {
                return -1;
            }
            #endif
            gk_update_flash(image_prt,&pkt_info_cfg,up_uboot);
        }

    }
    return 0;
}




#ifdef CONFIG_SYS_AUTO_UPDATE
static int *buffer = NULL;
void gk_set_reserved_update_flag_config(int updateflag)
{
    *(buffer+CONFIG_RESERVE_FLASH_OFFSET) = updateflag;
    *(buffer+CONFIG_RESERVE_FLASH_OFFSET+1) = recovery_count;
}

void gk_read_reserved_config(void* data, int len)
{
    int retVal = 0;
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    char begin_addr[16],length[16],mem_addr[16];
    int mtd_size = len;//+8;//

	if(mtd_size > CONFIG_RESERVE_FLASH_SIZE)
	{
		printf("Read size (0x%x)large than mtd size (0x%x) error\n", mtd_size, CONFIG_RESERVE_FLASH_SIZE);
		return;
	}

    buffer = malloc(mtd_size);
    if(!buffer){
        printf("%s malloc mtd_size:%d error\n", __func__, mtd_size);
        return;
    }

	memset(buffer,0,mtd_size);


    sprintf(mem_addr,"0x%x",(unsigned int)buffer);
    sprintf(begin_addr,"0x%x",CONFIG_RESERVE_FLASH_ADDR);
    sprintf(length,"0x%x",mtd_size);
    char *argv1 []={"nand","read",mem_addr,begin_addr,length};
    argc = 5;
    retVal = do_nand(cmdtp, flag, argc, argv1);
    if(retVal != 0)
    {
        printf("[%s:%d]read flash failed\n",__func__,__LINE__);
    }

    memcpy(data, buffer, mtd_size);
}

void gk_save_reserved_config(void)
{
    int retVal = 0;
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    char begin_addr[16],length[16],mem_addr[16];

    sprintf(begin_addr,"0x%x",CONFIG_RESERVE_FLASH_ADDR);
    sprintf(length,"0x%x",CONFIG_RESERVE_FLASH_SIZE);
    char *argv1 []={"nand","erase",begin_addr,length};
    argc = 4;
    retVal = do_nand(cmdtp, flag, argc, argv1);
    if(retVal != 0)
    {
        printf("[%s:%d]erase flash failed\n",__func__,__LINE__);
    }

	memset(mem_addr,0,sizeof(mem_addr));
	memset(length,0,sizeof(length));
    sprintf(mem_addr,"0x%x",(unsigned int)buffer);
    sprintf(length,"0x%x",CONFIG_RESERVE_FLASH_SIZE);
    char *argv2 []={"nand","write.e",mem_addr,begin_addr,length};
    argc = 5;
    retVal = do_nand(cmdtp, flag, argc, argv2);
    if (0 != retVal)
    {
        printf("[%s:%d]do_nand write failed\n",__func__,__LINE__);
    }

    if(buffer)
    {
        free(buffer);
    }
}
#endif

typedef struct backups_head
{
	char partitinon_name[32];
	int update_flag;
	int partitnon_addr;
	int partitnon_size;
	int partitnon_data_size;
	unsigned int partitnon_crc;
	unsigned int magic;
}BACKUPS_HEAD_T;

#define MTD_NAME_BACKUPS    "Backups"
#define MTD_NAME_ALL        "all"
#define RESERVER_SIZE  (128*1024)
#define OLD_HEAD_SIZE (128*1024)//|update head(64K)|reserver(64K)|
#define NEW_HEAD_SIZE (128*1024)
#define BACKUPS_HEAD_MAGIC (0X12345678)
#define FLASH_BLOCK_SIZE (128*1024)

void backups_update_write_zone(void* data, int addr, int len, int eraseSize)
{
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    int i = 0;
    int ret = -1;
    char begin_addr[16],length[16],eraseLength[16],mem_addr[16];

		
	sprintf(begin_addr,"0x%x",addr);
	sprintf(eraseLength,"0x%x",eraseSize);
	char *argv1 []={"nand","erase",begin_addr,eraseLength};// erase
	argc = 4;
    printf("nand erase %s %s...\n", begin_addr, eraseLength);
	sprintf(length,"0x%x",len);
	if(addr >= 0)
	{
		ret = do_nand(cmdtp, flag, argc,  argv1);
		if(ret != 0)
		{
			printf("erase bank %d addr 0x%x size %d %s\n",i,addr,len,(ret == 0)? "sucess":"failed");
			return;
		}
		sprintf(mem_addr,"0x%x",(unsigned int)data);
		printf("nand write %s %s %s\n", mem_addr,begin_addr,length);
		char *argv2 []={"nand","write.e",mem_addr,begin_addr,length};
		argc = 5;
		ret = do_nand(cmdtp, flag, argc,  argv2);
		if(ret != 0)
		{
			printf("write bank %d mem %s addr 0x%x size %d %s\n",i,mem_addr,addr,len,(ret == 0)? "sucess":"failed");
			return;
		}
	}
	#if 0
	printf("update flash finish,do reset\n");
	flag = 1;
	argc = 1;
	char *argv_reset []={"reset"};
	do_reset(cmdtp, flag, argc, argv_reset);
	#endif

}

static int my_atoi(const char *str)
{    
    int value = 0;    
    int flag = 1;
    while (*str == ' ')
    {
        str++;    
    }
    if (*str == '-')
    {
        flag = 0;
        str++;    
    }    
    else if (*str == '+')
    {
        flag = 1;        
        str++;    
    }
    else if (*str >= '9' || *str <= '0')     
    {
        return 0;
    }
    //while (*str != '/0' && *str <= '9' && *str >= '0')    
    while (*str <= '9' && *str >= '0')    
    {
        value = value * 10 + *str - '0';
        str++;    
    }
    if (flag == 0)
    {
        value = -value;    
    }
    return value;
}

static int get_mtd_size(char *mtd_name)
{
    int err = 0;
    const char *parts;
    char *p = NULL;
    char *p1 = NULL;
    char *buff = NULL;
    int unitIsM = 0;
    int size = 0;

    if(mtd_name == NULL)
    {
        printf("get_mtd_size param err\n");
        return err;    
    }
    parts = env_get("mtdparts");    
    if(parts == NULL)
    {
        printf("env_get mtdparts err\n");
        return err;    
    }
    //printf("mtdparts=%s\n",parts);

    p = strstr(parts, mtd_name);
    if(p == NULL)
    {
        printf("not find mtd_name:%s\n",mtd_name);
        return err;
    }

    if(strstr(mtd_name, "all") != NULL)
        unitIsM = 1;

    buff = malloc(16);
    if(buff == NULL)
    {
        printf("malloc err\n");
        return err;
    }
    memset(buff, 0, 16);

    for(; p != parts; p--)
    {		
        if(strncmp(p, ",", 1) == 0)
        break;		
    }

    if(unitIsM == 1)
        p1 = strstr(p, "M");
    else
    {
        p1 = strstr(p, "K");
        if (p1 == NULL)
            p1 = strstr(p, "k");
    }

    strncpy(buff, p + 1, p1 - p - 1);
    size = my_atoi(buff);
    free(buff);

    if(unitIsM == 1)
        return (size * 1024 * 1024);
    else
        return (size * 1024);
}

static int get_mtd_addr(char *mtd_name)
{
    int err = 0;
    const char *parts;
    char *p = NULL;
    char *p1 = NULL;
    char *buff = NULL;
    int size = 0;
    
    if(mtd_name == NULL)
    {
        printf("get_mtd_addr param err\n");
        return err;
    }

    parts = env_get("mtdparts");    
    if(parts == NULL)
    {
        printf("env_get mtdparts err\n");
        return err;    
    }
    printf("mtdparts=%s\n",parts);
    
    p = strstr(parts, mtd_name);
    if(p == NULL)
    {
        printf("not find mtd_name:%s\n",mtd_name);
        return err;
    }

    buff = malloc(16);			
    if(buff == NULL)
    {
        printf("malloc err\n");
        return err;
    }

    //找到mtd_name前面的"," 跳过，继续找下一个","
    for(; p != parts; p--)
    {		
        if(strncmp(p, ",", 1) == 0)
        {
            p--;
            break;
        }
    }
    for(; p != parts; p--)
    {		
        if(strncmp(p, ",", 1) == 0)
        {
            p1 = strstr(p, "K");
            if (p1 == NULL)
                p1 = strstr(p, "k");
            memset(buff, 0, 16);
            strncpy(buff, p + 1, p1 - p - 1);
            size += my_atoi(buff);
            p--;
        }
        else if(strncmp(p, ":", 1) == 0)
        {
            p1 = strstr(p, "K");
            if (p1 == NULL)
                p1 = strstr(p, "k");
            memset(buff, 0, 16);
            strncpy(buff, p + 1, p1 - p - 1);
            size += my_atoi(buff);
            break;
        }			
    }
    free(buff);
    return (size * 1024);
}

int gk_nand_ubi_mount(void)
{
    int retVal = 0;
    cmd_tbl_t * cmdtp = NULL;
    int flag =2,argc;
    char begin_addr[16],length[16],mem_addr[16];

	printf("ubi mount %s %s %s\n", mem_addr,begin_addr,length);
    char *argv1 []={"ubi","part", UBIFS_PART_NAME};
    argc = 3;
    retVal = do_ubi(cmdtp, flag, argc, argv1);
    if(retVal != 0)
    {
        printf("[%s:%d]read flash failed\n",__func__,__LINE__);
    }
    
    char *argv2 []={"ubifsmount","ubi0"};
    argc = 2;
    retVal = do_ubifs_mount(cmdtp, flag, argc, argv2);
    if(retVal != 0)
    {
        printf("[%s:%d]do_ubifs_mount failed\n",__func__,__LINE__);
    }
    
    return retVal;
    
}

void gkupdate_check(void)
{
	unsigned int crc = 0;
	char *buffer = (char *)IMAGE_MEM_ADDR;
	BACKUPS_HEAD_T old_head;
	int new_head_offset, old_head_offset, old_buff_offset;
	char *old_head_buffer = NULL;
    int mtd_size_backups = 0;
    int mtd_addr_backups = 0;
    int isReboot = 0;

    mtd_size_backups = get_mtd_size(MTD_NAME_BACKUPS);
    if(mtd_size_backups == 0)
    {
        printf("get backups size err.\n");
        return ;
    }
    //printf("mtd_size_backups:%d\n", mtd_size_backups);
    #if 0
	mtd_size_all = get_mtd_size(MTD_NAME_ALL);
    if(mtd_size_all == 0)
    {
        printf("get all size err.\n");
        return ;
    }
    #endif
	mtd_addr_backups = get_mtd_addr(MTD_NAME_BACKUPS);
    if(mtd_addr_backups == 0)
    {
        printf("get backups addr err.\n");
        return ;
    }
    //printf("mtd_addr_backups:0x%x\n", mtd_addr_backups);
	/*read update flag */
	//jump to updata head addr
	new_head_offset = mtd_addr_backups + mtd_size_backups - NEW_HEAD_SIZE;
	old_head_offset = mtd_addr_backups;
    old_buff_offset = mtd_addr_backups + OLD_HEAD_SIZE;

	old_head_buffer = malloc(OLD_HEAD_SIZE);
	if(old_head_buffer == NULL)
	{
		printf("malloc error  %s, %d\n", __func__, __LINE__);
		return ;
	}
    #if 0
	new_head_buffer = malloc(OLD_HEAD_SIZE);
	if(new_head_buffer == NULL)
	{
		printf("malloc error  %s, %d\n", __func__, __LINE__);
		free(old_head_buffer);
		return ;
	}	
    #endif
	memset(old_head_buffer, 0, OLD_HEAD_SIZE);
	//memset(new_head_buffer, 0, OLD_HEAD_SIZE);
	gk_read_update_zone(old_head_buffer, old_head_offset, OLD_HEAD_SIZE);
	//gk_read_update_zone(new_head_buffer, new_head_offset, NEW_HEAD_SIZE);
	memcpy(&old_head, old_head_buffer, sizeof(BACKUPS_HEAD_T));
	//memcpy(&new_head, new_head_buffer, sizeof(BACKUPS_HEAD_T));
	if(old_head.magic != BACKUPS_HEAD_MAGIC)
	{
		printf("old head magic error\n");
		return ;
	}
	printf(" %s, %d, update flag:%d, addr:0x%x, size:0x%x, file size:0x%x\n",
		__func__, __LINE__, old_head.update_flag, old_head.partitnon_addr, old_head.partitnon_size, old_head.partitnon_data_size);	
	#if 0
	buffer = malloc(old_head.partitnon_size);
	if(buffer == NULL)
	{
		printf("%s, %d malloc error\n", __func__, __LINE__);
		return ;
	}
	#endif
    // && (old_head.partitnon_addr < mtd_size_all) && (old_head.partitnon_size < mtd_size_all)
	if((old_head.update_flag == 1))
	{
        //gk_nand_ubi_mount();
        //nand有坏块，是用nand read时需要从分区开始读取需要的数据
		gk_read_update_zone(buffer, old_head_offset, old_head.partitnon_data_size + OLD_HEAD_SIZE);
		crc = gk_crc32(0, buffer + OLD_HEAD_SIZE, old_head.partitnon_data_size);
		if(crc != old_head.partitnon_crc)
        {
            printf("old partitnon data err\n");
        }
        else
        {
            gk_read_update_zone(buffer, old_head.partitnon_addr, old_head.partitnon_size);
            crc = gk_crc32(0, buffer, old_head.partitnon_data_size);
            if(crc != old_head.partitnon_crc)
            {
                printf("mtd crc is not equal to backups partitnon, crc:0x%x, head crc:0x%x\n", crc, old_head.partitnon_crc);
                /*reset partitnon data */
                printf("read backups data\n");
                memset(buffer, 0xff, old_head.partitnon_size);
                gk_read_update_zone(buffer, old_head_offset, old_head.partitnon_data_size + OLD_HEAD_SIZE);
                int badCnt = gk_get_nand_badblock_cnt(old_head.partitnon_addr, old_head.partitnon_size);
                printf("write backups data 2:0x%x, size:0x%x, partsize:0x%x, bad:%d\n", old_head.partitnon_addr, old_head.partitnon_data_size, old_head.partitnon_size, badCnt);
                backups_update_write_zone(buffer + OLD_HEAD_SIZE, old_head.partitnon_addr, old_head.partitnon_data_size, old_head.partitnon_size);
                printf("write backups data end\n");
            }
        }

		//set old_head.update_flag is 0
		printf("set old_head.update_flag is 0\n");
		old_head.update_flag = 0;
		memset(old_head_buffer, 0, OLD_HEAD_SIZE);
		memcpy(old_head_buffer, &old_head, sizeof(old_head));
		backups_update_write_zone(old_head_buffer, old_head_offset, OLD_HEAD_SIZE, OLD_HEAD_SIZE);
	}
    else if (old_head.update_flag == 2)
    {
        int curAdd = old_head_offset;
		gk_read_update_zone(buffer, old_head.partitnon_addr, old_head.partitnon_data_size);
		crc = gk_crc32(0, buffer, old_head.partitnon_data_size);
		if(crc != old_head.partitnon_crc)
        {
            printf("old partitnon data err\n");
        }
        else
        {
            gk_read_update_zone(buffer, 0, old_head.partitnon_size);
            crc = gk_crc32(0, buffer, old_head.partitnon_data_size);
            if(crc != old_head.partitnon_crc)
            {
                printf("mtd crc is not equal to backups partitnon, crc:0x%x, head crc:0x%x\n", crc, old_head.partitnon_crc);
                /*reset partitnon data */
                printf("read backups data\n");
                memset(buffer, 0xff, old_head.partitnon_size);
                gk_read_update_zone(buffer, old_head.partitnon_addr, old_head.partitnon_data_size);
                printf("write backups data 2:0x%x, size:0x%x\n", old_head.partitnon_addr, old_head.partitnon_size);

                //nand write会跳过坏块，如果一次性写整体，有坏块的话会导致写的数据不是真实想写的地址
                //所以需要读取所有分区，按分区写，即使写超过都后面分区了，后面分区也会重新从起始地址擦写
                //TODO: 假设env分区的地址不会变为0x80000开始
                char cmpHeader[] = "=nand:";
                char mtdInfo[512] = {0};
                char mtdSizeStr[32] = {0};
                int cnt = 0;
                int tmpSize = 0;
                int i; 
                int isWrite = 1;
                int isUnitK = 0;
                char *p = NULL;
                char *p1 = NULL;
                char *tmpAdd = NULL;
                int rdsize = 128 * 1024;
                char *tmpBuf = buffer + 0x80000;
                int headerLen = strlen(cmpHeader);
                curAdd = 0;
                for (i = 0; i < rdsize; i++)
                {
                    if (memcmp(tmpBuf + i, cmpHeader, headerLen) == 0)
                    {
                        strcpy(mtdInfo, tmpBuf + i + headerLen);
                        printf("find mtds:\n%s\n", mtdInfo);

                        if (strstr(mtdInfo, "K(") != NULL)
                            isUnitK = 1;
                        tmpAdd = mtdInfo;
                        p = strstr(tmpAdd, ",");
                        p[0] = 0;//赋0便于strstr找分区名
                        while(p != NULL)
                        {
                            memset(mtdSizeStr, 0, 32);
                            p1 = strstr(tmpAdd, "K(");
                            strncpy(mtdSizeStr, tmpAdd, p1 - tmpAdd);
                            tmpSize = my_atoi(mtdSizeStr);
                            tmpSize *= 1024;
                            printf("addr:0x%x,size:0x%x\n", curAdd, tmpSize);
                            if (isWrite)
                            {
                                backups_update_write_zone(buffer + curAdd, curAdd, tmpSize, tmpSize);
                            }
                            if (strstr(tmpAdd, "Rootfs") != NULL)
                            {
                                //已写到rootfs,不再继续擦写，但是要继续找到Backups分区地址
                                isWrite = 0;
                            }

                            if (strstr(tmpAdd, "Backups") != NULL)
                            {
                                //找到，停止
                                break;
                            }
                            curAdd += tmpSize;
                            tmpAdd = p + 1;
                            p = strstr(tmpAdd, ",");
                            if (p != NULL)
                                p[0] = 0;
                        }
                        
                        isReboot = 1;
                        break;
                    }
                }
                
                printf("write backups data end\n");
            }
        }

		//因为可能分区修改，所以还往以前标识分区写可能写到有效数据去，所以需要根据新bin分区的地址来写
		printf("set old_head.update_flag is 0, addr:0x%x\n", curAdd);
		old_head.update_flag = 0;
		memset(old_head_buffer, 0, OLD_HEAD_SIZE);
		memcpy(old_head_buffer, &old_head, sizeof(old_head));
		backups_update_write_zone(old_head_buffer, curAdd, OLD_HEAD_SIZE, OLD_HEAD_SIZE);

        if (isReboot)
        {
            printf("reboot\n");
            cmd_tbl_t * cmdtp = NULL;
            int flag = 1;
            int argc = 1;
            char *argv_reset []={"reset"};
            do_reset(cmdtp, flag, argc, argv_reset);
        }
    }
}
void do_mmc_gkupdate(void)
{
    gkupdate_check();
    #if 1
    cmd_tbl_t * gkupdate_cmdtp = NULL;
    int gkupdate_flag = 1;
    int gkupdate_argc = 2;
    char *gkupdate_argv []={"gkupdate","all"};
    do_gkupdate(gkupdate_cmdtp,  gkupdate_flag,  gkupdate_argc, gkupdate_argv);
    #endif
}

/*-----------------------------------------------------------------------
 */

/***************************************************/


U_BOOT_CMD(
    gkupdate,   2,  1,  do_gkupdate,
    "Gk_update sub-system",
    "use gkupdate all to enable uboot update"
);

