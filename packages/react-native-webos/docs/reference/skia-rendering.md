# Skia rendering on LG webOS TVs

Whether Skia can replace Evas as the renderer of a React Native host on webOS,
and how to build it. webOS has no EFL: the brief for this port gives a native app
a Wayland surface with EGL and OpenGL ES 2 on armv7 SoCs (2016 to 2026 TVs).
Neither is verified here; the webOS platform reference owns those facts. What the
renderer must do is listed in [efl-rendering.md](efl-rendering.md). Nothing in
this file was built or run: section 10 says what the first build has to confirm.

| Mark | Meaning |
| --- | --- |
| doc | Documented: stated in a project's documentation or a specification |
| src | Read from source at the commit named under Sources |
| inf | Inferred; the row says from what |
| obs | Observed on 2026-11-11 by a download or a query; the row says which |
| unk | Unknown; the row says how to find out |

## Sources

| Key | Where |
| --- | --- |
| `sk:` | Skia `chrome/m157` at `fef60e46c522b87e5802ddf0a215a763dd5e0483`, `https://github.com/google/skia/blob/fef60e46c522b87e5802ddf0a215a763dd5e0483/<path>#L<line>` |
| SK-build | "How to build Skia", `https://skia.org/docs/user/build/`, the same text as `sk:site/docs/user/build.md` |
| SK-rel | "Release Information", `https://skia.org/docs/user/release/`, `sk:site/docs/user/release/_index.md` |
| CD | Chrome's schedule, `https://chromiumdash.appspot.com/fetch_milestone_schedule?mstone=<n>` |
| `rs:` | rust-skia at `982dc49d2377895283fe333b5b08e3ee7e57dec5`, `https://github.com/rust-skia/rust-skia/blob/982dc49d/<path>` |
| `sp:` | JetBrains skia-pack at `bb88f41164f6a427576f612e15b7b86526d566ef` (archived), `https://github.com/JetBrains/skia-pack/blob/bb88f411/<path>` |
| `rns:` | Shopify react-native-skia (now `wcandillon/react-native-skia`) at `ebaccebb63386d2ce20a518c881174864af87806` |
| `nag:` | `react-native-skia/react-native-skia` (Nagra, Kudelski, OpenTV) at `df637f2f4448cb2eaa281c3c0e16825a10afb42e`, `https://github.com/react-native-skia/react-native-skia/blob/df637f2f/<path>` |
| `fl:` | Flutter at `b98afb52407777ee1b2544b93b064335ed56e340`, `https://github.com/flutter/flutter/blob/b98afb52/engine/src/flutter/<path>` |
| `vcpkg:` | `https://github.com/microsoft/vcpkg/blob/master/ports/skia/portfile.cmake`, read 2026-11-11 |
| GN-ref | GN reference, `https://gn.googlesource.com/gn/+/main/docs/reference.md` |
| `icu:` | ICU 78.3 source, the archive pinned in `runtime/dependencies.json`, `icu/source/<path>` |
| GCC-status, GCC-abi | `https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html`, `.../abi.html` |
| EGL-damage, EGL-age, EGL-interval | `https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_swap_buffers_with_damage.txt`, `.../EXT/EGL_EXT_buffer_age.txt`, `https://registry.khronos.org/EGL/sdk/docs/man/html/eglSwapInterval.xhtml` |

## 1. Version, releases, license, backends, compiler

| Fact | Mark | Source |
| --- | --- | --- |
| Skia has no numbered releases. It "cut[s] a stable milestone every four weeks along with Chrome" as a branch `chrome/mNNN`, and recommends tracking tip of tree | doc | SK-rel, `sk:site/docs/user/release/_index.md#L11-L22` |
| Chrome's branch points are two weeks apart now: m155 2026-09-14, m156 09-28, m157 10-12, m158 10-26, m159 11-09. m157 went stable on 2026-11-03, m158 is due 2026-11-17. m150 is the long-term-support milestone, refreshed until 2027-03-23 | obs | CD for 150 and 155 to 159 |
| `chrome/m157` is at `fef60e46` (2026-11-10, a Ganesh cherry-pick), `SK_MILESTONE 157`. A branch moves with cherry-picks, so a pin names a commit | obs + src | `git ls-remote`, `sk:include/core/SkMilestone.h` |
| Others on current milestones: rust-skia builds m157 (`skia = "m157.3"`); react-native-skia's submodule is milestone 158 and builds Android `cpu: "arm"`; skia-pack stopped at m144 when it was archived | src | `rs:skia-bindings/Cargo.toml#L36`, `rns:packages/skia/scripts/skia-configuration.ts#L148-L155`, `sp:.github/workflows/build.yml#L13`, `sp:README.md` |
| License: BSD 3-clause, "Copyright (c) 2011 Google Inc." | src | `sk:LICENSE` |
| "Skia provides no endorsement or recommendation on the revision to use" for the libraries in `DEPS`: they are what Skia happens to test | src | `sk:DEPS#L1-L4` |
| Ganesh's GL backend takes OpenGL ES 2.0: the GLES interface is refused only below 2.0 | src | `sk:src/gpu/ganesh/gl/GrGLAssembleGLESInterfaceAutogen.cpp#L37-L42` |
| Graphite has Dawn, Metal and Vulkan backends, no GL: "We only have a GL backend for Ganesh". It is off by default | src | `sk:src/gpu/graphite/` (`dawn/`, `mtl/`, `vk/`), `sk:gn/skia.gni#L113`, `#L198-L200` |
| The GN build compiles with `-std=c++20`, `-fno-exceptions`, `-fno-rtti`, `-fPIC`, `-fvisibility=hidden`. m130 used C++17, m140 either, m144 and later C++20 only | src | `sk:gn/skia/BUILD.gn#L127-L133`, `#L410-L416`, `#L732-L743`; the same file on `chrome/m130`, `m140`, `m144` |
| "Skia makes use of C++20 language features [...] Clang 21 implements most the features of the c++20 standard." Other compilers "should compile" but give "dramatically worse performance" in the software backend, image decoding and color conversion | doc | SK-build, `sk:site/docs/user/build.md#L53-L64` |
| C++20 library used outside PDF, Vulkan and Graphite: `<bit>` (`std::popcount`, `std::countl_zero`), `std::convertible_to` and `std::derived_from` in `requires` clauses, `std::ranges` concepts. `<span>` only when `SK_USE_LEGACY_SKSPAN` is undefined, and Skia defines it. No `std::bit_cast`, `<format>` or `<source_location>` | src | `sk:src/core/SkMathPriv.h#L15-L37`, `sk:src/text/gpu/GlyphVector.h#L74-L93`, `sk:src/codec/SkCodecPriv.h#L424-L426`, `sk:include/core/SkSpan.h#L11-L15`, `#L146-L153` |
| libstdc++ has the concepts library, ranges and `<compare>` from GCC 10.1 (`std::bit_cast` 11.1, `<format>` 13.1) | doc | GCC-status |
| So m157 needs libstdc++ headers from GCC 10 or later; GCC 9's cannot compile it. With clang and GCC 10 to 14 headers it should build | inf | the two rows above; not compiled |
| Supporting evidence: skia-pack built m144 with `gcc-10` and `g++-10` on Linux x64 and arm64; rust-skia's Linux binaries ask for glibc 2.31 and libstdc++ `GLIBCXX_3.4.28` at least, the version GCC 9.3 and 10.1 ship | src + doc | `sp:script/build.py#L73-L87`, `rs:README.md#L68`, GCC-abi |

What an ES 2.0 context without extensions loses in Ganesh
(`sk:src/gpu/ganesh/gl/GrGLCaps.cpp`):

