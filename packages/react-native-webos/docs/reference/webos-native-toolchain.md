# webOS native toolchain

Which compilers and sysroots exist for native code on LG webOS TV, what the
public sysroot contains, the ABI a TV runs, how far back one binary can reach,
and whether clang and lld can build against it from a Mac. Read it before
touching `runtime/cmake/toolchain/`, `runtime/compat/`, `src/sysroot/` or the
libraries the runtime links.

Tags: **D** documented (a page, README or source file opened on 2026-11-11 and
linked), **Xw** inspected (toolchain release webosbrew/native-toolchain
`webos-d7ed7ee.6`, darwin-arm64 asset), **Xo** inspected (toolchain release
openlgtv/buildroot-nc4 `2026.08-webos`, aarch64 asset), **I** inferred (from
what the row says), **B** built (the runtime's TV build of section 9: compiled
and linked against Xo on macOS 27 arm64 with Homebrew clang and lld 23.1.4 on
2026-11-11, then read with `scripts/check-tv-binary.ts` and `llvm-readelf`).

"Inspected" means read with `llvm-readelf`, `llvm-nm` and `llvm-strings` from
Homebrew LLVM 23.1.4, and, where a row says "link test", compiled and linked on
macOS arm64 with Homebrew clang 23.1.4 and `ld.lld` 23.1.4. Nothing from either
archive was executed and no produced binary was run, the runtime of section 9
included. **Nothing in this file is verified on a TV.**

Rows citing [fw] come from webosbrew's firmware symbol data: 14 firmware
images covering webOS 1.2.0 to 11.2.0, each from one SoC. They describe those
images, not every TV of that release.

## 1. Toolchains and sysroots that exist

| Fact | Tag | Source |
|---|---|---|
| webosbrew's `native-toolchain` is "Ports, downloads and documents for unofficial webOS NDK", the NDK being openlgtv's `buildroot-nc4`. Install is: download, extract to a path without spaces, run `relocate-sdk.sh`. CMake use is `-DCMAKE_TOOLCHAIN_FILE=<sdk>/share/buildroot/toolchainfile.cmake`. | D | [nt] |
| Its releases: `webos-d7ed7ee.6` (2024-03-04, newest) with `arm-webos-linux-gnueabi_sdk-buildroot_darwin-arm64.tar.bz2` (145,888,279 bytes), `..._darwin-x86_64.tar.bz2` and `..._linux-aarch64.tar.bz2`, at `https://github.com/webosbrew/native-toolchain/releases/download/webos-d7ed7ee.6/<asset>`; `webos-d7ed7ee.5` (prerelease, no assets); `webos-d7ed7ee.3` (darwin-x86_64, linux-aarch64); `webos-71307ba.1` (darwin-x86_64). No Linux x86_64 asset: the README links both Linux hosts to `openlgtv/buildroot-nc4/releases/latest`. | D | [nt-rel], [nt] |
| The port repackages buildroot-nc4 release `webos-d7ed7ee` (2023-09-19): it downloads that SDK, keeps `arm-webos-linux-gnueabi/sysroot`, `share/buildroot`, `environment-setup` and `relocate-sdk.sh`, drops `usr/share/{doc,i18n,locale,man,terminfo,zoneinfo}`, and builds binutils 2.38 and GCC 12.2.0 for the host. | D | [nt-cmake], [nt-sysroot], [nt-gcc] |
| buildroot-nc4's newest release is `2026.08-webos` (2026-09-18, commit `322ff04e`): `arm-webos-linux-gnueabi_sdk-buildroot-aarch64.tar.gz` (358,988,198 bytes) and `...-x86_64.tar.gz` (365,097,814 bytes), Linux hosts only, at `https://github.com/openlgtv/buildroot-nc4/releases/download/2026.08-webos/<asset>`. The same release has `arm-lgtv-linux-gnueabi_sdk-buildroot-*` for a second target, not inspected. | D | [br-rel] |
| Layout of both: compilers at `bin/arm-webos-linux-gnueabi-{gcc,g++}` (links to `toolchain-wrapper`, real binaries `*.br_real`), GCC runtime in `lib/gcc/arm-webos-linux-gnueabi/<version>/` (`crtbegin*.o`, `crtend*.o`, `libgcc.a`, `libgcc_eh.a`), C++ headers in `arm-webos-linux-gnueabi/include/c++/<version>/`, `libstdc++.a`, `libsupc++.a`, `libstdc++fs.a` and `libatomic.a` in `arm-webos-linux-gnueabi/lib/`, the sysroot at `arm-webos-linux-gnueabi/sysroot`. Target triple `arm-webos-linux-gnueabi`. | Xw, Xo | archives |
| `relocate-sdk.sh` reads the build machine's path from `share/buildroot/sdk-location` and rewrites it with `sed` in every text file that holds it, pkg-config files included (`wayland-webos-client.pc` carries `-L<build path>/sysroot/usr/lib`). | Xw, Xo | `relocate-sdk.sh` |
| The Xo archive holds one top-level directory, `arm-webos-linux-gnueabi_sdk-buildroot/`, and 46,802 entries with ASCII names: 41,563 files, 3,122 directories, 660 symbolic links and 1,457 hard links. No hard link in the four directories a build reads (below) points outside them. | Xo | `tar -tvzf`, 2026-11-11 |
| The Xw archive also holds copies named with a `''` suffix (`lib/libatomic.la''`, `share/buildroot/sdk-location''`), left by the BSD `sed -i ''` its relocate patch adds. | Xw, I | archive listing; [nt-patch] |
| `toolchainfile.cmake` is identical in both: `CMAKE_SYSTEM_NAME Buildroot`, `CMAKE_SYSTEM_PROCESSOR armv7l`, `CMAKE_SYSROOT` and `CMAKE_FIND_ROOT_PATH` at the sysroot, find modes `NEVER` for programs and `ONLY` for libraries, includes and packages, C and C++ flags `-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64`, Release `-O2 -DNDEBUG`, default build type Release, empty linker flags. It passes no `-march`: the ABI comes from how GCC was configured. | Xw, Xo | `share/buildroot/toolchainfile.cmake` |
| LG's developer site describes two things a developer builds: web apps and JavaScript services. Its CLI guide's commands create, install and launch "web apps". | D | [lg-discover], [lg-cli] |
| LG published "Flutter for webOS TV" in 2026. It "supports webOS TV 26 Re:New, and later", on an Ubuntu host only, and needs LG's "webOS NDK": `webos-ndk-flutter-starfish-x86_64-ca9v1-11.2.0.sh` (340,891,972 bytes), installed to `/usr/local/starfish-sdk-x86_64`, with `environment-setup-ca9v1-webosmllib32-linux-gnueabi`. | D | [lg-flutter-news], [ft-start], [lg-ndk] |
| That installer is gated by an LG "Software Development Kit License Agreement" whose section 2.5 forbids copying, redistribution and reverse engineering. It was not unpacked for this file. | D | the installer's own shell header, "webOS External NDK License Agreement (install-time gate)" |
| flutter-webos compiles the app's native runner with the host's clang: `CC=clang`, `CXX=clang++`, `-DCMAKE_SYSROOT=$SDKTARGETSYSROOT`, `-DCMAKE_C_COMPILER_TARGET` from `CROSS_COMPILE` (default `arm-starfishmllib32-linux-gnueabi`). | D | [ft-build], [ft-sdk] |
| The webOS OSE NDK is built by the user (`bitbake -c populate_sdk webos-image`) and yields `webos-sdk-x86_64-cortexa72-toolchain-2.27.0.g.sh` with `environment-setup-cortexa72-webos-linux`: an x86_64 Linux host and a 64-bit Raspberry Pi 4 target. | D | [ose-ndk] |
| webosbrew's earlier NDK (`meta-lg-webos-ndk`, webOS OSE 1.0.g rebuilt as softfp) is deprecated since 2023-07-03 in favour of `native-toolchain`. | D | [meta-ndk] |

