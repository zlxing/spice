/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2009-2015 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include "red-common.h"
#include "spice/macros.h"
#include "common/ring.h"
#include "common/mem.h"
#include "basic_event_loop.h"

int debug = 0;

#define DPRINTF(x, format, ...) { \
    if (x <= debug) { \
        printf("%s: " format "\n" , __FUNCTION__, ## __VA_ARGS__); \
    } \
}

static GMainContext *main_context = NULL;

GMainContext *basic_event_loop_get_context(void)
{
    return main_context;
}

static inline GMainContext *event_loop_context_from_iface(const SpiceCoreInterfaceInternal *iface)
{
    return main_context;
}

static void event_loop_channel_event(int event, SpiceChannelEventInfo *info)
{
    DPRINTF(0, "channel event con, type, id, event: %d, %d, %d, %d",
            info->connection_id, info->type, info->id, event);
}

#include "../event-loop.tmpl.c"

void basic_event_loop_mainloop(void)
{
    GMainLoop *loop = g_main_loop_new(main_context, FALSE);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}

static void ignore_sigpipe(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigfillset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);
}

static SpiceTimer* base_timer_add(SpiceTimerFunc func, void *opaque)
{
    return event_loop_core.timer_add(NULL, func, opaque);
}

static SpiceWatch *base_watch_add(int fd, int event_mask, SpiceWatchFunc func, void *opaque)
{
    return event_loop_core.watch_add(NULL, fd, event_mask, func, opaque);
}

static SpiceCoreInterface core = {
    .base = {
        .major_version = SPICE_INTERFACE_CORE_MAJOR,
        .minor_version = SPICE_INTERFACE_CORE_MINOR,
    },
    .timer_add = base_timer_add,
    .watch_add = base_watch_add,
    .channel_event = event_loop_channel_event,
};

SpiceCoreInterface *basic_event_loop_init(void)
{
    ignore_sigpipe();
    spice_assert(main_context == NULL);
    main_context = g_main_context_new();
    core.timer_start = event_loop_core.timer_start;
    core.timer_cancel = event_loop_core.timer_cancel;
    core.timer_remove = event_loop_core.timer_remove;
    core.watch_update_mask = event_loop_core.watch_update_mask;
    core.watch_remove = event_loop_core.watch_remove;
    return &core;
}
