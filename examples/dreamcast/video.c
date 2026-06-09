/*
 * Walnut-CGB Dreamcast frontend — PVR textured quad presentation.
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

static pvr_ptr_t tex;
static pvr_ptr_t ui_tex;
static pvr_poly_cxt_t poly_cxt;
static pvr_poly_hdr_t poly_hdr;
static pvr_poly_cxt_t ui_poly_cxt;
static pvr_poly_hdr_t ui_poly_hdr;
static uint16_t upload_buf[DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT];
static uint16_t ui_upload_buf[DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT];

static void dc_video_draw_quad_hdr(const pvr_poly_hdr_t *hdr,
				   int x, int y, int w, int h,
				   float u1, float v1)
{
	pvr_vertex_t vert;

	pvr_prim(hdr, sizeof(*hdr));

	vert.flags = PVR_CMD_VERTEX;
	vert.x = (float)x;
	vert.y = (float)y;
	vert.z = 1.0f;
	vert.u = 0.0f;
	vert.v = 0.0f;
	vert.argb = 0xFFFFFFFF;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = (float)(x + w);
	vert.u = u1;
	pvr_prim(&vert, sizeof(vert));

	vert.x = (float)x;
	vert.y = (float)(y + h);
	vert.u = 0.0f;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = (float)(x + w);
	vert.u = u1;
	vert.v = v1;
	pvr_prim(&vert, sizeof(vert));
}

static void dc_video_draw_game_quad(int x, int y, int w, int h)
{
	const float u1 = (float)LCD_WIDTH / (float)DC_FB_TEX_WIDTH;
	const float v1 = (float)LCD_HEIGHT / (float)DC_FB_TEX_HEIGHT;

	dc_video_draw_quad_hdr(&poly_hdr, x, y, w, h, u1, v1);
}

int dc_video_init(void)
{
	if (pvr_init_defaults() < 0)
		return -1;

	tex = pvr_mem_malloc(DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT * 2);
	if (!tex)
		return -1;

	memset(upload_buf, 0, sizeof(upload_buf));
	pvr_txr_load(upload_buf, tex, DC_FB_TEX_WIDTH * DC_FB_TEX_HEIGHT);

	pvr_poly_cxt_txr(&poly_cxt, PVR_LIST_TR_POLY,
			 PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
			 DC_FB_TEX_WIDTH, DC_FB_TEX_HEIGHT, tex, PVR_FILTER_NONE);
	pvr_poly_compile(&poly_hdr, &poly_cxt);

	ui_tex = pvr_mem_malloc(DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT * 2);
	if (!ui_tex)
		return -1;

	memset(ui_upload_buf, 0, sizeof(ui_upload_buf));
	pvr_txr_load(ui_upload_buf, ui_tex, DC_UI_TEX_WIDTH * DC_UI_TEX_HEIGHT);

	pvr_poly_cxt_txr(&ui_poly_cxt, PVR_LIST_TR_POLY,
			 PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
			 DC_UI_TEX_WIDTH, DC_UI_TEX_HEIGHT, ui_tex, PVR_FILTER_NONE);
	pvr_poly_compile(&ui_poly_hdr, &ui_poly_cxt);

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
	pvr_list_begin(PVR_LIST_TR_POLY);

	{
		const int scale = DC_DISPLAY_SCALE;
		const int draw_w = LCD_WIDTH * scale;
		const int draw_h = LCD_HEIGHT * scale;
		const int draw_x = (640 - draw_w) / 2;
		const int draw_y = (480 - draw_h) / 2;

		dc_video_draw_game_quad(draw_x, draw_y, draw_w, draw_h);
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
	pvr_list_begin(PVR_LIST_TR_POLY);
	dc_video_draw_quad_hdr(&ui_poly_hdr, 0, 0, DC_SCREEN_WIDTH, DC_SCREEN_HEIGHT,
			       (float)DC_SCREEN_WIDTH / (float)DC_UI_TEX_WIDTH,
			       (float)DC_SCREEN_HEIGHT / (float)DC_UI_TEX_HEIGHT);
	pvr_list_finish();
	pvr_scene_finish();
}