| Loss | Extension that restores it | Mark | Line |
| --- | --- | --- | --- |
| MSAA render targets | `GL_EXT_multisampled_render_to_texture`, `GL_IMG_multisampled_render_to_texture` | src | `#L1167-L1184` |
| Mipmaps, and repeat or mirror tiling of non-power-of-two textures ("our caps don't allow for limited MIP support") | `GL_OES_texture_npot` | src | `#L649-L657` |
| Shader derivatives, so `FillRRectOp`, the AA convex and hairline path renderers and the distance-field small path renderer are unavailable and other ops or CPU masks take over | `GL_OES_standard_derivatives` | src | `#L455-L457`; `sk:src/gpu/ganesh/ops/FillRRectOp.cpp#L959`, `AAConvexPathRenderer.cpp#L727`, `AAHairLinePathRenderer.cpp#L1389`, `SmallPathRenderer.cpp#L717` |
| Advanced blend equations in the fixed-function blender (`mixBlendMode` beyond Porter-Duff then needs a copy of the destination) | `GL_KHR_blend_equation_advanced` | src + inf | `#L1195-L1217` |
| Dual-source blending, integer shader types, explicit texture LOD, instancing, vertex array objects, buffer mapping, sampler objects, stencil formats other than `STENCIL_INDEX8` | `GL_EXT_blend_func_extended`, `GL_EXT_draw_instanced` + `GL_EXT_instanced_arrays`, `GL_OES_vertex_array_object`, `GL_OES_packed_depth_stencil` | src | `#L243-L244`, `#L286-L288`, `#L453-L466`, `#L572`, `#L869`, `#L1246-L1253` |
| Program binaries for the persistent shader cache | `GL_OES_get_program_binary` | src | `#L850` |
| BGRA textures. Skia's native 32-bit order on Linux is BGRA (`SK_R32_SHIFT=16`), so pixels meant for upload are better decoded as `kRGBA_8888_SkColorType` | `GL_EXT_texture_format_BGRA8888` | src + inf | `#L2040-L2052`, `sk:BUILD.gn#L32-L34` |

## 2. The build system and its inputs

| Fact | Mark | Source |
| --- | --- | --- |
| "Skia uses GN to configure its builds", then Ninja. Build files call `python3` (`script_executable`, the `alink` and `copy` tools) | doc + src | SK-build; `sk:.gn`, `sk:gn/toolchain/BUILD.gn#L290-L298`, `#L396-L400` |
| `gn/gn_to_cmake.py` exists (`gn gen out/config --ide=json --json-ide-script=../../gn/gn_to_cmake.py`) but "is not meant for any purpose beyond development" | doc | SK-build, `sk:site/docs/user/build.md#L463-L470` |
| Skia "is planning to migrate to Bazel in the coming quarters, making GN maintenance temporary". The `.gni` source lists are already generated from the Bazel files | src | `sk:gn/skia.gni#L259-L261`, `sk:modules/skunicode/skunicode.gni#L1-L9` |
| Nobody read here ships a hand-written CMake for Skia: rust-skia, skia-pack and react-native-skia all run `gn gen` then `ninja`; vcpkg runs GN too and swaps the third-party `BUILD.gn` files for pkg-config ones | src | `rs:skia-bindings/build_support/skia/config.rs#L352-L358`, `#L377-L385`; `sp:script/build.py#L153-L155`; `vcpkg:` (`declare_external_from_pkgconfig`, `vcpkg_gn_configure`) |
| `tools/git-sync-deps` clones every string entry of `DEPS` (about 50 repositories, `--depth=1` by default) into the path it names, then runs `bin/fetch-gn` and `bin/activate-emsdk` (`GIT_SYNC_DEPS_SKIP_EMSDK` skips the last) | src | `sk:tools/git-sync-deps#L116-L170`, `#L257-L275`; `sk:DEPS#L34-L88` |
| The build reads dependencies at fixed relative paths, `third_party/externals/<name>`. No GN argument moves them | src | `sk:third_party/freetype2/BUILD.gn#L52`, `sk:third_party/libpng/BUILD.gn#L22` |
| rust-skia's fork trims `DEPS` to brotli, d3d12allocator, expat, freetype, harfbuzz, icu, libjpeg-turbo, libpng, libwebp, vulkanmemoryallocator, spirv-cross, wuffs, zlib and buildtools, and builds GL, Vulkan, D3D, PDF, SVG and text from them | src | `https://github.com/rust-skia/skia/blob/m157.3/DEPS` |
| For the configuration of section 3 the externals are freetype, harfbuzz, libpng, libjpeg-turbo, libwebp, zlib, wuffs, and expat for SVG. ICU's sources are not needed with `skia_use_system_icu`; `buildtools` is not referenced by Skia's own GN files; partition_alloc is not needed with `skia_use_partition_alloc=false` (no-op headers are in the tree) | inf | each `sk:third_party/<name>/BUILD.gn`; `sk:src/partition_alloc/BUILD.gn#L8-L27`; a search of `*.gn` and `*.gni` for `buildtools` |
| `bin/fetch-gn` downloads `https://chrome-infra-packages.appspot.com/dl/gn/gn/<os>-<cpu>/+/git_revision:b2afae122eeb6ce09c52d63f67dc53fc517dbdc8`, with `<os>` in `mac`, `linux`, `windows` and `<cpu>` in `amd64`, `arm64`: a zip holding one `gn` binary | src | `sk:bin/fetch-gn#L22-L32` |
| Those zips are byte-stable. SHA-256: `linux-arm64` `376e1358ded109d955b8eb55ba2a7dec0e95ab1f8dfe0e18011e06eed78a011f`, `mac-arm64` `cbb260f2cf999f7d74da96b3828a061299c6337354dd6db642e778828576025c`, `linux-amd64` `6a3bfc53d825bccac5e5b4b7bdcc10ce9396a04a689ac684ec3308ca5c7f3d9c`, `mac-amd64` `4bea817159f060980f1ca1d0400ef6cd2e0ab0ade66bb34e91aab9c2cffc9d83` | obs | each downloaded, `linux-arm64` and `mac-arm64` twice, `mac-amd64` once |
| The `mac-arm64` build generates m157's Ninja files on macOS 27: "Made 111 targets from 57 files" for the arguments of "What this means" | obs | `runtime/cmake/Gn.cmake`, 2026-11-11 |
| Ubuntu packages GN as `generate-ninja` (universe, arm64 included): 24.04 `0.0~git20240221`, 26.04 `0.0~git20250917`. Whether those parse m157's build files is not tested | doc + unk | `https://packages.ubuntu.com/search?keywords=generate-ninja` |

Where each dependency can be fetched from, by the commit in `sk:DEPS`:

| Path under `third_party/externals/` | Commit | GitHub archive at that commit | Mark |
| --- | --- | --- | --- |
| `freetype` | `264b5fbf5b912b39f98d038bf75d39be0a73f21b` | `freetype/freetype`: yes | obs |
| `harfbuzz` | `9cb1fee51069b206effb4736e443b038d230789d` | `harfbuzz/harfbuzz`: yes | obs |
| `libpng` | `d5515b5b8be3901aac04e5bd8bd5c89f287bcd33` | `pnggroup/libpng`: yes | obs |
| `libwebp` | `845d5476a866141ba35ac133f856fa62f0b7445f` | `webmproject/libwebp`: yes | obs |
| `expat` | `6154446fccefbf3ca644894f598969113b0c7bcd` | `libexpat/libexpat`: yes | obs |
| `wuffs` | `e3f919ccfe3ef542cfc983a82146070258fb57f8` | `google/wuffs-mirror-release-c`: yes | obs |
| `libjpeg-turbo` | `e14cbfaa85529d47f9f55b0f104a579c1061f9ad` | no: Chromium's fork, `https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git` | obs |
| `zlib` | `646b7f569718921d7d4b5b8e22572ff6c76f2596` | no: Chromium's fork, `https://chromium.googlesource.com/chromium/src/third_party/zlib` | obs |

"yes" means `https://codeload.github.com/<owner>/<repo>/tar.gz/<commit>` answered
200, the URL form rust-skia downloads Skia with
(`rs:skia-bindings/build_support/binary_cache/download.rs#L243-L253`).

| Fact | Mark | Source |
| --- | --- | --- |
| A Gitiles archive (`<repo>/+archive/<commit>.tar.gz`) is not byte-stable: two downloads of zlib's gave two SHA-256 sums, and the unpacked tar streams differed too. It cannot be pinned by hash | obs | `https://chromium.googlesource.com/chromium/src/third_party/zlib/+archive/646b7f56....tar.gz`, twice |
| git can fetch one commit by id, which is what `git-sync-deps` does (`git fetch --depth=1 <repo> <commit>`), so the commit id is the checksum | src | `sk:tools/git-sync-deps#L162-L170` |
| The other way out is `skia_use_system_zlib` and `skia_use_system_libjpeg_turbo` with upstream releases built outside GN; vcpkg does this for zlib, libjpeg, libpng, libwebp, expat, FreeType, HarfBuzz and ICU | src | `sk:third_party/zlib/BUILD.gn#L9-L12`, `sk:third_party/libjpeg-turbo/BUILD.gn#L14-L17`, `vcpkg:` |

