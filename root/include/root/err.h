/*
 * Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include <errno.h>
#include <kern/compiler.h>
#include <uapi/err.h>

__BEGIN_CDECLS

int sys_err_to_errno(int sys_err);
int errno_to_sys_err(int err);

static inline void * ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR( const void *ptr) //__force
{
	return (long) ptr;
}
__END_CDECLS