## 2. What the sysroot contains

### C library, C++ runtime, kernel headers

| Fact | Tag | Source |
|---|---|---|
| Both sysroots carry glibc 2.12.2: `lib/libc-2.12.2.so` says "GNU C Library stable release version 2.12.2" (compiled by GCC 12.2.0 in Xw, 16.2.0 in Xo), highest version node `GLIBC_2.12`. The recipe builds it from `openlgtv/glibc`, whose `version.h` says `2.12.2`. | Xw, Xo, D | `lib/libc.so.6`; [br-glibc], [glibc-fork] |
| Kernel headers are Linux 3.10.19 (`LINUX_VERSION_CODE 199187`; `BR2_DEFAULT_KERNEL_HEADERS="3.10.19"`). | Xw, Xo, D | `usr/include/linux/version.h`; [br-config] |
| libstdc++ is GCC 12.2.0's in Xw (`libstdc++.so.6.0.30`, `GLIBCXX_3.4.30`, headers `c++/12.2.0`) and GCC 16.2.0's in Xo (`libstdc++.so.6.0.36`, `GLIBCXX_3.4.36`, headers `c++/16.2.0`, plus `libstdc++exp.a`). Neither `libstdc++.a` has an undefined symbol that the sysroot's glibc lacks; GCC 16.2.0's refers to `__cxa_thread_atexit_impl` weakly. | Xw, Xo | `usr/lib`, `arm-webos-linux-gnueabi/lib`, `llvm-nm` |
| The sysroot's glibc does not export `getrandom`, `getentropy`, `secure_getenv` (only `__secure_getenv`), `aligned_alloc`, `timespec_get`, `at_quick_exit`, `sendmmsg`, `getauxval`, `explicit_bzero`, `memfd_create`, `gettid`, `prlimit`, `pthread_cond_clockwait`, `__cxa_thread_atexit_impl`, `c16rtomb`, `reallocarray` or `strlcpy`. `clock_gettime` and `clock_nanosleep` are in `librt.so.1`, not in libc. | Xw, Xo | `llvm-nm -D` on `libc.so.6`, `librt.so.1`, `libpthread.so.0` |
| Headers missing from both: `sys/random.h`, `threads.h`, `linux/membarrier.h`, `linux/memfd.h`. `uchar.h` is missing in Xw and present in Xo. | Xw, Xo | `usr/include` |
| `usr/lib/libglibc_polyfills.a` (from `smx-smx/glibc-polyfills`) defines `getauxval` in Xw, and `getauxval` and `aligned_alloc` in Xo. | Xw, Xo, D | `llvm-nm`; [br-poly], [polyfills] |
| The SDK's GCC is patched for `*-webos-*`: its default link line adds `--as-needed -lrt --no-as-needed` and `-lglibc_polyfills` (off with `-tno-lgtv-compat`, `-tno-librt-compat`, `-tno-glibc-polyfill`). clang knows none of this. | D | [br-compat] |
| `libstdc++.a` in both reads entropy for `std::random_device` from `/dev/urandom` or `/dev/random` only: it holds those two paths and no reference to `getentropy`, `getrandom` or `arc4random`. | Xw, Xo | `llvm-strings`, `llvm-nm` on `libstdc++.a` |

### Libraries

"Stub" means every function symbol aliases one address in a `.text` of under
300 bytes: a link-time stand-in for a library only the TV has. Versions are Xw
then Xo. The last column is [fw], tag D.