## 3. GN arguments

`is_official_build=true` is what "most users of Skia should set": optimized, no
debug symbols, and every `skia_use_system_*` on, so each one is turned off
again by name (`sk:site/docs/user/build.md#L14-L36`). All rows are `src`.

| Argument | Default | What it does | Line |
| --- | --- | --- | --- |
| `is_official_build` | `false` | `is_debug=false`; drops `-g`, the warning configs, tracing and every dev tool (`skia_enable_tools`, Lua, gainmaps follow `is_skia_dev_build`) | `sk:gn/BUILDCONFIG.gn#L11`, `#L43`, `#L228-L230`; `sk:gn/skia.gni#L9`, `#L34-L36` |
| `is_component_build` | `false` | `true` makes shared libraries. When `false`, each component is a `complete_static_lib` | `sk:gn/BUILDCONFIG.gn#L12`, `#L255-L260` |
| `target_os`, `target_cpu` | host | `target_os` defaults to the host's, so a build on macOS for a TV must say `"linux"`. `target_cpu`: `"arm"`, `"arm64"`, `"x64"` | `sk:gn/BUILDCONFIG.gn#L56-L95` |
| `cc`, `cxx`, `ar` | `cc`, `c++`, `ar` | Compiler and archiver commands. `is_clang` is true for `clang` and `clang++`, otherwise a script asks the compiler | `sk:gn/BUILDCONFIG.gn#L21-L23`, `#L97-L106` |
| `cc_wrapper` | `""` | Prefix of every compile, such as `ccache` | `sk:gn/toolchain/BUILD.gn#L10`, `#L243` |
| `extra_cflags`, `extra_cflags_c`, `extra_cflags_cc`, `extra_asmflags`, `extra_ldflags` | `[]` | Appended last, after Skia's own flags, to the target toolchain only. `extra_cflags_cc=["-frtti"]` is the documented way to get RTTI | `sk:gn/skia/BUILD.gn#L12-L22`, `#L697-L706`; `sk:gn/BUILDCONFIG.gn#L213-L234`; `sk:site/docs/user/build.md#L110` |
| `werror` | `false` | `-Werror` | `sk:gn/skia/BUILD.gn#L20`, `#L424-L430` |
| `skia_enable_optimize_size` | `false` | `-Oz` and `SK_ENABLE_OPTIMIZE_SIZE` instead of `-O3` | `sk:gn/skia/BUILD.gn#L760-L772`, `sk:BUILD.gn#L35-L37` |
| `skia_use_partition_alloc` | `is_skia_standalone && is_clang` | Chrome's allocator and `raw_ptr`; needs `third_party/externals/partition_alloc`. rust-skia turns it off "since m151", react-native-skia since m152 | `sk:gn/skia.gni#L126-L135`; `rs:.../config.rs#L123-L124`; `rns:.../skia-configuration.ts#L80-L85` |
| `skia_enable_ganesh`, `skia_enable_graphite` | `true`, `false` | The two GPU back ends | `sk:gn/skia.gni#L112-L113` |
| `skia_use_gl` | `true` | Ganesh's GL backend, defines `SK_GL` | `sk:gn/skia.gni#L65`, `sk:BUILD.gn#L982-L983` |
| `skia_use_egl` | `false` | Compiles the EGL native interface (`GrGLInterfaces::MakeEGL`, includes `<EGL/egl.h>` and `<GLES2/gl2.h>`) and names `EGL` and `GLESv2` as link libraries, in place of GLX | `sk:BUILD.gn#L995-L1005`, `#L2229-L2234`; `sk:src/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.cpp#L12-L30` |
| `skia_gl_standard` | `""` off Apple | `"gles"` defines `SK_ASSUME_GL_ES=1`: the GL standard becomes a constant and the desktop GL branches are optimized away | `sk:gn/skia.gni#L138-L147`, `sk:BUILD.gn#L47-L49`, `sk:include/gpu/ganesh/gl/GrGLTypes.h#L29`, `sk:src/gpu/ganesh/gl/GrGLCaps.cpp#L133` |
| `skia_use_x11` | `is_linux` | GLX interface and `libGL` when EGL is not chosen | `sk:gn/skia.gni#L105`, `sk:BUILD.gn#L1008-L1013` |
| `skia_use_vulkan`, `skia_use_dawn`, `skia_use_angle` | `false` | Other backends | `sk:gn/skia.gni#L48-L52`, `#L149` |
| `skia_use_freetype`, `skia_use_system_freetype2` | `is_linux` etc., system when official | FreeType font host; bundled FreeType depends on libpng and uses its own zlib unless `skia_use_system_zlib` | `sk:gn/skia.gni#L62`, `sk:third_party/freetype2/BUILD.gn#L9-L21`, `#L36`, `#L52-L60` |
| `skia_use_fontconfig` | `is_linux` | `SkFontMgr_New_FontConfig`, needs fontconfig's headers and library | `sk:gn/skia.gni#L59`, `#L175-L178` |
| `skia_enable_fontmgr_custom_directory`, `_custom_empty`, `_custom_embedded`, `skia_enable_fontmgr_android` | follow `skia_use_freetype` (and `skia_use_expat`) | `SkFontMgr_New_Custom_Directory`, `..._Custom_Empty`, embedded fonts, Android's `fonts.xml` manager | `sk:gn/skia.gni#L168-L175`, `sk:BUILD.gn#L334-L383` |
| `skia_use_harfbuzz`, `skia_use_system_harfbuzz` | `true`, system when official | HarfBuzz for SkShaper. The bundled build has no ICU and no FreeType dependency (`HAVE_OT` only) | `sk:gn/skia.gni#L64`, `sk:third_party/harfbuzz/BUILD.gn#L9-L36` |
| `skia_pdf_subset_harfbuzz` | `skia_use_harfbuzz` | Makes PDF depend on HarfBuzz's subsetter; nothing without PDF | `sk:gn/skia.gni#L164-L166`, `sk:BUILD.gn#L1277-L1280` |
| `skia_enable_skshaper`, `skia_enable_skparagraph` | `true` | The modules. SkParagraph exists only with SkShaper, a SkUnicode backend and HarfBuzz | `sk:modules/skshaper/skshaper.gni#L48`, `sk:modules/skparagraph/BUILD.gn#L8-L16` |
| `skia_use_icu`, `skia_use_system_icu` | `true`, system when official | The ICU backend of SkUnicode; system means "headers on the include path, `-licuuc`" | `sk:gn/skia.gni#L67`, `sk:third_party/icu/icu.gni#L9`, `sk:third_party/icu/BUILD.gn#L10-L14` |
| `skia_use_client_icu`, `skia_use_libgrapheme`, `skia_use_icu4x`, `skia_use_bidi`, `skia_use_runtime_icu` | `false` | The other SkUnicode backends, section 5 | `sk:gn/skia.gni#L49-L50`, `#L68-L71`, `sk:modules/skunicode/BUILD.gn#L10-L12` |
| `skia_use_libpng_decode`, `_encode`, `skia_use_libjpeg_turbo_decode`, `_encode`, `skia_use_libwebp_decode`, `_encode`, and `skia_use_system_libpng`, `_libjpeg_turbo`, `_libwebp` | `true`; system when official | Codecs. `skia_use_no_jpeg_encode`, `_png_`, `_webp_` compile stubs for a disabled encoder | `sk:gn/skia.gni#L72-L81`, `sk:BUILD.gn#L2181-L2189` |
| `skia_use_wuffs` | `true` | GIF decoding (`SK_CODEC_DECODES_GIF`) | `sk:BUILD.gn#L1993-L2005` |
| `skia_use_zlib`, `skia_use_system_zlib` | `true`, system when official | zlib for libpng (and PDF) | `sk:gn/skia.gni#L114`, `sk:third_party/zlib/zlib.gni#L7` |
| `skia_use_expat`, `skia_use_system_expat` | `true`, system when official | XML: SVG and the Android font manager | `sk:gn/skia.gni#L56`, `sk:BUILD.gn#L2007-L2018` |
| `skia_enable_svg` | `!is_component_build` | `modules/svg`, needs expat, SkShaper, SkUnicode, skresources | `sk:gn/skia.gni#L33`, `sk:modules/svg/BUILD.gn#L8-L28` |
| `skia_enable_pdf`, `skia_enable_skottie` | `true` | PDF backend, Lottie player | `sk:gn/skia.gni#L30-L32` |
| `skia_use_dng_sdk`, `skia_use_piex` | on off Windows | The RAW codec, which needs two more externals and is "the one Skia source built with exceptions" | `sk:gn/skia.gni#L91`, `#L180`, `sk:BUILD.gn#L1357-L1369`; `rns:.../skia-configuration.ts#L61-L66` |
| `skia_use_perfetto`, `skia_use_lua`, `skia_use_xps`, `skia_use_libavif`, `skia_use_libjxl_decode` | mixed | Tracing, tools and codecs with their own externals | `sk:gn/skia.gni#L70-L106` |

