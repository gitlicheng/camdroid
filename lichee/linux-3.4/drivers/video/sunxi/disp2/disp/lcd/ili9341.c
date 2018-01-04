#include "ili9341.h"

#define TCON_DEBUG		(0)
#define CPU_TRI_MODE

static void lcd_panel_ili9341_init(disp_panel_para * info);
static void lcd_cpu_panel_fr(__u32 sel,__u32 w,__u32 h,__u32 x,__u32 y);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

extern s32 tcon0_cpu_set_tri_mode(u32 sel);

#if TCON_DEBUG
extern s32 tcon0_cpu_rd_24b_data(u32 sel, u32 index, u32 *data, u32 size);

void dump_tcon_register(void)
{
	volatile int *tcon_reg = (int *)(0xf1c0c000);
	int size = 0x180>>2;
	int val, i, sel;
	char output[128];
	char *p;

	while (size>0) {
		sel = size>8?8:size;
		size -= sel;

		p = output;
		p += sprintf(p, "0x%08x:", (int)tcon_reg);
		for (i=0; i<sel; i++) {
			val = *(tcon_reg);
			p += sprintf(p, " %08x", val);

			tcon_reg++;
		}
		printk("%s\n", output);
	}
}
#endif

static void LCD_cfg_panel_info(panel_extend_para * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 10);
#ifdef CPU_TRI_MODE
	LCD_OPEN_FUNC(sel, LCD_panel_init, 300);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 150);
#else
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 150);
	LCD_OPEN_FUNC(sel, LCD_panel_init, 300);
#endif
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 50);
#ifdef CPU_TRI_MODE
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	10);
#else
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	10);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
#endif
	LCD_CLOSE_FUNC(sel, LCD_power_off, 10);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power0
	sunxi_lcd_gpio_set_value(sel, 3, 0);//pwr_en, active low
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_gpio_set_value(sel, 3, 1);//pwr_en, active low
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power0
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight

}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);
}

static int bootup_flag = 0;
static void LCD_panel_init(u32 sel)
{
	disp_panel_para *info = disp_sys_malloc(sizeof(disp_panel_para));

    bsp_disp_get_panel_info(sel, info);
	lcd_panel_ili9341_init(info);

	if(LCD_CPU_AUTO_MODE == info->lcd_cpu_mode)
		sunxi_lcd_cpu_set_auto_mode(sel);

	disp_sys_free(info);
	return;
}

static void LCD_panel_exit(u32 sel)
{
	disp_panel_para *info = disp_sys_malloc(sizeof(disp_panel_para));

	pr_info("%s: enter sleep\n", __func__);
	sunxi_lcd_cpu_write_index(0,0x28);
	sunxi_lcd_cpu_write_index(0,0x10);
	sunxi_lcd_delay_ms(300);

	bsp_disp_get_panel_info(sel, info);
	disp_sys_free(info);
	return ;
}

static void lcd_dbi_wr_dcs(__u32 sel,__u8 cmd,__u8* para,__u32 para_num)
{
	__u8 index		= cmd;
	__u8* data_p	= para;
	__u16 i;
	sunxi_lcd_cpu_write_index(sel,index);
	for(i=0;i<para_num;i++)
	{
		sunxi_lcd_cpu_write_data(sel,*(data_p++));
	}
}

static void lcd_cpu_panel_fr(__u32 sel,__u32 w,__u32 h,__u32 x,__u32 y)
{
	__u8 para[4];
	__u32 para_num;
	para[0] = (x>>8)		& 0xff;
	para[1] = (x>>0) 		& 0xff;
	para[2] = ((x+w-1)>>8) 	& 0xff;
	para[3] = ((x+w-1)>>0) 	& 0xff;
	para_num = 4;
	lcd_dbi_wr_dcs(sel,DSI_DCS_SET_COLUMN_ADDRESS,para,para_num);

	para[0] = (y>>8)		& 0xff;
	para[1] = (y>>0) 		& 0xff;
	para[2] = ((y+h-1)>>8) 	& 0xff;
	para[3] = ((y+h-1)>>0) 	& 0xff;
	para_num = 4;
	lcd_dbi_wr_dcs(sel,DSI_DCS_SET_PAGE_ADDRESS,para,para_num);

	para_num = 0;
	lcd_dbi_wr_dcs(sel,DSI_DCS_WRITE_MEMORY_START,para,para_num);
}