| Library | SONAME and version in the sysroot | Kind | On the 14 firmware images |
|---|---|---|---|
| SDL2 | `libSDL2-2.0.so.0`, 2.24.1 / 2.32.10, with `SDL_webOS.h` and `libSDL2.a` | real | SONAME in all; file versions `0.0.0` (1.x) up to `0.14.0` (7.x to 11.x), far older than the sysroot's |
| SDL2_image, _ttf, _mixer, _net | `libSDL2_image-2.0.so.0` 2.6.2 / 2.8.12, `libSDL2_ttf-2.0.so.0` 2.0.18 / 2.24.0, `libSDL2_mixer-2.0.so.0` 2.0.4 / 2.6.3, `libSDL2_net-2.0.so.0` | real | all four in all |
| EGL | `libEGL.so.1`; headers declare EGL 1.5 | stub, 47 function symbols | SONAME in all; a front for the vendor driver named in `DT_NEEDED`: `libIMGegl.so` (1.x, 2.x), `libmali.so` (3.4.0 to 9.2.0 and 11.2.0, where `libEGL.so.1` exports no symbol of its own), Mesa `libglapi.so.0` (10.2.0) |
| GLESv2 | `libGLESv2.so.2`; headers `GLES2/`, `GLES3/gl3.h`, `gl31.h`, `gl32.h` | stub, 644 function symbols, GLES 3.x names included | SONAME in all; `glBindVertexArray` is exported in 1.4.0, 3.4.0, 3.9.2 and 4.10.0 onward, and absent in 1.2.0, 2.2.3 and 4.4.2 |
| wayland-client | `libwayland-client.so.0`; headers are Wayland 1.11.0 | real | SONAME in all; `wl_proxy_marshal_constructor_versioned` and `wl_proxy_get_version`, which the sysroot's inline protocol wrappers call, are exported from 4.4.2 onward only |
| wayland-egl | SONAME is the unversioned `libwayland-egl.so` | stub, `wl_egl_window_{create,destroy,resize,get_attached_size}` | `libwayland-egl.so.0` up to 4.4.2, `libwayland-egl.so.1` from 4.10.0; the unversioned name is in all |
| wayland-webos-client | `libwayland-webos-client.so.1`; headers `wayland-webos-{shell,extension,foreign,input-manager,surface-group,tablet}-client-protocol.h`, `wayland-text-`, `wayland-input-method-` | interface tables, no code | in all; `wl_webos_shell_interface` in all, `wl_webos_input_manager_interface` from 1.4.0, `wl_webos_foreign_interface` from 5.3.1 |
| freetype | `libfreetype.so.6`, 2.12.1 / 2.14.3 | real | in all (`6.10.0` to `6.20.1`); `FT_Get_Color_Glyph_Layer` from 7.4.0 |
| fontconfig | `libfontconfig.so.1`, 2.13.1 / 2.17.1 | real | in all |
| harfbuzz | in neither sysroot | | `libharfbuzz.so.0` from 1.4.0, absent in 1.2.0 |
| libpng | `libpng16.so.16`, 1.6.38 / 1.6.58 | real | in all |
| libjpeg | `libjpeg.so.8` (libjpeg-turbo 2.1.4 / 3.2.0), `libturbojpeg.so.0` | real | `libjpeg.so.8` up to 3.9.2, `libjpeg.so.62` from 4.4.2: the sysroot's SONAME does not exist on webOS 4 and later |
| libwebp | in neither sysroot | | `libwebp.so.7` from 4.4.2 |
| zlib | `libz.so.1`, 1.2.13 / 1.3.2 | real | in all |
| glib | `libglib-2.0.so.0` with gio, gobject, gmodule, gthread; 2.72.3 / 2.88.3 | real | in all, 2.34.3 (1.x) to 2.78.6 (11.x) |
| gstreamer | `libgstreamer-1.0.so.0` and base libraries, 1.20.4 / 1.24.13 | real | in all, `0.9.0` (1.2.0) to `0.2410.0` (11.2.0) |
| PulseAudio, ALSA | `libpulse.so.0`, `libpulse-simple.so.0` (16.1 / 17.0), `libasound.so.2` (1.2.8 / 1.2.16.1) | real | in all |
| curl | `libcurl.so.4`, 7.87.0 / 8.22.0, linked to the sysroot's OpenSSL | real | `libcurl.so.5` up to 4.10.0, `libcurl.so.4` from 5.3.1 (the 5.x and 6.x images index both names) |
| OpenSSL | Xw `libssl.so.1.1`, `libcrypto.so.1.1` (1.1.1q); Xo `libssl.so.3`, `libcrypto.so.3` (3.6.4) | real | four SONAMEs, see section 4 |
| xkbcommon | `libxkbcommon.so.0`, 1.4.0 / 1.9.2, with `xkbcommon-webos-keysyms.h` | real | in all; `xkb_state_key_get_utf8` from 4.4.2 |
| luna-service2 | `libluna-service2.so.3`, `libluna-service2++.so.3`, 3.21.2 | real | in all (`3.6.0` on 1.x, `3.21.2` from 4.4.2 to 11.2.0); `LSRegisterApplicationService` from 3.4.0 |
| PmLogLib | `libPmLogLib.so.3`, `libPmLogLibCpp.so.3`, 3.3.0 | real | `libPmLogLib.so.3` in all |
| pbnjson | `libpbnjson_c.so.2`, `libpbnjson_cpp.so.2`, 2.15.0 | real | in all |
| NDL_directmedia | `libNDL_directmedia.so.1`; headers `libndl-media/NDL_directmedia{,_v1,_v2,_types,_common}.h` | stub, 36 function symbols | from 3.9.2; `NDL_DirectVideoOpen` in 3.9.2 to 4.10.0, `NDL_DirectMediaLoad` from 5.3.1 |
| starfish-media-pipeline | header `starfish-media-pipeline/StarfishMediaAPIs.h`; the library is `libplayerAPIs.so.1` | stub, 479 function symbols | no `libstarfish-media-pipeline` in any image; `libplayerAPIs.so.1` in all |
| ICU | `libicuuc.so.70` / `.78`, `libicui18n`, `libicudata`; 70.1 / 78.2 | real | a different SONAME per generation: `.50` (1.x, 2.x), `.53` (3.x), `.57` (4.x), `.62` (5.x, 6.x), `.66` (7.x, 8.x), `.70` (9.x, 10.x), `.74` (11.x) |
| sqlite, uuid | `libsqlite3.so.0` (3.40.1 / 3.53.4), `libuuid.so.1` | real | both in all |
| udev | `libudev.so.1` | real | `libudev.so.0` up to 3.9.2, `libudev.so.1` from 4.4.2 |
| libatomic | `libatomic.so.1` and `libatomic.a` | real | absent in 1.2.0, 1.4.0, 4.4.2 and 4.10.0 |

Other stubs in both: `libAcbAPI.so.1`, `libGLESv1_CM.so.1`, `libhelpers.so.2`,
`liblgncopenapi.so.2`, `libpf-1.0.so.1`. Also present and real: Qt 5.15, Boost,
libxml2, expat, json-c, lttng-ust, libunwind, the uMediaServer client libraries.

## 3. Target ABI

| Fact | Tag | Source |
|---|---|---|
| GCC is configured `--with-abi=aapcs-linux --with-cpu=cortex-a9 --with-fpu=neon-fp16 --with-float=softfp --with-mode=arm`. The buildroot config says the same (`BR2_GCC_TARGET_CPU="cortex-a9"`, `_FPU="neon-fp16"`, `_FLOAT_ABI="softfp"`, `_MODE="arm"`) and builds its packages with `-Os`, no stack protector and no RELRO. | D | [nt-gcc], [br-config] |
| The sysroot objects checked are ELF32 little-endian ARM with `e_flags 0x5000200` (EABI5, soft-float calling convention) and build attributes CPU `7-A`, `VFPv3`, `NEONv1`, half-precision extension, Thumb-2 permitted, and no `Tag_ABI_VFP_args`. Floats are passed in core registers; VFP and NEON instructions are used. | Xw, Xo | `llvm-readelf -h -A` on `libc.so.6`, `crt1.o`, `libstdc++.so.6` |
| The dynamic linker is `/lib/ld-linux.so.3` (the `libc.so` linker script and `PT_INTERP` of the link test), the soft-float name. | Xw, Xo | `usr/lib/libc.so` |
| Code is ARM mode by default. Thumb-2 is allowed by the attributes; the glibc recipe forces `-marm` for glibc itself ("Thumb build is broken"). The C++20 test also links with `-mthumb` against the ARM-mode static libraries. | D, Xo | [br-glibc]; link test |
| webosbrew wrote of webOS 4.x: "LG's real devices are using armv7a (with arm64 kernel)", built `softfp`. | D | [meta-ndk] |
| No webOS TV offers apps a 64-bit userland. In [fw], packages are named `lib32-*` from 7.4.0, and the 11.2.0 image indexes `ld-linux-aarch64.so.1` without symbol data. webosbrew's 64-bit bridge says the TV's EGL and GLES are "32-bit (armv7a)", that webOS 11 has a `lib64` glibc set which is "not available from a jailed environment", and that an aarch64 app must ship its own loader and proxy GL through a 32-bit process. | D | [fw], [bridge] |
| LG's own NDK for webOS 26 is 32-bit: tune `ca9v1`, environment `ca9v1-webosmllib32-linux-gnueabi`. flutter-webos maps `OECORE_TARGET_ARCH=aarch64` to `arm64` and keeps a commented `aarch64-webos-linux-` default, so a 64-bit target is anticipated, not shipped. | D, I | [ft-start], [ft-sdk] |

