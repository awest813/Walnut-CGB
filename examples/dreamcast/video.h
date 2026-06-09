/*
 * Walnut-CGB Dreamcast frontend — video output.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_VIDEO_H
#define DC_VIDEO_H

#include "dc_priv.h"
#include "ui.h"

#define DC_UI_TEX_WIDTH  1024
#define DC_UI_TEX_HEIGHT 512

int dc_video_init(void);
void dc_video_shutdown(void);
void dc_video_present(const struct dc_priv *priv);
void dc_video_present_screen(const uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH]);
void dc_video_present_toast_overlay(void);

#endif /* DC_VIDEO_H */
