/*
 * Copyright (c) 2013 Google, Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/version.h>

#include <kern/debug.h>
#include <stdio.h>
#include <kern/init.h>
#include <kern/console_cmd.h>

/* generated for us */
#include <buildid.h>

/* ARCH, PLATFORM, TARGET, PROJECT should be defined by the build system */

/* BUILDID is optional, and may be defined anywhere */
#ifndef BUILDID
#define BUILDID ""
#endif

const kern_version_t kern_version = {
    .struct_version = VERSION_STRUCT_VERSION,
    .arch = ARCH,
    .platform = PLATFORM,
    .target = TARGET,
    .project = PROJECT,
    .buildid = BUILDID
};

void print_version(void) {
    printf("version:\n");
    printf("\tarch:     %s\n", kern_version.arch);
    printf("\tplatform: %s\n", kern_version.platform);
    printf("\ttarget:   %s\n", kern_version.target);
    printf("\tproject:  %s\n", kern_version.project);
    printf("\tbuildid:  %s\n", kern_version.buildid);
}

static int cmd_version(int argc, const console_cmd_args *argv) {
    print_version();
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("version", "print version", &cmd_version)
STATIC_COMMAND_END(version);

#if SYS_DEBUGLEVEL > 0
// print the version string if any level of debug is set
INIT_HOOK(version, (void *)&print_version, INIT_LEVEL_HEAP - 1);
#endif