static void lcd_panel_ili9341_init(disp_panel_para *info)
{
#if TCON_DEBUG
	u32 id[5];
	dump_tcon_register();
#endif

	printk("[debug_jaosn]:this is for test+++++++++++++\n");
	sunxi_lcd_gpio_set_value(0, 1, 0);

	if (!bootup_flag) {
		bootup_flag = 1;
		pr_info("ili9341 soft reset\n");
		sunxi_lcd_delay_ms(50);				/* wait for io stable */
		sunxi_lcd_cpu_write_index(0,0x01);	/* soft reset */
		sunxi_lcd_delay_ms(50);
		sunxi_lcd_cpu_write_index(0,0x01);	/* soft reset */
		sunxi_lcd_delay_ms(200);

	    sunxi_lcd_cpu_write_index(0,0x11);	/* sleep out */
		sunxi_lcd_delay_ms(120);
	} else {
		pr_info("ili9341 sleep out\n");
		sunxi_lcd_cpu_write_index(0,0x11);
		sunxi_lcd_delay_ms(300);
	}

    sunxi_lcd_cpu_write_index(0,0xCF);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_cpu_write_data(0,0xAA);
    sunxi_lcd_cpu_write_data(0,0xE0);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xED);
    sunxi_lcd_cpu_write_data(0,0x67);
    sunxi_lcd_cpu_write_data(0,0x03);
    sunxi_lcd_cpu_write_data(0,0x12);
    sunxi_lcd_cpu_write_data(0,0x81);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xE8);
    sunxi_lcd_cpu_write_data(0,0x85);
    sunxi_lcd_cpu_write_data(0,0x11);
    sunxi_lcd_cpu_write_data(0,0x78);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xCB);
    sunxi_lcd_cpu_write_data(0,0x39);
    sunxi_lcd_cpu_write_data(0,0x2C);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_cpu_write_data(0,0x34);
    sunxi_lcd_cpu_write_data(0,0x02);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xF7);
    sunxi_lcd_cpu_write_data(0,0x20);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xEA);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xC0);	//Power control
    sunxi_lcd_cpu_write_data(0,0x21);	//VRH[5:0]
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xC1);	//Power control
    sunxi_lcd_cpu_write_data(0,0x01);	//SAP[2:0];BT[3:0]
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xC5);	//VCM control
    sunxi_lcd_cpu_write_data(0,0x2a);
    sunxi_lcd_cpu_write_data(0,0x2f);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xC7);	//VCM control2
    sunxi_lcd_cpu_write_data(0,0xB8);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0x36);	// Memory Access Control
    if(info->lcd_x > info->lcd_y)
        sunxi_lcd_cpu_write_data(0,0x20);
    else
        sunxi_lcd_cpu_write_data(0,0x40);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0x3A);
    sunxi_lcd_cpu_write_data(0,0x55);
    sunxi_lcd_delay_ms(5);

    sunxi_lcd_cpu_write_index(0,0xF2);	// 3Gamma Function Disable
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0x26);	//Gamma curve selected
    sunxi_lcd_cpu_write_data(0,0x01);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xE0);	//Set Gamma
    sunxi_lcd_cpu_write_data(0,0x0F);
    sunxi_lcd_cpu_write_data(0,0x27);
    sunxi_lcd_cpu_write_data(0,0x23);
    sunxi_lcd_cpu_write_data(0,0x0B);
    sunxi_lcd_cpu_write_data(0,0x0F);
    sunxi_lcd_cpu_write_data(0,0x05);
    sunxi_lcd_cpu_write_data(0,0x50);
    sunxi_lcd_cpu_write_data(0,0x86);
    sunxi_lcd_cpu_write_data(0,0x41);
    sunxi_lcd_cpu_write_data(0,0x0E);
    sunxi_lcd_cpu_write_data(0,0x1B);
    sunxi_lcd_cpu_write_data(0,0x35);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_delay_ms(5);
    sunxi_lcd_cpu_write_index(0,0xE1);	//Set Gamma
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_cpu_write_data(0,0x1A);
    sunxi_lcd_cpu_write_data(0,0x1E);
    sunxi_lcd_cpu_write_data(0,0x03);
    sunxi_lcd_cpu_write_data(0,0x0F);
    sunxi_lcd_cpu_write_data(0,0x05);
    sunxi_lcd_cpu_write_data(0,0x2E);
    sunxi_lcd_cpu_write_data(0,0x25);
    sunxi_lcd_cpu_write_data(0,0x3E);
    sunxi_lcd_cpu_write_data(0,0x01);
    sunxi_lcd_cpu_write_data(0,0x04);
    sunxi_lcd_cpu_write_data(0,0x0A);
    sunxi_lcd_cpu_write_data(0,0x3F);
    sunxi_lcd_cpu_write_data(0,0x3F);
    sunxi_lcd_cpu_write_data(0,0x0F);
    sunxi_lcd_delay_ms(5);

#if defined(CPU_TRI_MODE)
	/* enable te, mode 0 */
    sunxi_lcd_cpu_write_index(0,0x35);
    sunxi_lcd_cpu_write_data(0,0x00);

	/* Tear_Scanline */
    sunxi_lcd_cpu_write_index(0,0x44);
    sunxi_lcd_cpu_write_data(0,0x00);
    sunxi_lcd_cpu_write_data(0,0x80);
#endif

    sunxi_lcd_cpu_write_index(0,0x11);	//Exit Sleep
    sunxi_lcd_delay_ms(50);
    sunxi_lcd_cpu_write_index(0,0x29);	//Display on
    sunxi_lcd_delay_ms(50);

#if TCON_DEBUG
	tcon0_cpu_rd_24b_data(0, 0x0e, id, 2);
	pr_err("ili9341 read signal mode:\n");
	pr_err("ili9341 0x%02x 0x%02x\n", id[1], id[0]);
#endif

    lcd_cpu_panel_fr(0, info->lcd_x, info->lcd_y, 0, 0);
}


//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t ili9341_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "ili9341",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};
