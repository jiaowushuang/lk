/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <assert.h>
#include <errno.h>
#include <compat/time.h>
#include <compat_syscalls.h>

#define NS_PER_SEC 1000000000

int compat_nanosleep(clockid_t clock_id, uint32_t flags, uint64_t sleep_time) {
    /* TODO validate clock_ids. */
    /* No flags, yet. */
    assert(flags == 0);
    return __sys_nanosleep(clock_id, flags, sleep_time, 0);
}

int nanodelay(clockid_t clock_id, uint32_t flags, uint64_t sleep_time) {
    int64_t target, time;
    int rc;

    /* TODO validate clock_ids. */
    /* No flags, yet. */
    assert(flags == 0);

    rc = gettime(clock_id, &time);
    if (rc)
        return rc;
    target = time + sleep_time;

    while (time < target) {
        rc = gettime(clock_id, &time);
        if (rc)
            return rc;
    }

    return 0;
}

int clock_nanosleep(clockid_t clock_id,
                    int flags,
                    const struct timespec* req,
                    struct timespec* rem) {
    /* Validate inputs. */
    if (!req) {
        errno = EFAULT;
        return -1;
    }
    if (req->tv_sec < 0) {
        errno = EINVAL;
        return -1;
    }
    if (req->tv_nsec >= NS_PER_SEC || req->tv_nsec < 0) {
        errno = EINVAL;
        return -1;
    }
    /* Convert timespec to nanoseconds. */
    uint64_t sleep_time;
    /*
     * Note: casting NS_PER_SEC to work around a Clang codegen bug.
     * See: b/145830721
     */
    if (__builtin_mul_overflow(req->tv_sec, (uint64_t)NS_PER_SEC,
                               &sleep_time)) {
        errno = EINVAL;
        return -1;
    }
    if (__builtin_add_overflow(sleep_time, req->tv_nsec, &sleep_time)) {
        errno = EINVAL;
        return -1;
    }
    /* Actually sleep. */
    int ret = compat_nanosleep(clock_id, flags, sleep_time);
    /* Handle the result. */
    if (rem) {
        /* Trusty should not wake up early, except for clock rounding. */
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    if (ret) {
        /* clock_id was invalid? */
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
    return clock_nanosleep(CLOCK_BOOTTIME, 0, req, rem);
}

int gettime(clockid_t clock_id, int64_t* time) {
    return __sys_gettime(clock_id, 0, time);
}

int __clock_gettime(clockid_t clock_id, struct timespec* ts) {
    if (ts == NULL) {
        errno = EFAULT;
        return -1;
    }
    int64_t time;
    int rc = gettime(clock_id, &time);
    if (rc) {
        /* &time is valid, so clock_id must have been invalid. */
        errno = EINVAL;
        return -1;
    }
    ts->tv_sec = (time_t)(time / NS_PER_SEC);
    ts->tv_nsec = (long)(time % NS_PER_SEC);
    return 0;
}

/*
 * Internally, Musl references __clock_gettime, and then provides an alias for
 * external users. Since we're providing the function, we need to provide the
 * internal name for Musl and the external name for the user.
 */
weak_alias(__clock_gettime, clock_gettime);
