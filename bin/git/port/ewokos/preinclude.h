#ifndef EWOKOS_GIT_PREINCLUDE_H
#define EWOKOS_GIT_PREINCLUDE_H

/* procinfo.h uses UNUSED as an enum constant; include it before Git's macro. */
#include <procinfo.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>

#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef NSIG
#define NSIG 32
#endif

#ifndef ELOOP
#define ELOOP 92
#endif

#ifndef SCNuMAX
#define SCNuMAX PRIuMAX
#endif

#define hashmap_put git_hashmap_put
#define hashmap_get git_hashmap_get
#define hashmap_remove git_hashmap_remove

#endif
