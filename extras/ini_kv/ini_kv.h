/*
 * PocketDC / Walnut-CGB — minimal key=value config helpers.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef INI_KV_H
#define INI_KV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool ini_kv_parse_bool(const char *value, bool *out);
bool ini_kv_get_int(const char *line, const char *key, int *out);
bool ini_kv_get_string(const char *line, const char *key, char *out, size_t out_len);

void ini_kv_fprint_int(FILE *f, const char *key, int value);
void ini_kv_fprint_bool(FILE *f, const char *key, bool value);
void ini_kv_fprint_string(FILE *f, const char *key, const char *value);

#endif /* INI_KV_H */
