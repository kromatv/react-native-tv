// Every definition here stands in for one a newer glibc exports. Hidden, so
// the TV's own glibc, which may define the same name, never competes with it.

#pragma once

#define RN_WEBOS_COMPAT __attribute__((visibility("hidden")))