| Fact | Mark | Source |
| --- | --- | --- |
| A complete static library holds the code of everything it depends on, except other complete static libraries. So `libskia.a` carries FreeType, libpng, libjpeg-turbo, libwebp, zlib, expat and wuffs, and `libskshaper.a` carries HarfBuzz | doc + inf | GN-ref, `complete_static_lib`; `sk:BUILD.gn#L2077-L2110`, `sk:modules/skshaper/BUILD.gn#L51-L57` |
| Ninja targets and outputs: `skia`, `skparagraph`, `skshaper`, `skunicode_core`, `skunicode_icu`, `svg`, `skresources` give `lib<name>.a` in the output directory | src | `rs:skia-bindings/build_support/binaries_config.rs#L20-L28`, `#L70-L101`; `sk:gn/toolchain/BUILD.gn#L300-L307` |
| Code that includes Skia's headers must be compiled with Skia's public defines (`SK_GANESH`, `SK_GL`, `SK_ASSUME_GL_ES=1`, `SK_R32_SHIFT=16`, the `SK_CODEC_DECODES_*` set, ...) and with the Skia root as include directory. rust-skia reads them from the `defines =` line of `obj/skia.ninja`, `obj/gpu.ninja` and the modules' ninja files | src | `sk:BUILD.gn#L21-L136`; `rs:skia-bindings/build_support/skia_bindgen.rs#L964-L1015` |
| Skia is built without RTTI, so a host class compiled with RTTI that derives from a Skia class does not link. skia-pack (macOS) and react-native-skia (every target) add `-frtti` | src + inf | `sk:gn/skia/BUILD.gn#L732-L743`; `sp:script/build.py#L50`; `rns:.../skia-configuration.ts#L182-L186` |

## 4. Cross-compiling for armv7, building on aarch64

| Fact | Mark | Source |
| --- | --- | --- |
| For `target_cpu="arm"` GN adds `-march=armv7-a -mfpu=neon -mthumb`. It sets no float ABI | src | `sk:gn/skia/BUILD.gn#L135-L140` |
| 32-bit ARM is a live target in m157: `SK_CPU_ARM32`, a NEON raster pipeline for it, NEON sources in libpng, libjpeg-turbo and zlib chosen for `current_cpu == "arm"` | src | `sk:include/private/SkFeatures.h#L161-L170`, `sk:src/opts/SkRasterPipeline_opts.h#L82`, `#L1754`; `sk:third_party/libpng/BUILD.gn#L45-L49`, `sk:third_party/libjpeg-turbo/BUILD.gn#L159-L185`, `sk:third_party/zlib/BUILD.gn#L32-L33` |
| NEON is assumed, not probed: the third-party NEON files are compiled whatever the flags, so a SoC without NEON cannot run the build | inf | the two rows above |
| Skia documents a clang cross build for 32-bit ARM Linux with EGL (its ChromeOS recipe): `target_cpu="arm"`, `--target=armv7a-linux-gnueabihf` and `--sysroot=` in `extra_cflags`, `extra_asmflags` and `extra_ldflags`, `-DMESA_EGL_NO_X11_HEADERS`, `skia_use_fontconfig=false`, `skia_use_system_freetype2=false`, `skia_use_egl=true` | doc | SK-build, `sk:site/docs/user/build.md#L186-L250` |
| rust-skia cross-compiles the same way: `--target=<triple>` in `extra_cflags` and `extra_asmflags`, `--sysroot=` from `SDKTARGETSYSROOT` (the variable a Yocto SDK sets), compilers from `CLANGCC` and `CLANGCXX`. It sets no `extra_ldflags`: only archives are built | src | `rs:skia-bindings/build_support/platform.rs#L170-L203`; `rs:.../skia/config.rs#L43-L50`, `#L111-L114`, `#L238-L240` |
| `host_cc` and `host_cxx` default to `cc` and `cxx`, and `extra_*` flags do not reach the host toolchain. The configuration of section 3 compiles nothing for the host (no SkSL compiler, no libgrapheme generators) | src + inf | `sk:gn/toolchain/BUILD.gn#L5-L9`, `sk:gn/skia/BUILD.gn#L697-L706`, `sk:gn/skia.gni#L19`, `sk:third_party/libgrapheme/BUILD.gn#L21-L23` |
| On aarch64 Linux a native build is `target_cpu="arm64"` with `cc="clang" cxx="clang++"`. skia-pack's arm64 Linux build adds only `skia_gl_standard="gles"` and `skia_use_egl=true` | src | `sp:script/build.py#L74-L81` |
| The simulator VM is Ubuntu 26.04 aarch64 with clang, lld, ninja, python3 and ccache already installed | src | `simulator/lima.yaml:8-11`, `simulator/guest/provision.sh:12-18` |

## 5. SkUnicode and ICU

