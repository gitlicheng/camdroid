/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <drv_display_sun7i.h>

#include "log.h"

#ifdef ANDROID
#include <cutils/memory.h>
#else
void android_memset16(void *_ptr, unsigned short val, unsigned count)
{
    unsigned short *ptr = _ptr;
    count >>= 1;
    while(count--)
        *ptr++ = val;
}
#endif

struct FB {
    unsigned short *bits;
    unsigned size;
    int fd;
    struct fb_fix_screeninfo fi;
    struct fb_var_screeninfo vi;
};

#define fb_width(fb) ((fb)->vi.xres)
#define fb_height(fb) ((fb)->vi.yres)
#define fb_size(fb) ((fb)->vi.xres * (fb)->vi.yres * 4)

static int fb_open(struct FB *fb)
{
    fb->fd = open("/dev/graphics/fb0", O_RDWR);
    if (fb->fd < 0)
        return -1;

    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fi) < 0)
        goto fail;
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vi) < 0)
        goto fail;

    fb->bits = mmap(0, fb_size(fb), PROT_READ | PROT_WRITE, 
                    MAP_SHARED, fb->fd, 0);
    if (fb->bits == MAP_FAILED)
        goto fail;

    return 0;

fail:
    close(fb->fd);
    return -1;
}

static void fb_close(struct FB *fb)
{
    munmap(fb->bits, fb_size(fb));
    close(fb->fd);
}

/* there's got to be a more portable way to do this ... */
static void fb_update(struct FB *fb)
{
    fb->vi.yoffset = 1;
    ioctl(fb->fd, FBIOPUT_VSCREENINFO, &fb->vi);
    fb->vi.yoffset = 0;
    ioctl(fb->fd, FBIOPUT_VSCREENINFO, &fb->vi);
}

static int vt_set_mode(int graphics)
{
    int fd, r;
    fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (fd < 0)
        return -1;
    r = ioctl(fd, KDSETMODE, (void*) (graphics ? KD_GRAPHICS : KD_TEXT));
    close(fd);
    return r;
}

/* 565RLE image format: [count(2 bytes), rle(2 bytes)] */

int load_565rle_image(char *fn)
{
    struct FB fb;
    struct stat s;
    unsigned short *data, *bits, *ptr;
    unsigned count, max;
    int fd;

    if (vt_set_mode(1)) 
        return -1;

    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        ERROR("cannot open '%s'\n", fn);
        goto fail_restore_text;
    }

    if (fstat(fd, &s) < 0) {
        goto fail_close_file;
    }

    data = mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
        goto fail_close_file;

    if (fb_open(&fb))
        goto fail_unmap_data;

    max = fb_width(&fb) * fb_height(&fb);
    ptr = data;
    count = s.st_size;
    bits = fb.bits;
    while (count > 3) {
        unsigned n = ptr[0];
        if (n > max)
            break;
        android_memset16(bits, ptr[1], n << 1);
        bits += n;
        max -= n;
        ptr += 2;
        count -= 4;
    }

    munmap(data, s.st_size);
    fb_update(&fb);
    fb_close(&fb);
    close(fd);
    unlink(fn);
    return 0;

fail_unmap_data:
    munmap(data, s.st_size);    
fail_close_file:
    close(fd);
fail_restore_text:
    vt_set_mode(0);
    return -1;
}

int load_argb8888_image(char *fn)
{
    struct FB fb;
    struct stat s;
    unsigned long *data, *bits, *ptr;
    unsigned long *lineptr;
    unsigned long width;
    unsigned long height;
    unsigned long countw = 0;
    unsigned long counth = 0;
    unsigned long *linebits;
    unsigned long fbsize;
    int fd;
    int disp;
    unsigned long image_width;
    unsigned long image_height;
    unsigned int layer_hdl;
    __disp_layer_info_t layer_para;
    unsigned long arg[4];

    if (vt_set_mode(1)) 
        return -1;

    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        ERROR("cannot open '%s'\n", fn);
        goto fail_restore_text;
    }

    if (fstat(fd, &s) < 0) 
    {
        ERROR("fstat failed!\n");
        goto fail_close_file;
    }

    data = mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        ERROR("MMAP failed!\n");
        goto fail_close_file;
    }

    if (fb_open(&fb))
    {
        ERROR("FB_OPEN failed!\n");
        goto fail_unmap_data;
    }

    disp = open("/dev/disp", O_RDWR, 0);
    if (disp < 0) 
    {
        ERROR("Error opening display driver");
        goto fail_unmap_data;
    } 
    ioctl(fb.fd , FBIOGET_LAYER_HDL_0, &layer_hdl);

    arg[0] = 0;
    arg[1] = layer_hdl;
    arg[2] = (unsigned long)&layer_para;
    ioctl(disp,DISP_CMD_LAYER_GET_PARA,(unsigned long)arg);
    
    image_width = layer_para.src_win.width;
    image_height = layer_para.src_win.height;
    width       = fb_width(&fb);
    height      = fb_height(&fb);

    fbsize      = image_width * image_height * 4;
    ERROR("width = %d\n",image_width);
	ERROR("height = %d\n",image_height);
	ERROR("s.st_size = %d\n",s.st_size);
    
    if(fbsize != s.st_size)
    {
        ERROR("logo match failed!fbsize = %d\n",fbsize);

        munmap(data, s.st_size);
        fb_update(&fb);
        fb_close(&fb);
        close(fd);

        return -1;
    }
    
    counth      = image_height;
    linebits    = fb.bits;
    lineptr     = data;

    while (counth > 0) 
    {
        bits    = linebits;
        ptr     = lineptr;
        countw  = image_width;
        while(countw > 0)
        {
            *bits = *ptr;
            ptr++;
            bits++;
            countw--;
        }
        linebits    += width;
        lineptr     += image_width;
        counth--;
    }

    munmap(data, s.st_size);
    fb_update(&fb);
    fb_close(&fb);
    close(fd);
    //unlink(fn);
    return 0;

fail_unmap_data:
    munmap(data, s.st_size);    
fail_close_file:
    close(fd);
fail_restore_text:
    vt_set_mode(0);
    return -1;
}

