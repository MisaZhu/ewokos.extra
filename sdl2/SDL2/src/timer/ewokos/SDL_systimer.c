/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if defined(SDL_TIMER_EWOKOS)

#include "SDL_timer.h"
#include "ewoksys/proc.h"
#include "ewoksys/kernel_tic.h"

static SDL_bool ticks_started = SDL_FALSE;
static uint64_t _start = 0;

void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }

    _start = kernel_tic_ms(0);
    ticks_started = SDL_TRUE;
}

void
SDL_TicksQuit(void)
{
    ticks_started = SDL_FALSE;
}

Uint32
SDL_GetTicks(void)
{
    if (!ticks_started || _start == 0) {
        SDL_TicksInit();
    }

    uint64_t now = kernel_tic_ms(0);
    return now - _start;
}

Uint64
SDL_GetPerformanceCounter(void)
{
    return SDL_GetTicks();
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 1000;
}

void
SDL_Delay(Uint32 ms)
{
    proc_usleep(ms*1000);
}

#endif /* SDL_TIMER_DUMMY || SDL_TIMERS_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