## 4. The glibc floor and what differs per release

Release years and names are LG's ([lg-engine]); the mapping of 7.x to 11.x onto
webOS 22 to 26 is webosbrew's ([brew-libver]); the rest is [fw], tag D.
libstdc++ file versions map to GCC releases per [gcc-abi].

| webOS (image) | Sold as | glibc | libstdc++ (GCC) | libcurl | OpenSSL SONAME |
|---|---|---|---|---|---|
| 1.2.0 (WT1M), 1.4.0 (WT1H) | 1.x, 2014 | 2.14.1 | 6.0.16 (4.6) | `.so.5` | `libssl.so.1.0.0` |
| 2.2.3 (W15M) | 2.x, 2015 | 2.19 | 6.0.19 (4.8) | `.so.5` | `libssl.so.1.0.0` |
| 3.4.0 (W16N), 3.9.2 (W17H) | 3.x, 2016 to 2017 | 2.19 | 6.0.19 (4.8) | `.so.5` | `libssl.so.1.0.0` |
| 4.4.2 (W18R), 4.10.0 (W19P) | 4.x, 2018 to 2019 | 2.24 | 6.0.22 (6) | `.so.5` | `libssl.so.1.0.0` |
| 5.3.1 (W20L) | 5.x, 2020 | 2.28 | 6.0.25 (8) | `.so.4` | `libssl.so.1.0.2` |
| 6.4.0 (W21P) | 6.x, 2021 | 2.28 | 6.0.25 (8) | `.so.4` | `libssl.so.1.0.2` and `libssl.so.1.1` |
| 7.4.0 (W22O), 8.3.0 (W23O) | webOS 22, 23 | 2.31 | 6.0.28 (9.3 or 10) | `.so.4` | `libssl.so.1.1` |
| 9.2.0 (W23O), 10.2.0 (W22H) | webOS 24, 25 | 2.35 (highest version node) | 6.0.29 (11) | `.so.4` | `libssl.so.3` |
| 11.2.0 (W23O) | webOS 26 | 2.39 (highest version node) | 6.0.32 (13.2) | `.so.4` | `libssl.so.3` |

