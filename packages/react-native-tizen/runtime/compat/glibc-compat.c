// What GCC 14's static libstdc++ and libgcc call and the glibc 2.30 of a Tizen
// 6.0 to 9.0 TV does not export. Hidden, so a newer glibc's own definition never
// competes.

#define _GNU_SOURCE
#include <errno.h>
#include <link.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/types.h>

#define RN_TIZEN_HIDDEN __attribute__((visibility("hidden")))

// Zero tells libstdc++ the process may have threads, so it never skips an atomic.
RN_TIZEN_HIDDEN char __libc_single_threaded = 0;

RN_TIZEN_HIDDEN unsigned long __isoc23_strtoul(const char *text, char **end, int base) {
  return strtoul(text, end, base);
}

RN_TIZEN_HIDDEN uint32_t arc4random(void) {
  uint32_t value;
  for (;;) {
    ssize_t received = getrandom(&value, sizeof value, 0);
    if (received == (ssize_t)sizeof value) return value;
    if (received < 0 && errno != EINTR) abort();
  }
}

#if (defined(__aarch64__) || defined(__x86_64__)) && !__GLIBC_PREREQ(2, 35)
// GCC 14's DWARF unwinder needs glibc 2.35's _dl_find_object, with no fallback.
// glibc's struct for these CPUs has no eh_dbase or eh_count; ARM's EHABI never calls it.
struct dl_find_object {
  unsigned long long dlfo_flags;
  void *dlfo_map_start;
  void *dlfo_map_end;
  struct link_map *dlfo_link_map;
  void *dlfo_eh_frame;
  unsigned long long dlfo_reserved[7];
};

struct object_search {
  uintptr_t address;
  struct dl_find_object *result;
};

static int find_object(struct dl_phdr_info *info, size_t size, void *data) {
  (void)size;
  struct object_search *search = data;
  uintptr_t start = UINTPTR_MAX;
  uintptr_t end = 0;
  void *eh_frame = NULL;
  for (ElfW(Half) index = 0; index < info->dlpi_phnum; index++) {
    const ElfW(Phdr) *header = &info->dlpi_phdr[index];
    const uintptr_t segment = info->dlpi_addr + header->p_vaddr;
    if (header->p_type == PT_LOAD) {
      if (segment < start) start = segment;
      if (segment + header->p_memsz > end) end = segment + header->p_memsz;
    } else if (header->p_type == PT_GNU_EH_FRAME) {
      eh_frame = (void *)segment;
    }
  }
  if (search->address < start || search->address >= end) return 0;
  memset(search->result, 0, sizeof *search->result);
  search->result->dlfo_map_start = (void *)start;
  search->result->dlfo_map_end = (void *)end;
  search->result->dlfo_eh_frame = eh_frame;
  return 1;
}

RN_TIZEN_HIDDEN int _dl_find_object(void *address, struct dl_find_object *result) {
  struct object_search search = {.address = (uintptr_t)address, .result = result};
  return dl_iterate_phdr(find_object, &search) != 0 ? 0 : -1;
}
#endif
