/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/msm_mdp.h>

#include <pixelflinger/pixelflinger.h>

#include "font_10x18.h"
#include "chinese.h"

#include "minui.h"

#if defined(RECOVERY_BGRA)
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_BGRA_8888
#define PIXEL_SIZE   4
#elif defined(RECOVERY_RGBX)
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGBX_8888
#define PIXEL_SIZE   4
#else
#define PIXEL_FORMAT GGL_PIXEL_FORMAT_RGB_565
#define PIXEL_SIZE   2
#endif

#define NUM_BUFFERS 2

#define BUILD_DEVICE_N909 1 //enable to use round_to_pagesize() 

#ifdef BUILD_DEVICE_N909 

inline size_t round_to_pagesize(size_t x) {
	return (x + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}
#endif


#define VI2 1 //just not need it now 

typedef struct {
    GGLSurface texture;
    unsigned cwidth;
    unsigned cheight;
    unsigned ascent;
} GRFont;

static GRFont *gr_font = 0;
static GGLContext *gr_context = 0;
static GGLSurface gr_font_texture;
static GGLSurface gr_framebuffer[NUM_BUFFERS];
static GGLSurface gr_mem_surface;
static unsigned gr_active_fb = 0;
static unsigned double_buffering = 0;
static int overscan_percent = OVERSCAN_PERCENT;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static int gr_fb_fd = -1;
static int gr_vt_fd = -1;

static struct fb_var_screeninfo vi;
static struct fb_fix_screeninfo fi;

static int get_framebuffer(GGLSurface *fb)
{
    int fd;
#ifdef VI2
    void *bits, *vi2;
#else
    void *bits;
#endif

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        perror("cannot open fb0");
        return -1;
    }
#ifdef VI2
    vi2 = malloc(sizeof(vi) + sizeof(__u32));
    if (ioctl(fd, FBIOGET_VSCREENINFO, vi2) < 0) {
#else
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
#endif
        perror("failed to get fb0 info");
        close(fd);
#ifdef VI2
	free(vi2);
#endif
        return -1;
    }
#ifdef VI2
    memcpy((void*) &vi, vi2, sizeof(vi));
    free(vi2);
#endif
    fprintf(stderr, "Pixel format: %dx%d @ %dbpp\n", vi.xres, vi.yres, vi.bits_per_pixel);

    vi.bits_per_pixel = PIXEL_SIZE * 8;
    if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_BGRA_8888) {
      vi.red.offset     = 8;
      vi.red.length     = 8;
      vi.green.offset   = 16;
      vi.green.length   = 8;
      vi.blue.offset    = 24;
      vi.blue.length    = 8;
      vi.transp.offset  = 0;
      vi.transp.length  = 8;
    } else if (PIXEL_FORMAT == GGL_PIXEL_FORMAT_RGBX_8888) {
      vi.red.offset     = 24;
      vi.red.length     = 8;
      vi.green.offset   = 16;
      vi.green.length   = 8;
      vi.blue.offset    = 8;
      vi.blue.length    = 8;
      vi.transp.offset  = 0;
      vi.transp.length  = 8;
    } else { /* RGB565*/
      vi.red.offset     = 11;
      vi.red.length     = 5;
      vi.green.offset   = 5;
      vi.green.length   = 6;
      vi.blue.offset    = 0;
      vi.blue.length    = 5;
      vi.transp.offset  = 0;
      vi.transp.length  = 0;
    }
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("failed to put fb0 info");
        close(fd);
        return -1;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return -1;
    }
#ifdef BUILD_DEVICE_N909
    size_t size = round_to_pagesize(vi.yres * fi.line_length) * NUM_BUFFERS;
    bits = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#else
    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        close(fd);
        return -1;
    }

    overscan_offset_x = vi.xres * overscan_percent / 100;
    overscan_offset_y = vi.yres * overscan_percent / 100;

    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length/PIXEL_SIZE;
    fb->data = bits;
    fb->format = PIXEL_FORMAT;
#ifdef BUILD_DEVICE_N909
    memset(fb->data, 0, round_to_pagesize(vi.yres * fi.line_length));
#else
    memset(fb->data, 0, vi.yres * fi.line_length);
#endif
    fb++;

    /* check if we can use double buffering */
    if (vi.yres * fi.line_length * 2 > fi.smem_len)
        return fd;

    double_buffering = 1;

    fb->version = sizeof(*fb);
    fb->width = vi.xres;
    fb->height = vi.yres;
    fb->stride = fi.line_length/PIXEL_SIZE;
#ifdef BUILD_DEVICE_N909
    fb->data = (void*) (((unsigned) bits) + round_to_pagesize(vi.yres * fi.line_length));
