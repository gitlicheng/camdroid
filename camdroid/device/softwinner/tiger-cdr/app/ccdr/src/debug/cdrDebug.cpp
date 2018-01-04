#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sunxi_display2.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <cutils/log.h>


static unsigned long *addr_vir=0;
static unsigned long *addr_phy=0;
static HDC dc=0;

#define debug_buf_w 85
#define debug_buf_h 16


#define CHECK_PATH "/proc/meminfo"
#define CHECK_NODE "MemFree"

static int mDisp_fd=0;

extern "C" {
extern int MemAdapterOpen(void);
extern void MemAdapterClose(void);
extern void* MemAdapterPalloc(int nSize);
extern void  MemAdapterPfree(void* pMem);
extern void  MemAdapterFlushCache(void* pMem, int nSize);
extern void* MemAdapterGetPhysicAddress(void* pVirtualAddress);
extern void* MemAdapterGetVirtualAddress(void* pPhysicAddress);
extern void* MemAdapterGetPhysicAddressCpu(void* pVirtualAddress);
extern void* MemAdapterGetVirtualAddressCpu(void* pCpuPhysicAddress);
}


static int layer_config(__DISP_t cmd, disp_layer_config *pinfo)
{
	unsigned long args[4] = {0};
	unsigned int ret = 0;
	
	args[0] = 0;
    args[1] = (unsigned long)pinfo;
    args[2] = 1;
    ret = ioctl(mDisp_fd, cmd, args);
	if(0 != ret) {
		ALOGD("fail to get para\n");
		ret = -1;
	}
	return ret;
}
static int layer_set_para(disp_layer_config *pinfo)
{
	return layer_config(DISP_LAYER_SET_CONFIG, pinfo);
}

static int layer_get_para(disp_layer_config *pinfo)
{
	return layer_config(DISP_LAYER_GET_CONFIG, pinfo);
}


static void layer_debug_init()
{
	ALOGD("%s %d\n", __FUNCTION__, __LINE__);
	mDisp_fd = open("/dev/disp", O_RDWR);
	disp_layer_config debug_layer_config, ui_config;
	ui_config.channel = 2;
	ui_config.layer_id = 0;
	layer_get_para(&ui_config); //get ui para
	debug_layer_config = ui_config;
	debug_layer_config.channel = 2;
	debug_layer_config.layer_id = 2;
	debug_layer_config.enable = 1;
	debug_layer_config.info.zorder = 11;
	debug_layer_config.info.fb.addr[0] = (long long unsigned int)addr_phy;
	debug_layer_config.info.fb.size[0].width = debug_buf_w;
	debug_layer_config.info.fb.size[0].height = debug_buf_h;
	debug_layer_config.info.fb.crop.x = 0;
	debug_layer_config.info.fb.crop.y = 0;
	debug_layer_config.info.fb.crop.width  = debug_buf_w;
	debug_layer_config.info.fb.crop.height = debug_buf_h;
	debug_layer_config.info.fb.crop.x = debug_layer_config.info.fb.crop.x << 32;
	debug_layer_config.info.fb.crop.y = debug_layer_config.info.fb.crop.y << 32;
    debug_layer_config.info.fb.crop.width  = debug_layer_config.info.fb.crop.width << 32;
    debug_layer_config.info.fb.crop.height = debug_layer_config.info.fb.crop.height << 32;
	debug_layer_config.info.screen_win.x      = 320-debug_buf_w;
	debug_layer_config.info.screen_win.y      = 240-debug_buf_h;
	debug_layer_config.info.screen_win.width  = debug_buf_w;
	debug_layer_config.info.screen_win.height = debug_buf_h;
	layer_set_para(&debug_layer_config);
}

static int debug_get_info(){
       char line[128];
       FILE *fp = fopen(CHECK_PATH, "r");
       char num[32]="0";
       if(fp==NULL){
        printf("open file failed.");
        return 0;
       }
       while(fgets(line, sizeof(line), fp)) {
               if (strncmp(line, CHECK_NODE, strlen(CHECK_NODE)) == 0) {
                        puts(line);
                        char *str = strstr(line, ":");
                        str += 1;
                        sscanf(str, "%[^a-z]", num);
                        break;
               }
       }
       fclose(fp);
       return atoi(num);

}

static void debug_mem_init()
{
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned char *fb_vir, *fb_phy;
	int fd_fb = open("/dev/graphics/fb0", O_RDWR);
	ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix);
    ioctl(fd_fb, FBIOGET_VSCREENINFO, &var);
	int x = debug_buf_w;
	int y = debug_buf_h;
	int Bpp = var.bits_per_pixel / 8;
	int one_frame_size = Bpp * x * y;
	ALOGD("<**Bpp:%d one_frame_size:%d x:%d y:%d**>",Bpp,one_frame_size,x,y);
	MYBITMAP myBitmap;
	MemAdapterOpen();
		addr_vir = (unsigned long *)MemAdapterPalloc(one_frame_size);
		memset(addr_vir, 0xff, one_frame_size);
		addr_phy = (unsigned long *)MemAdapterGetPhysicAddressCpu((void*)addr_vir);
		myBitmap.flags  = MYBMP_FLOW_DOWN | MYBMP_TYPE_RGB | MYBMP_ALPHA;
		myBitmap.frames = 1;
		myBitmap.depth  = var.bits_per_pixel;
		myBitmap.w      = debug_buf_w;
		myBitmap.h      = debug_buf_h;
		myBitmap.pitch  = debug_buf_w * 4;
		myBitmap.size   = debug_buf_w*debug_buf_h*4;
		myBitmap.bits   = (BYTE*)addr_vir;
		RGB pal[256] = {{0,0,0,0}};
		dc = CreateMemDCFromMyBitmap (&myBitmap,pal);
		ALOGD("dc :0x%x", dc);
}

static void debug_mem_exit()
{
	if(addr_vir) {
		if (dc) {
			DeleteMemDC(dc);
		}
		MemAdapterPfree(addr_vir);
	}
	MemAdapterClose();
}

static void* _debug_start(void *arg)
{
	int num;
	char str[16];
	while(1) {
		num = debug_get_info();
		sprintf(str, "%8dkb", num);
		TextOut(dc, 0, 0, str);
		
		usleep(500*1000);
	}
	return NULL;
}

void debug_start()
{
	debug_mem_init();
	layer_debug_init();
	
	int err;
    pthread_t ntid;
    err = pthread_create(&ntid, NULL, _debug_start, NULL);	
}

void debug_end()
{
	debug_mem_exit();
	close(mDisp_fd);
}