| Fact | Mark | Source |
| --- | --- | --- |
| SkParagraph and SkShaper ask SkUnicode for bidi levels and visual reordering, line breaks (soft and hard), grapheme starts, word boundaries, and per-code-point classes: whitespace, control, tab, ideographic, emoji | src | `sk:modules/skunicode/include/SkUnicode.h#L86-L100`, `#L130-L192`, `#L314` |
| ICU backend (`SkUnicodes::ICU::Make`): calls 30 ICU C functions, all in ICU's common library (`ubrk_*`, `ubidi_*`, `uloc_*`, `uscript_getScript`, `utext_*`, `u_hasBinaryProperty`, ...). "Skia avoids the ICU C++ API" | src | `sk:modules/skunicode/src/SkUnicode_icupriv.h#L21-L51`, `sk:third_party/icu/config/BUILD.gn#L6-L12` |
| Client backend (`skia_use_client_icu`): not "your own ICU data". `SkUnicodes::Client::Make(text, words, graphemeBreaks, lineBreaks)` takes break positions the client computed for one text; only bidi comes from a renamed ICU subset built in | src | `sk:modules/skunicode/include/SkUnicode_client.h`, `sk:modules/skunicode/BUILD.gn#L93-L124` |
| libgrapheme backend: breaks from libgrapheme, generated on the host from Unicode 15.0.0 data files; react-native-skia uses it on Apple platforms | src | `sk:third_party/libgrapheme/BUILD.gn#L8-L50`; `rns:.../skia-configuration.ts#L39-L46` |
| ICU4X backend: needs Rust (and Bazel, per SK-build). Bidi-only backend (`skia_use_bidi`): no line breaking | src + doc | `sk:modules/skunicode/BUILD.gn#L126-L207`, `sk:site/docs/user/build.md#L38-L43`, `sk:modules/skunicode/include/SkUnicode_bidi.h` |
| Runtime ICU (`skia_use_runtime_icu`, Android and Linux): `dlopen("libicuuc.so")` and a search for the versioned symbols; react-native-skia uses it on Android | src | `sk:modules/skunicode/BUILD.gn#L81-L89`, `sk:modules/skunicode/src/SkUnicode_icu_runtime.cpp#L14-L41`; `rns:.../skia-configuration.ts#L31-L37` |
| Skia's bundled ICU is version 78 too, compiled with `U_DISABLE_RENAMING=1`, its whole data file linked in as assembly | src | `sk:third_party/icu/BUILD.gn#L16-L57`, `#L73-L82` |
| With `skia_use_system_icu=true` the ICU backend is compiled without `U_DISABLE_RENAMING`, against whatever `<unicode/*.h>` is on the include path. rust-skia and vcpkg build this way | src | `sk:modules/skunicode/BUILD.gn#L70-L75`; `rs:.../config.rs#L191-L192`; `vcpkg:` (`skia_use_icu=true skia_use_system_icu=true`) |
| The runtime's ICU 78.3 is static, hidden, and keeps ICU's default renaming (`U_ICU_VERSION_SUFFIX _78`): `Icu.cmake` defines no `U_DISABLE_RENAMING`. Its public headers are copied to `${CMAKE_BINARY_DIR}/icu/include` at configure time | src | `runtime/cmake/Icu.cmake:14-36`; `icu:common/unicode/uvernum.h:82`, `icu:common/unicode/urename.h:34` |
| So SkUnicode compiled against those headers references `ubrk_open_78` and the like, which the runtime's `rn_tizen_icuuc` archive defines; hidden visibility does not matter inside one shared object. ICU's i18n library is not needed | inf | the rows above |
| A break iterator opens `brkitr/<locale>.res`, reads its `boundaries` table, then opens the `.brk` rule file it names | src | `icu:common/brkiter.cpp:78-113` |
| Emoji properties (`UCHAR_EMOJI` ...) load the item `uemoji.icu` | src | `icu:common/emojiprops.cpp:84`, `icu:common/uprops.cpp:465-472` |
| The app's trimmed `icudt78l.dat` keeps only the trees `''`, `coll`, `curr` and `zone`: the whole `brkitr` tree is dropped today, on purpose ("break iteration [...] are what Hermes never asks ICU for"). `uemoji.icu` and `ulayout.icu` are in the root tree and stay | src | `src/runtime/icu-data.ts:17-25`, `:41-59` |
| `brkitr` in ICU 78.3's data is 3.8 MB of 33.1 MB. Its main items: `root.res` 800 B, `res_index.res` 240 B, `char.brk` 14 KB, `word.brk` 23 KB, `line.brk` 73 KB, eight more `line_*.brk` of 73 to 86 KB, `sent.brk` 20 KB, `cjdict.dict` 2.0 MB, `khmerdict.dict` 446 KB, `burmesedict.dict` 254 KB, `laodict.dict` 163 KB, `thaidict.dict` 126 KB, and 14 locale files under 3 KB each | obs | table of contents of `~/.cache/react-native-tizen/icu/78.3/icudt78l.dat` |
| `brkitr/root.res` names, under `boundaries`, `char.brk` (grapheme), `line.brk`, `line_loose.brk`, `line_normal.brk`, `sent.brk`, `title.brk` and `word.brk`, and under `dictionaries` one file per script: `Hani`, `Hira` and `Kana` to `cjdict.dict`, `Khmr`, `Laoo`, `Mymr` and `Thai` to the dictionary named after each | obs | the `.brk` and `.dict` names in the UTF-16 strings of each `brkitr` item of ICU 78.3's `icudt78l.dat`, read with `src/runtime/icu-common-data.ts`, 2026-11-11 |
| Fourteen locales have a `brkitr` item (`brkitr/res_index.res`): `de`, `el`, `en`, `en_US`, `en_US_POSIX`, `es`, `fr`, `it`, `ja`, `ko`, `pt`, `ru`, `zh`, `zh_Hant`. Of those only five name rule files: `ja` and `ko` the `line_*cj.brk` and `line_*phrase_cj.brk` files and `line_normal.brk`, `zh` and `zh_Hant` `line_cj.brk`, `line_loose_cj.brk` and `line_normal_cj.brk`, `el` `sent_el.brk`, `en_US_POSIX` `word_POSIX.brk`. The others hold sentence break exceptions only. There is no `word_ja.brk` | obs | the same reading |
| "ICU provides dictionary support for word boundaries in Chinese, Japanese, Thai, Lao, Khmer and Burmese"; the word lists serve "word break, and for some languages, line break" | doc | `https://unicode-org.github.io/icu/userguide/boundaryanalysis/` |
| So which dictionary a language needs follows from the two rows above: `cjdict.dict` for `ja` and `zh`, `thaidict.dict` for `th`, `laodict.dict` for `lo`, `khmerdict.dict` for `km`, `burmesedict.dict` for `my`. Korean is not in ICU's list, and Hangul has no entry in `dictionaries` | inf | the script each language is written in, against the `dictionaries` table |
| Trimmed as `src/runtime/icu-data.ts` trims it (root's `char.brk`, `word.brk` and `line.brk`, the app languages' `brkitr` items, the rule files those name, and their dictionaries), `icudt78l.dat` is 2.56 MiB for `en` (134 KB of it `brkitr`), 2.66 MiB for `en` and `fr`, 2.60 MiB for `th`, 5.04 MiB for `ja` (2.6 MB of `brkitr`, 2.0 MB of that `cjdict.dict`) | obs | `trimIcuData` on ICU 78.3's data, 2026-11-11 |
| Without break data the failure is silent: `computeCodeUnitFlags` ignores what the line and grapheme passes return and reports success, so a paragraph gets no soft break and no grapheme start | src | `sk:modules/skunicode/src/SkUnicode_icu.cpp#L372-L387`, `#L574-L622` |
| The ICU backend's break iterator cache is one process-wide object behind a mutex; iterators are cloned per use | src | `sk:modules/skunicode/src/SkUnicode_icu.cpp#L148-L332` |

## 6. SkParagraph and React Native `<Text>`

`skia::textlayout`, all `src` in `sk:modules/skparagraph/`:

| Need | What SkParagraph has | Line |
| --- | --- | --- |
| Nested styles | `ParagraphBuilder::make(style, fontCollection, unicode)`, then `pushStyle`, `addText`, `pop`, `Build` | `include/ParagraphBuilder.h#L28-L59`, `#L87-L89` |
| Weight, slant, families | `TextStyle::setFontStyle(SkFontStyle)`, `setFontFamilies(vector)`: every family is tried in order, then the collection's default families, then per-character fallback | `include/TextStyle.h#L228`, `#L253`; `src/FontCollection.cpp#L139-L225` |
| Synthetic bold and italic | `ParagraphStyle::setFakeMissingFontStyles`, on by default | `include/ParagraphStyle.h#L125-L126`, `src/ParagraphStyle.cpp#L30` |
| Line height | `setHeight(multiple)` with `setHeightOverride(true)`: a run is `multiple x fontSize` high. `setHalfLeading(true)` splits the extra space above and below; otherwise ascent and descent are scaled | `include/TextStyle.h#L260-L267`, `src/Run.cpp#L64-L84` |
| First and last line | `TextHeightBehavior`: `kDisableFirstAscent`, `kDisableLastDescent`. Strut: `StrutStyle` with `setForceStrutHeight` | `include/DartTypes.h#L136-L141`, `include/ParagraphStyle.h#L21-L76` |
| Letter and word spacing | `setLetterSpacing`, `setWordSpacing` | `include/TextStyle.h#L269-L273` |
| Alignment | `TextAlign`: `kLeft kRight kCenter kJustify kStart kEnd`; `TextDirection` | `include/DartTypes.h#L52-L64` |
| Max lines and ellipsis | `setMaxLines(n)`, `setEllipsis(string)`. The ellipsis string goes at the end of the last line; with an ellipsis and unlimited lines the first line is the last. No head or middle mode exists | `include/ParagraphStyle.h#L103-L109`, `src/TextWrapper.cpp#L292-L312`, `src/TextLine.h#L116` |
| Decoration | underline, overline, line-through, combinable; style `kSolid kDouble kDotted kDashed kWavy`; colour; thickness multiplier | `include/TextStyle.h#L41-L56`, `#L220-L224` |
| Shadows | `addShadow(TextShadow{color, offset, blurSigma})`, several per style | `include/TextShadow.h#L13-L15`, `include/TextStyle.h#L231-L233` |
| Inline views | `addPlaceholder(PlaceholderStyle{width, height, alignment, baseline, offset})`; boxes back from `getRectsForPlaceholders()` | `include/TextStyle.h#L87-L130`, `include/Paragraph.h#L55-L57` |
| Measuring | `layout(width)`, then `getHeight`, `getLongestLine`, `getMaxIntrinsicWidth`, `getMinIntrinsicWidth`, `getAlphabeticBaseline`, `didExceedMaxLines`, `getLineMetrics`, `lineNumber` | `include/Paragraph.h#L26-L42`, `#L67-L69` |
| Selection and caret | `getRectsForRange(start, end, heightStyle, widthStyle)`, `getGlyphPositionAtCoordinate(x, y)`, `getWordBoundary`, `getLineNumberAtUTF16Offset`, `getGlyphInfoAtUTF16Offset`, `getClosestUTF16GlyphInfoAt` | `include/Paragraph.h#L50-L65`, `#L167`, `#L229-L243` |
| Cheap restyle | `updateTextAlign`, `updateFontSize`, `updateForegroundPaint`, `updateBackgroundPaint`, marked experimental | `include/Paragraph.h#L78-L83` |
| Font scaling | No scale factor in the API: the host multiplies font sizes | inf: no such member in `include/TextStyle.h` or `include/ParagraphStyle.h` |
| App fonts | `TypefaceFontProvider::registerTypeface(typeface, alias)`, set with `FontCollection::setAssetFontManager` | `include/TypefaceFontProvider.h#L37-L40`, `include/FontCollection.h#L27-L32` |

