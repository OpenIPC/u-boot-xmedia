
#ifndef __XMEDIA_IMAGE_H_
#define __XMEDIA_IMAGE_H_    1

/* HW GZIP Image head */
bool check_hwzimage_header(unsigned char *header);
int do_go_hwzimage(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#endif /* __XMEDIA_IMAGE_H_ */
