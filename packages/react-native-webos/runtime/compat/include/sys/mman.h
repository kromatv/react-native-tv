// <sys/mman.h> with the madvise advice glibc 2.12.2's <bits/mman.h> stops
// short of. The values are those of the SDK's own kernel header,
// asm-generic/mman-common.h.

#pragma once

#include_next <sys/mman.h>

#ifndef MADV_HUGEPAGE
#define MADV_HUGEPAGE 14
#endif
