/*
 * Walnut-CGB Dreamcast frontend — video output.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_VIDEO_H
#define DC_VIDEO_H

#include "dc_priv.h"

int dc_video_init(void);
void dc_video_shutdown(void);
void dc_video_present(const struct dc_priv *priv);

#endif /* DC_VIDEO_H */