| Fact | Mark | Source |
| --- | --- | --- |
| `SkFontMgr_New_Custom_Directory(dir)` scans `dir` and its subdirectories for `.ttf`, `.ttc`, `.otf` and `.pfb` at construction, every face and named instance. It matches a family by exact name, and its `onMatchFamilyStyleCharacter` returns `nullptr`: no per-character fallback | src | `sk:include/ports/SkFontMgr_directory.h`, `sk:src/ports/SkFontMgr_custom_directory.cpp#L17-L110`, `sk:src/ports/SkFontMgr_custom.cpp#L175-L197` |
| The fontconfig manager does fall back: `FcFontMatch` on a pattern with the character and the languages | src | `sk:src/ports/SkFontMgr_fontconfig.cpp#L834-L874` |
| SkParagraph's last resort for an unresolved character is `matchFamilyStyleCharacter` on each font manager, so with the directory manager a script outside the listed families draws no glyph | src + inf | `sk:modules/skparagraph/src/FontCollection.cpp#L202-L225` |
| `SkFontMgr::RefDefault` is gone: a font manager is always passed in. The Nagra port, on m108, still used it | src | `sk:include/core/SkFontMgr.h#L149`; `nag:ReactSkia/textlayoutmanager/RSkTextLayoutManager.cpp#L88-L89` |
| `FontCollection` is not thread-safe: `findTypefaces` writes its face cache with no lock. `ParagraphCache` inside it has a mutex | src | `sk:modules/skparagraph/src/FontCollection.cpp#L20-L51`, `#L181`; `sk:modules/skparagraph/include/ParagraphCache.h#L41`, `sk:modules/skparagraph/src/ParagraphCache.cpp#L322-L363` |
| "Typeface objects are immutable, and so they can be shared between threads"; FreeType calls are serialized by one mutex; `SkTextBlob` and a finished `SkPicture` are immutable | src | `sk:include/core/SkTypeface.h#L52`, `sk:src/ports/SkFontHost_FreeType.cpp#L221-L222`, `sk:include/core/SkTextBlob.h#L38`, `sk:include/core/SkPicture.h#L36-L46` |
| So a paragraph is built, laid out and recorded on one thread at a time (a lock around the collection, or one text thread), and what crosses to the drawing thread is the recorded picture | inf | the three rows above |
| The Nagra port measures with `layout(maxWidth)`, then `min(getMaxIntrinsicWidth, getMaxWidth)` and `getHeight` | src | `nag:ReactSkia/textlayoutmanager/RSkTextLayoutManager.cpp#L100-L108` |

## 7. Performance design on weak GPUs

| Fact | Mark | Source |
| --- | --- | --- |
| A Ganesh context is "matched one to one" with a GL context, and Skia "assumes that the correct OpenGL context has been made current to the current thread when Skia calls are made" | doc | `https://skia.org/docs/user/api/skcanvas_creation/`, `sk:site/docs/user/api/skcanvas_creation.md#L69-L76` |
| Nagra: every component records its drawing into an `SkPicture` held by a layer; a frame walks the layer tree, applies each layer's matrix, plays its picture, clips children to the frame when `masksToBounds` | src | `nag:ReactSkia/components/RSkComponent.cpp#L74-L97`, `#L289-L290`; `nag:rns_shell/compositor/layers/PictureLayer.cpp#L22-L30`, `Layer.cpp#L259-L282` |
| Flutter's raster cache turns a display list into a texture only when it is not marked `will_change`, its complexity score says so, it has been seen on more than 3 frames while visible, and fewer than 3 caches were made this frame ("Generating too many caches in one frame may cause jank"). Cached layers are snapped to physical pixels | src | `fl:flow/layers/display_list_raster_cache_item.cc#L24-L49`, `#L103-L124`; `fl:flow/raster_cache.h#L141-L143`, `#L217-L222`; `fl:flow/raster_cache_util.h#L17-L21`, `#L65-L76` |
| `saveLayer` "allocates a SkSurface for subsequent drawing", drawn back on `restore` | doc | `sk:include/core/SkCanvas.h#L603-L604` |
| Nagra pays that for every layer with opacity below 1 (`saveLayerAlpha`) | src | `nag:rns_shell/compositor/layers/Layer.cpp#L374-L378` |
| Flutter avoids it: an opacity layer hands its alpha to children that can apply it themselves, and otherwise draws a cached raster of the children. Skia's picture optimizer folds SaveLayer, one draw, Restore into a draw with alpha | src | `fl:flow/layers/opacity_layer.cc#L36-L96`; `sk:src/core/SkRecordOpts.h#L19-L22` |
| Ganesh clips: an axis-aligned rectangle becomes a scissor. A rectangle or rounded rectangle under a translate-and-scale matrix becomes a coverage term in the draw's shader, for equal corners, per-corner circular "tab" shapes and nine-patch ellipses, up to 4 per draw. Anything else (rotated, mixed radii) is a CPU-rasterized mask texture when anti-aliased without MSAA, or the stencil buffer | src | `sk:src/gpu/ganesh/ClipStack.cpp#L256-L266`, `#L597-L637`, `#L1182-L1185`, `#L1522-L1549`; `sk:src/gpu/ganesh/effects/GrRRectEffect.cpp#L715-L812` |
| Flutter's GL settings: `fAvoidStencilBuffers = true` ("Using stencil buffers has caused memory and performance regressions"), a persistent shader cache, the window framebuffer wrapped with 0 samples and 0 stencil bits, resource cache 24 MiB | src | `fl:shell/common/context_options.cc#L14-L44`; `fl:shell/gpu/gpu_surface_gl_skia.cc#L40`, `#L54-L61`, `#L143-L146` |
| Ganesh's own defaults: resource cache 256 MiB (`setResourceCacheLimit`, `purgeUnlockedResources`, `performDeferredCleanup` change and trim it), glyph atlas 8 MB in pages of at most 2048 px, distance-field text from 18 px, 4x internal MSAA where there is MSAA, 256 cached programs, program binaries cached through `fPersistentCache` | src | `sk:src/gpu/ganesh/GrResourceCache.h#L89`; `sk:include/gpu/ganesh/GrDirectContext.h#L199-L263`; `sk:include/gpu/ganesh/GrContextOptions.h#L98`, `#L115`, `#L150`, `#L166`, `#L174`, `#L191`; `sk:src/gpu/ganesh/GrDrawOpAtlas.h#L291` |
| A blurred rounded rectangle (`SkMaskFilter::MakeBlur` on `drawRRect`) with equal circular corners is drawn from a cached mask texture keyed by blur sigma and corner radii, not by rendering and blurring each time | src | `sk:src/gpu/ganesh/GrBlurUtils.cpp#L729-L748`, `#L833-L885`, `#L922-L946` |
| `SkShadowUtils::DrawShadow` (ambient and spot shadow of an elevated shape) "may be cached, depending on the path type and canvas matrix". Nagra draws shadows through `saveLayer` and a `clipRRect` difference | doc + src | `sk:include/utils/SkShadowUtils.h#L40-L43`; `nag:ReactSkia/views/common/RSkDrawUtils.cpp#L413-L475` |
| `SkImages::DeferredFromEncodedData` "attempts to defer decoding until the image is actually used/drawn", so on the drawing thread, and decodes again after a cache purge. `SkImages::TextureFromImage(context, image)` returns a texture-backed image, budgeted in the resource cache by default | doc + inf | `sk:include/core/SkImage.h#L86-L107`, `sk:include/gpu/ganesh/SkImageGanesh.h#L185-L206` |
| Nagra decodes with `SkImage::MakeFromEncoded` (deferred). Flutter uploads decoded images on an IO thread through a second `GrDirectContext` whose cache limit is 0: "These textures should be deleted when they are no longer referenced by an SkImage" | src | `nag:ReactSkia/components/RSkComponentImage.cpp#L140`; `fl:shell/common/shell_io_manager.cc#L16-L35` |
| `eglSwapBuffersWithDamageKHR` passes damage rectangles (origin bottom-left) "to a system compositor" as a hint; "the entire contents of the back buffer will still be swapped", so it does not allow drawing only part of a frame | doc | EGL-damage |
| `EGL_BUFFER_AGE_EXT` says how many frames old the back buffer's content is (0: undefined), which is what lets an app repaint only what changed since | doc | EGL-age |
| Nagra repaints only layers that intersect the frame's damage, clips to it, swaps with `eglSwapBuffersWithDamageEXT` or `KHR`; a buffer-age path that adds the damage of earlier frames is off by default, and an offscreen copy is the fallback | src | `nag:rns_shell/compositor/Compositor.cpp#L86-L257`, `layers/Layer.cpp#L308-L336`; `nag:rns_shell/platform/graphics/gl/egl/GLWindowContextEGL.cpp#L406-L460`; `nag:rns_shell/rns_shell.gni#L24-L25` |
| Flutter's embedder API has partial repaint (`populate_existing_damage`, frame and buffer damage in `FlutterPresentInfo`), but on Android it is switched off: "We have determined that we're not going to attempt to use partial repaint with GLES on Android", for lack of confidence in drivers | src + doc | `fl:shell/platform/embedder/embedder.h#L709-L726`, `#L794-L813`; `fl:shell/platform/android/android_egl_surface.cc#L63-L84`; `https://github.com/flutter/flutter/issues/123353` |
| "The default swap interval is 1"; 0 means "buffer swaps are not synchronized to a video frame". Nagra sets 1 | doc + src | EGL-interval; `nag:.../GLWindowContextEGL.cpp#L459` |