#else
    fb->data = (void*) (((unsigned) bits) + vi.yres * fi.line_length);
#endif

    fb->format = PIXEL_FORMAT;
#ifdef BUILD_DEVICE_N909
    memset(fb->data, 0, round_to_pagesize(vi.yres * fi.line_length));
#else
    memset(fb->data, 0, vi.yres * fi.line_length);
#endif

    return fd;
}

static void get_memory_surface(GGLSurface* ms) {
  ms->version = sizeof(*ms);
  ms->width = vi.xres;
  ms->height = vi.yres;
  ms->stride = fi.line_length/PIXEL_SIZE;
  ms->data = malloc(fi.line_length * vi.yres);
  ms->format = PIXEL_FORMAT;
}

static void set_active_framebuffer(unsigned n)
{
    if (n > 1 || !double_buffering) return;
    vi.yres_virtual = vi.yres * NUM_BUFFERS;
    vi.yoffset = n * vi.yres;
    vi.bits_per_pixel = PIXEL_SIZE * 8;
    if (ioctl(gr_fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("active fb swap failed");
    }
}

void gr_flip(void)
{
    GGLContext *gl = gr_context;

    /* swap front and back buffers */
    if (double_buffering)
        gr_active_fb = (gr_active_fb + 1) & 1;

    /* copy data from the in-memory surface to the buffer we're about
     * to make active. */
    memcpy(gr_framebuffer[gr_active_fb].data, gr_mem_surface.data,
           fi.line_length * vi.yres);

    /* inform the display driver */
    set_active_framebuffer(gr_active_fb);
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    GGLContext *gl = gr_context;
    GGLint color[4];
    color[0] = ((r << 8) | r) + 1;
    color[1] = ((g << 8) | g) + 1;
    color[2] = ((b << 8) | b) + 1;
    color[3] = ((a << 8) | a) + 1;
    gl->color4xv(gl, color);
}

int gr_measure(const char *s)
{
    return gr_font->cwidth * str_utf8_length(s);
}

void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}

static GGLSurface font_ftex;
static int font_bitmap_count;
static int font_char_per_bitmap = 128;
static void** font_data;
#include <sys/time.h>


int gr_text(int x, int y, const char *s)
{
    GGLContext *gl = gr_context;
    GRFont *gfont = gr_font;
    unsigned off, width, height, font_bitmap_width, n;


    x += overscan_offset_x;
    y += overscan_offset_y;

    y -= gfont->ascent;

    gl->bindTexture(gl, &gfont->texture);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

     while((off = *s)) {
        if(*((unsigned char*)(s)) < 0x20) {
            s++;
            continue;
        }
        width = gfont->cwidth;
        height = gfont->cheight;
        off = ch_utf8_to_custom(s);
        if(off >= 96)
            width *= 2;
        memcpy(&font_ftex, &gfont->texture, sizeof(font_ftex));
        font_bitmap_width = (font.width % (font.cwidth * font_char_per_bitmap));
        if(!font_bitmap_width)
            font_bitmap_width = font.cwidth * font_char_per_bitmap;
        font_ftex.width = font_bitmap_width;
        font_ftex.stride = font_bitmap_width;
        font_ftex.data = font_data[(off < 96) ? (off / font_char_per_bitmap) : ((96 + (off - 96) * 2) / font_char_per_bitmap)];
        gl->bindTexture(gl, &font_ftex);
        if(off >= 96)
            gl->texCoord2i(gl, ((96 + (off - 96) * 2) * font.cwidth) % (font_char_per_bitmap * font.cwidth) - x, 0 - y);
        else
            gl->texCoord2i(gl, (off % font_char_per_bitmap) * width - x, 0 - y);
        gl->recti(gl, x, y, x + width, y + height);
        x += width;
        n = ch_utf8_length(s);
        if(n <= 0)
            break;
        s += n;
    }

    return x;
}

void gr_texticon(int x, int y, gr_surface icon) {
    if (gr_context == NULL || icon == NULL) {
        return;
    }
    GGLContext* gl = gr_context;

    x += overscan_offset_x;
    y += overscan_offset_y;

    gl->bindTexture(gl, (GGLSurface*) icon);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);

    int w = gr_get_width(icon);
    int h = gr_get_height(icon);

    gl->texCoord2i(gl, -x, -y);
    gl->recti(gl, x, y, x+gr_get_width(icon), y+gr_get_height(icon));
}

void gr_fill(int x1, int y1, int x2, int y2)
{
    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    GGLContext *gl = gr_context;
    gl->disable(gl, GGL_TEXTURE_2D);
    gl->recti(gl, x1, y1, x2, y2);
}

