/*
 * Copyright (c) 2015, Google, Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <sys/types.h>
#include <stdint.h>

typedef struct uuid
{
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t time_hi_and_version;
	uint8_t clock_seq_and_node[8];
} uuid_t;

#define ZERO_UUID \
	{ 0x0, 0x0, 0x0, \
		{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} }

extern const struct uuid zero_uuid;


#define UUID_INITIAL_VALUE(uuid) \
    {                            \
        0, 0, 0, { 0 }           \
    }

/* UUID: {3c06d579-6cbc-476f-9363-503262d21b23} */
#define UUID_KERNEL_VALUE                                         \
    {                                                             \
        0x3c06d579, 0x6cbc, 0x476f,                               \
                {0x93, 0x63, 0x50, 0x32, 0x62, 0xd2, 0x1b, 0x23}, \
    }