## 8. Sizes and build times

| Fact | Mark | Source |
| --- | --- | --- |
| react-native-skia's prebuilt armeabi-v7a archives (not stripped, before the linker drops unused sections): `libskia.a` 20.1 MB, `libskshaper.a` 7.0 MB, `libsvg.a` 7.1 MB, `libskparagraph.a` 0.71 MB, `libskunicode_icu.a` 85 KB | obs | file list of npm `react-native-skia-android@154.0.0` |
| Linked into an app on arm64, react-native-skia's `librnskia.so` "is only around 3,8 MB" inside the APK, a download increase of about 4 MB | doc | `https://github.com/Shopify/react-native-skia/blob/v2.0.0/apps/docs/docs/getting-started/bundle-size.md` |
| A whole Skia-based UI engine for 32-bit ARM: `libflutter.so` (armeabi-v7a, Flutter 3.47.9) has 8.4 MB of loadable sections, `.text` 5.3 MB and `.rodata` 2.6 MB, holding the Dart VM, Impeller, Skia, HarfBuzz, FreeType, ICU and BoringSSL | obs | `llvm-size` and `llvm-nm` on `https://storage.googleapis.com/flutter_infra_release/flutter/89c39edfdfae2949f6806e4a315c0c08522238b7/android-arm-release/artifacts.zip` |
| CanvasKit: "Using Harfbuzz/ICU substantially increases code size (4.3 MB to 6.4 MB)" | doc | `sk:modules/canvaskit/CHANGELOG.md#L1139` |
| Expected for section 3's configuration on armv7, stripped: 5 to 8 MB. An estimate until the first build measures it | inf | the four rows above |
| "Building Skia takes a lot of time", which is why rust-skia downloads binaries | doc | `rs:README.md#L63` |
| A full build (`skia` and `modules`, with PDF and Skottie) on GitHub's `ubuntu-22.04-arm` runner, checkout and dependency sync included: 15 min 53 s. Android arm64 on `ubuntu-latest`: 13 min 5 s | obs | skia-pack workflow run 22231750905, 2026-02-20, jobs "linux-arm64 (Release)" and "linux-android (Release, arm64)" |

## 9. Alternatives

**ThorVG.** MIT, "as small as approximately 170KB". Its GL engine needs "OpenGL
3.3, OpenGL ES 3.0, or a browser with WebGL2 support", so it cannot run on an
ES 2 TV; its text is "unicode, scalable TTF/OTF fonts, and multi-line text
layout", with no word on shaping or bidi (doc,
`https://github.com/thorvg/thorvg/blob/main/README.md`). Rejected: it cannot
drive the TV's GPU, and Skia rasterizes react-native-svg's paths itself.

**A hand-written GLES 2 renderer with FreeType and HarfBuzz.** Feasible and
small, but it means writing what sections 6 and 7 list: line breaking, bidi,
fallback, ellipsis, justification, placeholders, hit testing, anti-aliased
rounded clips, blur, a glyph atlas, a path rasterizer for SVG, and a software
path for the simulator. Skia's bundled HarfBuzz and FreeType are the same
libraries (inf). Kept only as the fallback if Skia fails on a TV's driver.

**EFL built for webOS.** The host's code would carry over. EFL is many libraries
under several licenses and "if linked together, the result will be effectively
be LGPL" (doc, `https://github.com/Enlightenment/efl/blob/v1.28.1/COPYING`),
which a statically linked MIT package cannot take lightly. Whether its Wayland
engine runs against webOS's compositor is unknown, and it would be one more
large library to cross-build and ship. Rejected.

**Impeller.** No longer Flutter-only: there is a "Standalone SDK", a single-header
C API with optional text layout (doc,
`fl:impeller/toolkit/interop/README.md`, `fl:impeller/README.md#L290-L299`).
But it has "no API stability guarantees", prebuilt libraries only for
`linux-arm64`, `linux-x64` and Android (no 32-bit ARM Linux), must otherwise be
built through the Flutter engine's build system, and "will likely never have a
dedicated software renderer", which the simulator and the tests need. Rejected.

**Cairo and Pango.** Cairo removed its "GL and GLES drawing" backends in 1.17.8
(doc, `https://gitlab.freedesktop.org/cairo/cairo/-/raw/master/NEWS`), so it
would draw every frame on the CPU and upload it (inf). Pango was not read.
Rejected.

## 10. Open questions

- Nothing here was compiled. The first build of each target confirms the
  argument set of section 3 and gives the stripped size.
- The GN build is declared temporary (section 2). Each milestone bump has to
  check that GN still builds, or move to Bazel.
- Which GL and EGL extensions the oldest target TV has: log `GL_VERSION`,
  `GL_RENDERER`, `GL_EXTENSIONS`, `GL_MAX_TEXTURE_SIZE` and
  `eglQueryString(EGL_EXTENSIONS)`. The ones that matter:
  `GL_OES_standard_derivatives`, `GL_OES_texture_npot`,
  `GL_OES_get_program_binary`, `GL_EXT_texture_format_BGRA8888`,
  `EGL_EXT_buffer_age`, `EGL_KHR_swap_buffers_with_damage`.
- Whether every target SoC has NEON (`/proc/cpuinfo`), since the build assumes it.
- The webOS toolchain's triple, float ABI and libstdc++ version (GCC 10 headers
  at least), and which macro keeps its `eglplatform.h` from including X11.
- Where webOS keeps its fonts, whether fontconfig is on the TV, and which
  families cover CJK, Arabic, Thai and emoji.
- How much GPU memory an app may use, hence the resource cache limit.
- Whether partial repaint pays on a TV; Flutter backed out of it on Android.
- What ICU's line breaker does with Thai, Lao, Khmer or Burmese text when the
  dictionary is not in the data file.
- Whether `eglSwapBuffers` with interval 1 blocks on webOS's compositor on top
  of a Wayland frame callback.
- Whether `chrome/m150` gets fixes for as long as Chrome's long-term-support
  channel does, which would make it a calmer pin than m157.
- Whether Ubuntu's `generate-ninja` can replace the pinned `gn` binary.

## What this means for react-native-webos

**Pin.** Skia `chrome/m157` at
`fef60e46c522b87e5802ddf0a215a763dd5e0483`: Chrome's stable milestone on
2026-11-11, C++20 like the runtime, Ganesh still on ES 2.0, 32-bit ARM with
NEON still built, and the milestone rust-skia cross-compiles today. Ganesh with
the GL backend; Graphite has no GL.

**GN arguments, both targets.**