void gr_blit(gr_surface source, int sx, int sy, int w, int h, int dx, int dy) {
    if (gr_context == NULL || source == NULL) {
        return;
    }
    GGLContext *gl = gr_context;

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    gl->bindTexture(gl, (GGLSurface*) source);
    gl->texEnvi(gl, GGL_TEXTURE_ENV, GGL_TEXTURE_ENV_MODE, GGL_REPLACE);
    gl->texGeni(gl, GGL_S, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->texGeni(gl, GGL_T, GGL_TEXTURE_GEN_MODE, GGL_ONE_TO_ONE);
    gl->enable(gl, GGL_TEXTURE_2D);
    gl->texCoord2i(gl, sx - dx, sy - dy);
    gl->recti(gl, dx, dy, dx + w, dy + h);
}

unsigned int gr_get_width(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->width;
}

unsigned int gr_get_height(gr_surface surface) {
    if (surface == NULL) {
        return 0;
    }
    return ((GGLSurface*) surface)->height;
}

static void gr_init_font(void)
{
     GGLSurface *ftex;
    unsigned char *bits, *rle;
    unsigned char *in, data;
    int i, d, n, bmp, pos;

    gr_font = calloc(sizeof(*gr_font), 1);
    ftex = &gr_font->texture;

    font_bitmap_count = font.width / font.cwidth / font_char_per_bitmap + 1;
    font_data = (void**)malloc(font_bitmap_count * sizeof(void*));
    for(n = 0; n < font_bitmap_count; n++)
    {
        font_data[n] = malloc(font_char_per_bitmap * font.cwidth * font.cheight);
        memset(font_data[n], 0, font_char_per_bitmap * font.cwidth * font.cheight);
    }
    d = 0;
    in = font.rundata;
    while((data = *in++))
    {
        n = data & 0x7f;
        for(i = 0; i < n; i++, d++)
        {
            bmp = d % font.width / (font.cwidth * font_char_per_bitmap);
            pos = d / font.width * (font.cwidth * font_char_per_bitmap) + (d % (font.cwidth * font_char_per_bitmap));
            ((unsigned char*)(font_data[bmp]))[pos] = (data & 0x80) ? 0xff : 0;
        }
    }
    bits = font_data[0];

    ftex->version = sizeof(*ftex);
    ftex->width = font.width;
    ftex->height = font.height;
    ftex->stride = font.width;
    ftex->data = (void*) bits;
    ftex->format = GGL_PIXEL_FORMAT_A_8;

    gr_font->cwidth = font.cwidth;
    gr_font->cheight = font.cheight;
    gr_font->ascent = font.cheight - 2;


}

int gr_init(void)
{
    gglInit(&gr_context);
    GGLContext *gl = gr_context;

    gr_init_font();
    gr_vt_fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (gr_vt_fd < 0) {
        // This is non-fatal; post-Cupcake kernels don't have tty0.
        perror("can't open /dev/tty0");
    } else if (ioctl(gr_vt_fd, KDSETMODE, (void*) KD_GRAPHICS)) {
        // However, if we do open tty0, we expect the ioctl to work.
        perror("failed KDSETMODE to KD_GRAPHICS on tty0");
        gr_exit();
        return -1;
    }

    gr_fb_fd = get_framebuffer(gr_framebuffer);
    if (gr_fb_fd < 0) {
        gr_exit();
        return -1;
    }

    get_memory_surface(&gr_mem_surface);

    fprintf(stderr, "framebuffer: fd %d (%d x %d)\n",
            gr_fb_fd, gr_framebuffer[0].width, gr_framebuffer[0].height);

        /* start with 0 as front (displayed) and 1 as back (drawing) */
    gr_active_fb = 0;
    set_active_framebuffer(0);
    gl->colorBuffer(gl, &gr_mem_surface);

    gl->activeTexture(gl, 0);
    gl->enable(gl, GGL_BLEND);
    gl->blendFunc(gl, GGL_SRC_ALPHA, GGL_ONE_MINUS_SRC_ALPHA);

    gr_fb_blank(true);
    gr_fb_blank(false);

    return 0;
}

void gr_exit(void)
{
    close(gr_fb_fd);
    gr_fb_fd = -1;

    free(gr_mem_surface.data);

    ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
    close(gr_vt_fd);
    gr_vt_fd = -1;
}

int gr_fb_width(void)
{
    return gr_framebuffer[0].width - 2*overscan_offset_x;
}

int gr_fb_height(void)
{
    return gr_framebuffer[0].height - 2*overscan_offset_y;
}

gr_pixel *gr_fb_data(void)
{
    return (unsigned short *) gr_mem_surface.data;
}

void gr_fb_blank(bool blank)
{
    int ret;

    ret = ioctl(gr_fb_fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    if (ret < 0)
        perror("ioctl(): blank");
}


