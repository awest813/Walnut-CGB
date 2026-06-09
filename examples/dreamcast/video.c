/*
 * Walnut-CGB Dreamcast frontend — PVR hardware sprite presentation.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdint.h>
#include <string.h>

#include <kos.h>
#include <dc/pvr.h>
#include <dc/video.h>

#include "ui.h"
#include "video.h"

#define DC_PVR_PACK_UV(u, v) \
	((uint32_t)(((int)((u) * 1024.0f)) & 0x3ff) | \
	 ((((int)((v) * 1024.0f)) & 0x3ff) << 10))

static pvr_ptr_t tex;
static pvr_ptr_t ui_tex;
static pvr_sprite_cxt_t game_sprite_cxt;
static pvr_sprite_hdr_t game_sprite_hdr;
static pvr_sprite_cxt_t ui_sprite_cxt;
static pvr_sprite_hdr_t ui_sprite_hdr;
static uint16_t upload_buf[DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT];
static uint16_t ui_upload_buf[DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT];

static void dc_video_init_sprite_cxt(pvr_sprite_cxt_t *cxt, pvr_sprite_hdr_t *hdr,
				     pvr_list_t list, int tex_fmt,
				     int tex_w, int tex_h, pvr_ptr_t tex_addr)
{
	pvr_sprite_cxt_txr(cxt, list, tex_fmt, tex_w, tex_h, tex_addr,
			   PVR_FILTER_NONE);
	cxt->gen.culling = PVR_CULLING_NONE;
	cxt->depth.write = false;
	pvr_sprite_compile(hdr, cxt);
	hdr->argb = 0xFFFFFFFF;
	hdr->oargb = 0;
}

static void dc_video_draw_sprite(const pvr_sprite_hdr_t *hdr,
				 int x, int y, int w, int h,
				 float u1, float v1)
{
	pvr_sprite_txr_t vert;

	vert.flags = PVR_CMD_VERTEX;
	vert.ax = (float)x;
	vert.ay = (float)y;
	vert.az = 1.0f;
	vert.bx = (float)(x + w);
	vert.by = (float)y;
	vert.bz = 1.0f;
	vert.cx = (float)x;
	vert.cy = (float)(y + h);
	vert.cz = 1.0f;
	vert.dx = (float)(x + w);
	vert.dy = (float)(y + h);
	vert.dummy = 0;
	vert.auv = DC_PVR_PACK_UV(0.0f, 0.0f);
	vert.buv = DC_PVR_PACK_UV(u1, 0.0f);
	vert.cuv = DC_PVR_PACK_UV(0.0f, v1);

	pvr_prim(hdr, sizeof(*hdr));
	pvr_prim(&vert, sizeof(vert));
}

static void dc_video_draw_game_sprite(int x, int y, int w, int h)
{
	const float u1 = (float)LCD_WIDTH / (float)DC_FB_TEX_WIDTH;
	const float v1 = (float)LCD_HEIGHT / (float)DC_FB_TEX_HEIGHT;

	dc_video_draw_sprite(&game_sprite_hdr, x, y, w, h, u1, v1);
}

int dc_video_init(void)
{
	const int tex_fmt = PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED;

	if (pvr_init_defaults() < 0)
		return -1;

	tex = pvr_mem_malloc(DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT * 2);
	if (!tex)
		return -1;

	memset(upload_buf, 0, sizeof(upload_buf));
	pvr_txr_load(upload_buf, tex, DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT);
	dc_video_init_sprite_cxt(&game_sprite_cxt, &game_sprite_hdr, PVR_LIST_OP_POLY,
				 tex_fmt, DC_FB_TEX_WIDTH, DC_FB_TEX_HEIGHT, tex);

	ui_tex = pvr_mem_malloc(DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT * 2);
	if (!ui_tex)
		return -1;

	memset(ui_upload_buf, 0, sizeof(ui_upload_buf));
	pvr_txr_load(ui_upload_buf, ui_tex, DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT);
	dc_video_init_sprite_cxt(&ui_sprite_cxt, &ui_sprite_hdr, PVR_LIST_OP_POLY,
				 tex_fmt, DC_UI_TEX_WIDTH, DC_UI_TEX_HEIGHT, ui_tex);

	return 0;
}

void dc_video_shutdown(void)
{
	if (tex) {
		pvr_mem_free(tex);
		tex = NULL;
	}
	if (ui_tex) {
		pvr_mem_free(ui_tex);
		ui_tex = NULL;
	}
	pvr_shutdown();
}

void dc_video_present(const struct dc_priv *priv)
{
	unsigned int y;

	for (y = 0; y < LCD_HEIGHT; y++)
		memcpy(&upload_buf[y * DC_FB_TEX_WIDTH], priv->fb[y],
		       LCD_WIDTH * sizeof(uint16_t));

	pvr_txr_load(upload_buf, tex, DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT);

	pvr_wait_ready();
	pvr_scene_begin();
	pvr_list_begin(PVR_LIST_OP_POLY);

	{
		const int scale = DC_DISPLAY_SCALE;
		const int draw_w = LCD_WIDTH * scale;
		const int draw_h = LCD_HEIGHT * scale;
		const int draw_x = (640 - draw_w) / 2;
		const int draw_y = (480 - draw_h) / 2;

		dc_video_draw_game_sprite(draw_x, draw_y, draw_w, draw_h);
	}

	pvr_list_finish();
	pvr_scene_finish();
}

void dc_video_present_screen(const uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH])
{
	unsigned int y;

	for (y = 0; y < DC_SCREEN_HEIGHT; y++)
		memcpy(&ui_upload_buf[y * DC_UI_TEX_WIDTH], screen[y],
		       DC_SCREEN_WIDTH * sizeof(uint16_t));

	pvr_txr_load(ui_upload_buf, ui_tex, DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT);

	pvr_wait_ready();
	pvr_scene_begin();
	pvr_list_begin(PVR_LIST_OP_POLY);
	dc_video_draw_sprite(&ui_sprite_hdr, 0, 0, DC_SCREEN_WIDTH, DC_SCREEN_HEIGHT,
			     (float)DC_SCREEN_WIDTH / (float)DC_UI_TEX_WIDTH,
			     (float)DC_SCREEN_HEIGHT / (float)DC_UI_TEX_HEIGHT);
	pvr_list_finish();
	pvr_scene_finish();
}