| Fact | Tag | Source |
|---|---|---|
| The sysroot's glibc 2.12.2 is older than the oldest TV's (2.14.1), so a binary whose glibc needs stop at `GLIBC_2.12` asks for nothing a TV lacks. webosbrew's tutorial says a C11 SDL2 app built this way "can run through webOS 1 to webOS 6 without any issue". | I, D | table above; [pages-tutorial] |
| The sysroot is not an image of any webOS release: glibc older than any TV, kernel headers 3.10, upstream libraries newer than any TV, stubs for LG's libraries. No README or release note names a webOS version for it. webosbrew: "The SDK one holds the generic buildroot build, which is what you compile against. It is not what the TV runs". | Xw, Xo, D | section 2; [brew-gdb] |
| Old symbol versions stay resolvable on newer TVs: `clock_gettime@GLIBC_2.4` is in `librt.so.1` up to 6.4.0 and in `libc.so.6` from 7.4.0; `pthread_create@GLIBC_2.4` and `dlopen@GLIBC_2.4` are in `libpthread.so.0` and `libdl.so.2` up to 8.3.0 and in `libc.so.6` from 9.2.0. | D | [fw] |
| "Do not link the system OpenSSL. It crosses four SONAMEs between webOS 1 and webOS 11, so a binary linked against one will not start on a TV carrying another." | D | [brew-net], [brew-crypto] |
| "before webOS 5, webOS shipped libcurl as `libcurl.so.5` instead of `libcurl.so.4`". Homebrew apps bridge it with an empty shim library named for one SONAME and linked to the other. | D | [brew-net] |
| A shipping app's notes: "libstdc++ is linked statically, never bundled". A bundled `lib/libstdc++.so.6` found through `DT_RPATH` outranks `LD_LIBRARY_PATH` and is then used by the TV's own C++ libraries, which need more: "webOS 11's `libNDL_media_impl.so.1` needs `GLIBCXX_3.4.32`", webOS 10's needs 3.4.30. | D | [punkt-notes] |
| The same notes: "glibc shims required", because "webOS glibc ~2.12 lacks `getauxval`/`gettid`/`sendmmsg`" (that is the toolchain's glibc; no image in [fw] is older than 2.14.1); and on getrandom, the libc lookup "finds nothing on webOS 4.x's old glibc, then falls back to /dev/urandom, which the app sandbox does not expose", so they issue the syscall directly. Their devices: LG CX (webOS 5.6) and G5 (webOS 10.3). | D | [punkt-notes], [punkt-cargo] |
| The jailer sets the app directory's permissions wrong before webOS 4 (unwritable until reboot on 3, always on 1 and 2), and native services are "only properly supported since webOS 4.x". | D | [brew-fs], [pages-native] |

## 5. How homebrew checks compatibility

| Fact | Tag | Source |
|---|---|---|
| `dev-toolbox-cli` (0.10.0, Apache-2.0) builds `webosbrew-elf-verify` (one executable: missing libraries and symbols per firmware), `webosbrew-ipk-verify` (a whole IPK and which releases it should run on), `webosbrew-gen-manifest` and `webosbrew-fw-symbols-extract`. Releases are Debian packages (`webosbrew-toolbox-*`) for amd64 and arm64 only. | D | [dtc], [dtc-rel], [brew-tools] |
| Its data is `common/data/<firmware>/`: `info.json` (firmware version, OTA id, webOS release), `index.json` (SONAME to file) and one JSON per library with `name`, `package`, `needed`, `symbols`. | D | [fw] |
| Exit codes: 0 compatible, 1 an incompatibility, 3 unreadable input, 4 no firmware data, 6 "built in a way no TV will install". | D | [dtc] |
| "It reads the package, it does not run it. A library opened with `dlopen`, a code path behind a feature test, anything decided at run time, none of that is visible to it." | D | [brew-workflow] |
| Predecessors: `compat-checker` (deprecated) and `dev-utils` (2023), which holds `scripts/verify-symbols.sh`, `verify-package.sh`, `gen_webosbrew_manifest.js` and `cmake/WebOSPackage.cmake`. | D | [dev-utils] |

## 6. clang and lld against the sysroot

| Fact | Tag | Source |
|---|---|---|
| clang cross-compiles against both SDKs with `--target=arm-webos-linux-gnueabi --sysroot=<sdk>/arm-webos-linux-gnueabi/sysroot --gcc-install-dir=<sdk>/lib/gcc/arm-webos-linux-gnueabi/<12.2.0 or 16.2.0> -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp -marm -fPIC --ld-path=<ld.lld>`. It reports the GCC installation as selected, finds the C++ headers at `<gcc dir>/../../../../arm-webos-linux-gnueabi/include/c++/<version>`, takes `Scrt1.o`, `crti.o`, `crtn.o` from the sysroot and `crtbeginS.o`, `crtendS.o`, `libgcc` from the GCC directory, and compiles as `armv7-webos-linux-gnueabi`, `-target-abi aapcs-linux`, soft-float ABI with `+vfp3 +neon +fp16`. | Xw, Xo | link test, `clang -v` |
| Without `-lrt`, a C program calling `clock_gettime` fails: `ld.lld: error: undefined symbol: clock_gettime`. | Xw | link test |
| A C++20 test (ranges, concepts, coroutines, `jthread`, atomics, `filesystem`, `random_device`, `thread_local`, exceptions, `<=>`) links as a PIE and as `-shared -Wl,--no-undefined` with `-static-libstdc++ -static-libgcc -pthread -l:libatomic.a -Wl,--as-needed -lrt -Wl,--no-as-needed`. The result needs `librt.so.1`, `libm.so.6`, `libpthread.so.0`, `libc.so.6`, `ld-linux.so.3`, symbol versions `GLIBC_2.4` and `GLIBC_2.6` only, has interpreter `/lib/ld-linux.so.3` and `e_flags 0x5000200`, and no strong undefined symbol outside the sysroot's glibc. | Xw, Xo | link test, `llvm-readelf -d --version-info`, `llvm-nm -D` |
| With Xo, `<format>`, `<expected>` and chrono formatting compile and link at `-std=c++23`. libstdc++ 12.2.0 (Xw) has neither `<format>` nor chrono calendars and time zones: its status table leaves P0645R10 and P0355R7 empty. So no newer libstdc++ has to be built; Xo ships one. | Xo, D | link test; [gcc12-status] |
| Apple clang 21.0.0 parses the same C++20 test against both header sets (`-fsyntax-only`). | Xw, Xo | `/usr/bin/clang++` |
| lld links against every library of section 2 as shipped (one test naming EGL, GLESv2, wayland-client, wayland-egl, wayland-webos-client, freetype, fontconfig, png16, z, glib, gobject, luna-service2, PmLogLib, xkbcommon, icuuc, sqlite3, uuid, curl, pulse, asound, SDL2, gstreamer, pbnjson_c, NDL_directmedia, jpeg). No patched copy is needed, unlike the Tizen rootstrap (`src/sysroot/mirror-sysroot.ts`). | Xw, Xo | link test |
| The Linux-host archive serves a Mac: clang reads only target files. Extracting Xo on case-insensitive APFS makes `tar` report errors inside `usr/share/terminfo` and silently merges 8 header pairs under `usr/include/linux/netfilter*` (`xt_CONNMARK.h` and `xt_connmark.h`). | Xo | `tar`, archive listing |
| macOS's `tar` (bsdtar) extracts `arm-webos-linux-gnueabi/sysroot`, `arm-webos-linux-gnueabi/include`, `arm-webos-linux-gnueabi/lib` and `lib/gcc/arm-webos-linux-gnueabi/16.2.0` from Xo with `--strip-components=1` and one `--exclude` for each of `usr/share/{doc,i18n,locale,man,terminfo,zoneinfo}` of the sysroot, without an error, in 5 seconds: 469 MB on disk, 411 MB of it the sysroot. Against that subset alone, the clang and lld of this file link a C++20 program (`jthread`, atomics, `filesystem`, `clock_gettime`) with the flags of "What this means": ELF32 ARM, `e_flags 0x5000200`, interpreter `/lib/ld-linux.so.3`, needing `librt.so.1`, `libm.so.6`, `libpthread.so.0`, `libc.so.6` and `ld-linux.so.3`. | Xo | `rn-webos setup` (`src/sysroot/webos-sdk.ts`) on macOS 27 arm64, 2026-11-11; link test |
| Projects building for webOS TV with LLVM: LG's flutter-webos (clang, section 1) and Rust apps such as punktfunk (`armv7-unknown-linux-gnueabi`, `+neon,+vfp3`, linked through webosbrew's GCC; "needs webOS 5 or newer"). No project was found that links with lld or ships a clang-built C++ runtime. | D | [ft-build], [punkt-cargo], [brew-native] |

## 7. Network libraries

