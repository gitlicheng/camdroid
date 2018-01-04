
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define FB_WIDTH    320
#define FB_HEIGHT   240
#define BIT_PER_PIX 32
#define PIX_LENGTH (BIT_PER_PIX / 8)
#define LINE_LENGTH (FB_WIDTH * PIX_LENGTH) 
#define LENGTH (LINE_LENGTH * FB_HEIGHT)

unsigned long *buf;

int main(int argc, char *argv[])
{
	int fd;
	unsigned long color;
	char *pos;

	if (argc != 2)
	{
		usage();
		return -1;
	}
	
	if ((fd = open("/dev/graphics/fb0", O_RDWR)) < 0)
	{
		printf("open /dev/fb0 failed\n");
	//	return -1;
	}
	buf = (unsigned long *)mmap(NULL, LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	color = strtol(argv[1],&pos,16);
	fprintf(stderr, "color:0x%x", color);
	//color = atoi(argv[1]);
		/* mmap for the app */
	unsigned long  *ctt	= mmap(NULL, LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);//在isp_mmap调用remap_pfn_range(vma,vma->vm_start,mypfn,vmsize,vma->vm_page_prot)吧物理映射成虚拟
	/* set the display content */
	int cnt = 0;
	printf("%x\n", color);
//	color = 0x10ff0000;
	while(cnt < LENGTH)
	{
		memcpy(ctt+(cnt/4), &color, 4);
		cnt += 4;
		
	}
	munmap(ctt, LENGTH);
	close(fd);
	return 0;
}

int usage(void)
{
	fprintf(stderr, "memset fb0 color\n");
	fprintf(stderr, "usage: fb_color <color>\n");
	return 1;
}