```gn
is_official_build = true
is_component_build = false
skia_use_partition_alloc = false
skia_enable_tools = false
skia_enable_ganesh = true
skia_enable_graphite = false
skia_use_gl = true
skia_use_egl = true
skia_gl_standard = "gles"
skia_use_x11 = false
skia_use_vulkan = false
skia_use_freetype = true
skia_use_system_freetype2 = false
skia_use_fontconfig = false
skia_enable_fontmgr_custom_directory = true
skia_enable_fontmgr_custom_empty = true
skia_enable_fontmgr_custom_embedded = false
skia_enable_fontmgr_android = false
skia_use_harfbuzz = true
skia_use_system_harfbuzz = false
skia_pdf_subset_harfbuzz = false
skia_enable_skshaper = true
skia_enable_skparagraph = true
skia_use_icu = true
skia_use_system_icu = true
skia_use_libpng_decode = true
skia_use_libpng_encode = true
skia_use_system_libpng = false
skia_use_libjpeg_turbo_decode = true
skia_use_libjpeg_turbo_encode = false
skia_use_no_jpeg_encode = true
skia_use_system_libjpeg_turbo = false
skia_use_libwebp_decode = true
skia_use_libwebp_encode = false
skia_use_no_webp_encode = true
skia_use_system_libwebp = false
skia_use_wuffs = true
skia_use_system_zlib = false
skia_use_expat = true
skia_use_system_expat = false
skia_enable_svg = true
skia_enable_pdf = false
skia_enable_skottie = false
skia_use_dng_sdk = false
skia_use_piex = false
skia_use_perfetto = false
extra_cflags_cc = [ "-frtti" ]
```

Simulator, built inside the aarch64 Ubuntu VM (`<icu>` is
`${CMAKE_BINARY_DIR}/icu/include`):

```gn
target_os = "linux"
target_cpu = "arm64"
cc = "clang"
cxx = "clang++"
extra_cflags = [ "-I<icu>", "-DU_STATIC_IMPLEMENTATION", "-DMESA_EGL_NO_X11_HEADERS" ]
```

TV, cross-compiled from macOS or Linux. The target, sysroot and ABI flags are
the ones the runtime's webOS toolchain file gives its own sources:

```gn
target_os = "linux"
target_cpu = "arm"
cc = "<llvm>/clang"
cxx = "<llvm>/clang++"
ar = "<llvm>/llvm-ar"
extra_cflags = [ "--target=<triple>", "--sysroot=<sysroot>", "-mfloat-abi=softfp", "-mfpu=neon",
                 "-I<icu>", "-DU_STATIC_IMPLEMENTATION", "-DMESA_EGL_NO_X11_HEADERS" ]
extra_asmflags = [ "--target=<triple>", "--sysroot=<sysroot>", "-mfloat-abi=softfp", "-mfpu=neon" ]
```

The simulator needs Mesa's EGL and GLES headers and libraries (llvmpipe); with
`skia_use_gl = false` it would draw with the CPU rasterizer only, and the GL
path would be tested nowhere but on a TV.

**Dependencies to pin** in `runtime/dependencies.json`, each a URL and a SHA-256
taken when it is added:

| Name | URL |
| --- | --- |
| skia | `https://codeload.github.com/google/skia/tar.gz/fef60e46c522b87e5802ddf0a215a763dd5e0483` |
| freetype | `https://codeload.github.com/freetype/freetype/tar.gz/264b5fbf5b912b39f98d038bf75d39be0a73f21b` |
| harfbuzz | `https://codeload.github.com/harfbuzz/harfbuzz/tar.gz/9cb1fee51069b206effb4736e443b038d230789d` |
| libpng | `https://codeload.github.com/pnggroup/libpng/tar.gz/d5515b5b8be3901aac04e5bd8bd5c89f287bcd33` |
| libwebp | `https://codeload.github.com/webmproject/libwebp/tar.gz/845d5476a866141ba35ac133f856fa62f0b7445f` |
| expat | `https://codeload.github.com/libexpat/libexpat/tar.gz/6154446fccefbf3ca644894f598969113b0c7bcd` |
| wuffs | `https://codeload.github.com/google/wuffs-mirror-release-c/tar.gz/e3f919ccfe3ef542cfc983a82146070258fb57f8` |
| gn | `https://chrome-infra-packages.appspot.com/dl/gn/gn/<linux-arm64, mac-arm64, linux-amd64>/+/git_revision:b2afae122eeb6ce09c52d63f67dc53fc517dbdc8`, sums in section 2 |
| libjpeg-turbo | git `https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git` at `e14cbfaa85529d47f9f55b0f104a579c1061f9ad` |
| zlib | git `https://chromium.googlesource.com/chromium/src/third_party/zlib` at `646b7f569718921d7d4b5b8e22572ff6c76f2596` |

The last two have no hash-stable archive. Either `dependencies.json` learns a
git form (repository and commit, fetched with `git fetch --depth=1`), or both
are replaced by upstream releases built by the runtime's CMake and
`skia_use_system_zlib` and `skia_use_system_libjpeg_turbo`. Each library's
license goes into `THIRD_PARTY_NOTICES.md`.

**Driving it from CMake.** Not generated CMake: Skia calls `gn_to_cmake.py` a
development aid. `FetchContent` downloads Skia and the dependencies as it does
today; a step links each dependency into Skia's `third_party/externals/<name>`;
an `ExternalProject` runs `gn gen <out> --script-executable=python3 --args=...`
and `ninja -C <out> skia skparagraph skshaper skunicode_core skunicode_icu svg
skresources`, with the seven archives as `BUILD_BYPRODUCTS` of imported static
targets and `cc_wrapper="ccache"` when ccache is found. The host is compiled
with the defines read from `<out>/obj/skia.ninja` and links, in this order, the
Skia archives, `rn_tizen_icuuc`, then `EGL`, `GLESv2`, `dl` and `pthread`. One
output directory per target and per argument set, in the shared cache, so a
build costs its 10 to 15 minutes once per machine.

**ICU.** SkUnicode's ICU backend, compiled against the runtime's ICU 78.3
headers with renaming left on, linked against the runtime's own archive: no
second ICU and no second data file. The data trimmer must start keeping
`brkitr`: `root.res`, `res_index.res`, `char.brk`, `word.brk` and `line.brk`
(112 KB) for every app, the locale files of the app's languages, and the
dictionaries of the scripts it declares; `TRIM_VERSION` goes up. Without them
text silently stops wrapping.

**Text stack.** FreeType and HarfBuzz as bundled by Skia, SkShaper, SkParagraph.
Fonts: `SkFontMgr_New_Custom_Directory` over the TV's font directory as the
default manager, a `TypefaceFontProvider` for the app's fonts, and a fallback
written by the host (an ordered family list per style, or a font manager that
answers `matchFamilyStyleCharacter` from the scanned faces), because the
directory manager has none. One `FontCollection`, used under a lock: Fabric
measures on its own thread. `<Text>` maps to section 6's table; `ellipsizeMode`
`head` and `middle` are the host's to implement. A measured paragraph is kept
and painted into an `SkPicture` on the thread that laid it out.

**Rendering architecture.** One thread owns the EGL context and one
`GrDirectContext` (`GrGLInterfaces::MakeEGL`, the window framebuffer wrapped as
an `SkSurface`, no MSAA, Flutter's context options as the starting point:
`fAvoidStencilBuffers`, a persistent program cache in the app's data directory,
an explicit resource cache limit). Each view keeps an `SkPicture` of its own
content, recorded again only when its props change; a frame walks the view
tree, concatenates transforms, clips with `clipRect` or an anti-aliased
`clipRRect` (a shader term for the shapes React Native produces unrotated), and
plays the pictures. Opacity is paint alpha on views that draw one thing, and a
`saveLayer` only where children overlap; a subtree that is animated as a whole
(a focused card that scales, a row that slides) is cached in a texture once it
has been stable for a few frames, a few caches per frame at most, as Flutter
does. Shadows are blurred rounded rectangles, which Ganesh serves from a cached
mask. Images are decoded off the drawing thread to RGBA at the size they are
shown, then uploaded with `SkImages::TextureFromImage`; without
`GL_OES_texture_npot` there are no mipmaps to hide a lazy downscale. Every
dirty frame is redrawn in full and the damage is passed to the compositor as a
hint when the extension exists; repainting only the damage waits for
`EGL_EXT_buffer_age` and a measurement on a TV. The simulator runs the same
code on Mesa, and tests draw into a raster `SkSurface`.