| Fact | Tag | Source |
|---|---|---|
| "Let libcurl Carry TLS" is webosbrew's simplest way out of the OpenSSL spread; "Bundle mbedTLS" and "Bundle OpenSSL" are the others. libcurl is "Present since webOS 1, and the version moves without breaking the ABI". | D | [brew-crypto], [brew-net] |
| The TV's libcurl exports `curl_multi_poll`, `curl_multi_wakeup` and `curl_url` from 7.4.0 only, `curl_mime_init` from 5.3.1, `curl_ws_send` in 11.2.0 only. It links `libnghttp2.so.14` from 4.10.0, and OpenSSL 1.0.0 up to 4.10.0, 1.0.2 on 5.3.1, 1.1 from 6.4.0, 3 from 9.2.0. `SSL_CTX_set_ciphersuites`, the TLS 1.3 call, is exported from 6.4.0 only. | D | [fw] |
| The runtime copied from Tizen calls `curl_multi_wakeup`, `curl_multi_poll` and `curl_url`, so the TV's libcurl cannot serve it before webOS 7. | I | `runtime/host/src/network/HttpEngine.cpp:93,105,210`, `WebSocketHandshake.cpp:86`; row above |
| For web apps, TLS 1.3 is "Not supported" on webOS TV 1.x to 4.x and supported from 5.x; TLS 1.2 everywhere. The trusted roots vary by platform version. ISRG Root X1 "is supported from webOS TV 5.0", and on 3.x and 4.x only "if the software update is properly performed". | D | [lg-tls] |
| Homebrew Channel ships its own CA bundle (`services/certs/cacert-2024-03-11.pem`) and makes its HTTPS client trust that instead of the TV's. | D | [hbc] |
| A script for rooted LG B9 and C9 TVs edits `/etc/ca-certificates.conf`, overlays `/etc/ssl` and `/usr/share/ca-certificates`, and runs `update-ca-certificates`: the Debian ca-certificates layout. No source opened shows the bundle file itself, so `/etc/ssl/certs/ca-certificates.crt` is a guess from that layout. | D, I | [tf318] |
| The jail maps `/bin`, `/lib` and `/usr/lib` "among them", read only. Whether it maps `/etc/ssl` is not documented. | D | [brew-vocab] |
| `libmbedtls` is on the 11.2.0 image only. | D | [fw] |
| curl publishes Mozilla's root certificates as one PEM file per extraction date, `https://curl.se/ca/cacert-<date>.pem`, with its digest at the same address plus `.sha256`. The file "is only a converted version of the original one and thus it is licensed under the same license as the Mozilla source file: MPL 2.0". `cacert-2026-11-05.pem` is 200,116 bytes, 118 certificates, sha256 `427c064216761a1e72a8b30f0d954d08cb6e4435a82091fb865afd20c8497285` (downloaded; equals the published digest). | D | [curl-ca] |
| curl 8.23.0 (2026-10-14) builds with "mbedTLS 3.2.0 or greater"; Mbed TLS 3.6.7 (2026-07-07) is the newest release of its 3.6 long-time support branch. `curl-8.23.0.tar.xz` is sha256 `8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5` (the copy on curl.se hashes the same) and `mbedtls-3.6.7.tar.bz2` `a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6` (equals the release's `sha256sum.txt`). | D | [curl] `CMakeLists.txt` L875-L879; [mbedtls] `BRANCHES.md`; the two projects' GitHub releases, downloaded 2026-11-11 |
| "mbedTLS can be used safely in a multi-threaded environment provided that mbedTLS is compiled with MBEDTLS_THREADING_C enabled." Its default configuration leaves that off, and has the server side, DTLS and a key store in files on. | D | [curl] `docs/libcurl/libcurl-thread.md`; [mbedtls] `include/mbedtls/mbedtls_config.h` |
| Mbed TLS takes entropy from the `getrandom` system call only when `SYS_getrandom` is defined, and reads `/dev/urandom` when it is not, or when the call answers `ENOSYS`. `MBEDTLS_ENTROPY_HARDWARE_ALT` adds a source the application defines, `mbedtls_hardware_poll`, and `MBEDTLS_NO_PLATFORM_ENTROPY` removes the built-in one. curl seeds its generator from that and turns prediction resistance on: "Entropy is gathered before each mbedtls_ctr_drbg_random() call". | D | [mbedtls] `library/entropy_poll.c` L81-L95, L150-L185, `include/mbedtls/mbedtls_config.h` L531-L543, L1201-L1223; [curl] `lib/vtls/mbedtls.c` L1594-L1625 |
| curl's CMake looks for a CA bundle on the build machine unless it cross-compiles; `CURL_CA_BUNDLE=none` turns that off and a path becomes the compiled-in default. With Mbed TLS as the only backend, HTTP and HTTPS as the only protocols and zlib, it reports the features `AsynchDNS HTTPS-proxy IPv6 Largefile libz SSL threadsafe`. | D, B | [curl] `CMakeLists.txt` L1408-L1470; configure log |

## 8. Debugging and logs in Developer Mode

| Fact | Tag | Source |
|---|---|---|
| The official CLI's TV profile has no `ares-log`, `ares-shell`, `ares-push` or `ares-pull`; those are OSE and APOLLO only. The TV profile has `ares-novacom` (`--getkey`, `--run DEVICE_COMMAND`, `--forward`), `ares-install`, `ares-launch`, `ares-inspect`, `ares-device`. `ares-log` "shows, filters, and saves logs collected by journald" on OSE. | D | [wt-cli], [lg-cli], [ose-cli] |
| Developer Mode gives SSH on port 9922 as user `prisoner`, with a key fetched from the TV's key server, for a limited session (webosbrew: 1000 hours, then apps are removed). The server is old: a current OpenSSH needs `HostKeyAlgorithms ssh-rsa` and `PubkeyAcceptedKeyTypes ssh-rsa`. The `prisoner` shell has no PTY. | D | [lg-devmode], [brew-devmode], [brew-env], [pages-sdk] |
| webosbrew's `ares-cli-rs` adds `ares-shell`, `ares-push`, `ares-pull` and `ares-novacom --reverse`, which publishes a host port on the TV's loopback. It has no `ares-log` either. | D | [ares-rs], [ares-rs-novacom] |
| No source says the app launcher keeps a native app's stdout or stderr. They are seen when the binary is started from a shell: as root through `jailer -t native_devmode ...`, or directly with `APPID=<id>` set, since "The launcher sets `APPID`. A shell does not" and registration with the application manager fails quietly without it. | D, I | [pages-native], [brew-gdb] |
| A native app receives a JSON object in `argv[1]`, such as `{"@system_native_app":true,"nid":"com.retroarch"}`. | D | [pages-native] |
| PmLogLib messages go to `PmLogDaemon` and `/var/log/messages`; production firmware logs whitelisted sources only, and lifting that takes `luna-send` calls on the TV. "On non-rooted devices" webosbrew points instead to a PmLogLib build that prints to standard output. | D | [pages-logging], [meta-ndk] |
| Crash reports are gzipped text (maps, backtraces) in `/var/log/reports/librdx/` or `/tmp/faultmanager/crash/`, sent to LG unless `rdxd` is blocked; `SEGFAULT_SIGNALS=` turns them off. webosbrew's "Read a Core Dump" section is empty. | D | [pages-logging], [brew-gdb] |
| The Developer Mode app ships a `gdbserver` "old enough to be barely worth using". Xo ships `arm-webos-linux-gnueabi/debug-root/usr/bin/gdbserver` (GDB 16.3, needs `libatomic.so.1`, which the 1.x and 4.x images lack) and a Linux-host `bin/arm-webos-linux-gnueabi-gdb`; Xw ships neither. GDB's sysroot should be a copy of that TV's own `/lib` and `/usr/lib`. | D, Xo, Xw | [brew-gdb]; archives; [fw] |
| The jail gives the app a private root, a non-root user, `/media/developer` writable as `HOME`; the app lives in `/media/developer/apps/usr/palm/applications/<appid>`. | D | [brew-vocab], [brew-fs] |

## Open questions

- Does a clang and lld build start on a TV at all? Nothing was run: relocation
  handling, the `DT_GNU_HASH`-only table clang asks for, PIE in the jail and
  static libgcc unwinding across the runtime's `.so` are untested on hardware.
- Which glibc 2.12.2 gaps Hermes, folly, React Native, ICU, curl and mbedTLS
  hit. Only a real build tells; the list in section 2 is where to look first.
- Whether the jail exposes `/dev/urandom`, on which webOS versions, and which
  kernel each release runs (`getrandom(2)` needs Linux 3.17). One project
  reports it missing; `std::random_device` in the static libstdc++ depends on it.
- Where the CA bundle is on a TV and whether the jail maps it.
- Whether stdout and stderr of a launcher-started native app go anywhere, and
  whether `prisoner` can read crash reports.
- The oldest clang that accepts libstdc++ 16.2.0 headers. Tested: Homebrew
  clang 23.1.4 (build) and Apple clang 21.0.0 (parse only). Ubuntu's packaged
  clang and lld were not tried.
- What LG's webOS NDK 11.2.0 contains (glibc, GCC, flags), and whether its
  licence allows any use outside Flutter. Not unpacked.
- How far [fw] generalises: one SoC per release. GLES 3 is missing on the
  4.4.2 image (W18R) and present on 4.10.0 (W19P).
- Whether a `-mthumb` build runs and what it saves on the real runtime.
- `native-toolchain`'s `main` gained Linux x86_64 and GCC 14 host builds in
  2025-10 with no release since 2024-03; a newer macOS port may appear.

## What this means for react-native-webos

**Sysroot.** Use openlgtv/buildroot-nc4 `2026.08-webos`. It has the same glibc
2.12.2 floor as webosbrew's port and a libstdc++ from GCC 16.2.0 that links
against it. One archive serves macOS and Linux, because clang reads only its
target files:

- `https://github.com/openlgtv/buildroot-nc4/releases/download/2026.08-webos/arm-webos-linux-gnueabi_sdk-buildroot-aarch64.tar.gz`,
  sha256 `3a9330f3789f8008ece7c8a48f58c3a71d0307a9a1f81896db938b785524c3ce`
  (downloaded; equals the digest GitHub's API reports).
- Fallback with a macOS GCC and a GCC 12.2.0 runtime:
  `https://github.com/webosbrew/native-toolchain/releases/download/webos-d7ed7ee.6/arm-webos-linux-gnueabi_sdk-buildroot_darwin-arm64.tar.bz2`,
  sha256 `954333694ab1fe1f7737e81a0be1e3188cdfffc1a119c81c86f5fcebf1857c45`
  (downloaded; GitHub reports no digest for it).

Verify the sha256, then extract only `arm-webos-linux-gnueabi/sysroot` (without
`usr/share/{doc,i18n,locale,man,terminfo,zoneinfo}`),
`arm-webos-linux-gnueabi/include`, `arm-webos-linux-gnueabi/lib` and
`lib/gcc/arm-webos-linux-gnueabi/16.2.0`. Do not run `relocate-sdk.sh`: clang
takes every path as a flag, and `PKG_CONFIG_SYSROOT_DIR` with
`PKG_CONFIG_LIBDIR` set as `tizen.cmake` does covers pkg-config. No ELF
patching mirror is needed. LG's NDK cannot be mirrored under its licence.

**Compile flags.**

```
--target=arm-webos-linux-gnueabi --sysroot=<sdk>/arm-webos-linux-gnueabi/sysroot
--gcc-install-dir=<sdk>/lib/gcc/arm-webos-linux-gnueabi/16.2.0
-mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp -marm -fPIC
-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
```

**Link flags.**

```
--ld-path=<ld.lld> -static-libstdc++ -static-libgcc -pthread -Wl,--no-undefined
-l:libatomic.a -Wl,--as-needed -lrt -Wl,--no-as-needed
```

plus a compat archive of hidden definitions for what glibc 2.12.2 lacks, as
`runtime/compat/glibc-compat.c` does for Tizen (or `-lglibc_polyfills` for
`getauxval` and `aligned_alloc`). Linking against the 2.12.2 sysroot with
`--no-undefined` is itself the glibc check: a libc symbol newer than 2.12
fails the link.

**Link statically or compile in.** libstdc++, libgcc and libatomic (never
bundle `libstdc++.so.6`; `libatomic.so.1` is missing on webOS 1 and 4). curl
with mbedTLS (or a static OpenSSL) and a CA bundle file shipped in the ipk: the
TV's libcurl changes SONAME at webOS 5, lacks the calls the runtime makes
before webOS 7, has no TLS 1.3 before webOS 6 and trusts an old root list.
ICU, as on Tizen. libjpeg-turbo, libwebp and harfbuzz if used: wrong SONAME,
absent before 4.x, absent on 1.2.

**Safe to link from the TV** (same SONAME on all 14 images): `libc.so.6`,
`libm.so.6`, `libpthread.so.0`, `libdl.so.2`, `librt.so.1`, `libEGL.so.1`,
`libGLESv2.so.2`, `libwayland-client.so.0`, `libwayland-egl.so` (the unversioned
name the stub records), `libwayland-webos-client.so.1`, `libxkbcommon.so.0`,
`libfreetype.so.6`, `libfontconfig.so.1`, `libpng16.so.16`, `libz.so.1`,
`libglib-2.0.so.0`, `libluna-service2.so.3`, `libPmLogLib.so.3`,
`libpbnjson_c.so.2`, `libpulse.so.0`, `libasound.so.2`, `libsqlite3.so.0`,
`libuuid.so.1`. A SONAME is not a symbol: the sysroot's headers are newer than
any TV, so every such call needs the check below. Treat GLES 2 as the baseline
and probe GLES 3 at run time. Load `libNDL_directmedia.so.1` with `dlopen`: it
is absent on the 3.4.0 image and older, and has two API generations.

**Floor.** webOS 4.x (2018) is the lowest release these facts support without
extra work: Wayland code built from the sysroot's headers needs
`wl_proxy_marshal_constructor_versioned`, which the 1.x to 3.x images lack, and
the jailer and native services misbehave before 4. The build still targets
glibc 2.12.2, because no other public sysroot exists. The floor is a decision
for `docs/decisions.md`.

**Check every build** against [fw]: each `DT_NEEDED` SONAME and each undefined
symbol, per firmware. The data is Apache-2.0 JSON, so `rn-webos` can read it
with zod on macOS, where `webosbrew-ipk-verify` has no release.

**One 32-bit build.** No release gives an app a 64-bit userland, and LG's own
NDK for webOS 26 is 32-bit.

**Logs and debugging.** Keep the Tizen design: the app streams its logs to the
CLI over a socket, since `ares-log` does not exist for TVs and stdout is only
visible from a shell. For a shell run, set `APPID`. For GDB, push the SDK's
`gdbserver` (with `libatomic.so.1` beside it on webOS 4) and forward its port
with `ares-novacom --forward`.

[nt]: https://github.com/webosbrew/native-toolchain/blob/730dbd680cc319935655b1b04c2d80fcb62fdb22/README.md
[mbedtls]: https://github.com/Mbed-TLS/mbedtls/tree/mbedtls-3.6.7
[curl]: https://github.com/curl/curl/tree/curl-8_23_0
[curl-ca]: https://curl.se/docs/caextract.html
[nt-rel]: https://github.com/webosbrew/native-toolchain/releases
[nt-cmake]: https://github.com/webosbrew/native-toolchain/blob/730dbd680cc319935655b1b04c2d80fcb62fdb22/CMakeLists.txt
[nt-sysroot]: https://github.com/webosbrew/native-toolchain/blob/730dbd680cc319935655b1b04c2d80fcb62fdb22/sysroot/CMakeLists.txt
[nt-patch]: https://github.com/webosbrew/native-toolchain/blob/730dbd680cc319935655b1b04c2d80fcb62fdb22/sysroot/relocate-sdk.patch
[nt-gcc]: https://github.com/webosbrew/native-toolchain/blob/730dbd680cc319935655b1b04c2d80fcb62fdb22/package/gcc/CMakeLists.txt
[br-rel]: https://github.com/openlgtv/buildroot-nc4/releases
[br-config]: https://github.com/openlgtv/buildroot-nc4/blob/322ff04e625a7927338c2ac5f3d02cd0511c9b90/configs/webos_tv_defconfig
[br-compat]: https://github.com/openlgtv/buildroot-nc4/blob/322ff04e625a7927338c2ac5f3d02cd0511c9b90/package/gcc/16.2.0/0003-lgtv-compat.patch
[br-glibc]: https://github.com/openlgtv/buildroot-nc4/blob/322ff04e625a7927338c2ac5f3d02cd0511c9b90/package/glibc/glibc.mk
[br-poly]: https://github.com/openlgtv/buildroot-nc4/blob/322ff04e625a7927338c2ac5f3d02cd0511c9b90/package/glibc-polyfills/glibc-polyfills.mk
[glibc-fork]: https://github.com/openlgtv/glibc/blob/fe819f0414e6385206dc8682c20d681ee0eb5998/version.h
[polyfills]: https://github.com/smx-smx/glibc-polyfills/tree/cf6fa6f9b00f833a2bcc4cec12ecb3ff65ab83aa/src
[meta-ndk]: https://github.com/webosbrew/meta-lg-webos-ndk/blob/3b146731c4/README.md
[fw]: https://github.com/webosbrew/dev-toolbox-cli/tree/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/common/data
[dtc]: https://github.com/webosbrew/dev-toolbox-cli/blob/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/README.md
[dtc-rel]: https://github.com/webosbrew/dev-toolbox-cli/blob/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3/RELEASING.md
[dev-utils]: https://github.com/webosbrew/dev-utils/tree/b25ad97e73/scripts
[brew-env]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/env-setup.md
[brew-libver]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/library-version.md
[brew-net]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/native/net.md
[brew-crypto]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/crypto.md
[brew-gdb]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/gdb.md
[brew-workflow]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/workflow.md
[brew-tools]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/tools.md
[brew-fs]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/filesystem.md
[brew-vocab]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/vocabulary.md
[brew-native]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/develop/guides/native/index.md
[brew-devmode]: https://github.com/webosbrew/webosbrew.github.io/blob/fc4b8aaa23cbb4784f8eabc540a778d44965b3ab/src/views/devmode/index.md
[pages-native]: https://github.com/webosbrew/pages/blob/c97b41f72a/content/pages/native.md
[pages-logging]: https://github.com/webosbrew/pages/blob/c97b41f72a/content/pages/logging.md
[pages-sdk]: https://github.com/webosbrew/pages/blob/c97b41f72a/content/pages/sdk.md
[pages-tutorial]: https://github.com/webosbrew/pages/blob/c97b41f72a/content/pages/develop/tutorials/native-gui-app.md
[bridge]: https://github.com/webosbrew/webos-bridge-64to32/blob/f767726f1d/README.md
[punkt-notes]: https://github.com/dyptan-io/punktfunk-webos/blob/1e42dde55d6590012c256e54588d8762465d372d/docs/NOTES.md
[punkt-cargo]: https://github.com/dyptan-io/punktfunk-webos/blob/1e42dde55d6590012c256e54588d8762465d372d/.cargo/config.toml
[hbc]: https://github.com/webosbrew/webos-homebrew-channel/blob/6e928d01b2/services/fetch-wrapper.ts
[tf318]: https://github.com/tf318/lg/blob/036ce46a1574d6020477a4bfbe27181335b40be3/update-ca-certs.sh
[ares-rs]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/README.md
[ares-rs-novacom]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-novacom/README.md
[wt-cli]: https://github.com/webos-tools/cli/blob/ed7a6994be443207f3427b19f210f6606f004eb6/README.md
[ft-start]: https://github.com/lg-flutter-webos/flutter-webos/blob/950398131830e9bd5dc91e588c3db8f5c42322be/doc/getting-started.md
[ft-build]: https://github.com/lg-flutter-webos/flutter-webos/blob/950398131830e9bd5dc91e588c3db8f5c42322be/lib/webos_build_target.dart#L644-L672
[ft-sdk]: https://github.com/lg-flutter-webos/flutter-webos/blob/950398131830e9bd5dc91e588c3db8f5c42322be/lib/webos_sdk.dart#L27-L32
[lg-ndk]: https://github.com/lg-flutter-webos/ndk/releases/tag/11.2.0
[lg-engine]: https://webostv.developer.lge.com/develop/specifications/web-api-and-web-engine
[lg-tls]: https://webostv.developer.lge.com/develop/specifications/tls
[lg-cli]: https://webostv.developer.lge.com/develop/tools/cli-dev-guide
[lg-discover]: https://webostv.developer.lge.com/discover
[lg-devmode]: https://webostv.developer.lge.com/develop/getting-started/developer-mode-app
[lg-flutter-news]: https://webostv.developer.lge.com/news/flutter-for-webos-tv-is-now-available-for-developers
[ose-ndk]: https://www.webosose.org/docs/guides/setup/setting-up-native-development-kit/
[ose-cli]: https://www.webosose.org/docs/tools/sdk/cli/cli-user-guide/
[gcc-abi]: https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
[gcc12-status]: https://gcc.gnu.org/onlinedocs/gcc-12.2.0/libstdc++/manual/manual/status.html
