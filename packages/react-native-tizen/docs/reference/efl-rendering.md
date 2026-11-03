# EFL rendering on Samsung Tizen TVs

How react-native-tizen draws Fabric views with Evas, and what Evas on a Tizen 9
TV (EFL 1.25.1) can and cannot do. Every row carries a mark and a source.

| Mark | Meaning |
| --- | --- |
| doc | Stated in API documentation, a header, or a Samsung/Tizen guide |
| src | Read in the EFL 1.25.1 source or the Tizen 9 binaries |
| inf | Inferred from documented or source facts, not tested on a TV |
| unk | Unknown; the row says how to find out |

## Sources

| Key | Where |
| --- | --- |
| `efl:` | EFL v1.25.1 source, `https://github.com/Enlightenment/efl/blob/v1.25.1/<path>#L<line>` |
| `efl128:` | EFL v1.28.1 source, the simulator's EFL, `https://github.com/Enlightenment/efl/blob/v1.28.1/<path>` |
| `rs:` | Tizen 9 TV rootstrap, `~/tizen-studio/platforms/tizen-9.0/tizen/rootstraps/tizen-9.0-device.core/` |
| `rs:libevas` | `rs:usr/lib/libevas.so.1.25.1`, read with `strings` and `objdump -T` / `-p` |
| TD-hw | Tizen guide "Hardware Acceleration", `https://github.com/Samsung/tizen-docs/blob/a9976172a2b5aa30c5170d257af0c5f6f0b39713/docs/application/native/guides/graphics/hw-acceleration.md` (removed from docs.tizen.org in Nov 2025) |
| TD-font | Tizen guide "Fonts", `https://github.com/Samsung/tizen-docs/blob/2d38692d78ac8c36bea621acbad892ed108fbc39/docs/application/native/guides/ui/efl/font-setting.md` |
| SS-res | Samsung, "Managing Screen Resolution", `https://developer.samsung.com/smarttv/develop/guides/fundamentals/managing-screen-resolution.html` |
| SS-design | Samsung TV design, "Apps Screen", `https://developer.samsung.com/tv/design/apps-screen` |
| CSS-img3 | W3C, CSS Images Module Level 3, `https://www.w3.org/TR/css-images-3/` |
| `RC:` | React Native 0.88 (react-native-tvos 0.88.0-0) `ReactCommon/`; `RN:` its `Libraries/` |
| `eina:` | EFL v1.25.1 `src/lib/eina/`, the same tree as `efl:` |
| `rs:libeina` | `rs:usr/lib/libeina.so.1.25.1`, read with `objdump -T` |
| `rs:webp` | `rs:usr/lib/evas/modules/image_loaders/webp/v-1.25/module.so`, read with `strings` |
| `cr:` | Chromium at `86828148fefdc39c8ce20976da7376ffa042e9c8`, `https://chromium.googlesource.com/chromium/src/+/86828148fefdc39c8ce20976da7376ffa042e9c8/<path>` |
| `hb:` | HarfBuzz 8.2.2, the Tizen 9 rootstrap's (`rs:usr/include/harfbuzz/hb-version.h:63`), `https://github.com/harfbuzz/harfbuzz/blob/8.2.2/<path>` |
| `aosp:` | Android `frameworks/base` at `main`, `https://github.com/aosp-mirror/platform_frameworks_base/blob/main/<path>` |
| `xb:` | expo-blur 58.0.5, `node_modules/expo-blur/` |
| W3C-fe | W3C, Filter Effects Module Level 1, `feGaussianBlur`, `https://www.w3.org/TR/filter-effects-1/#feGaussianBlurElement` |
| CSS-bg3 | W3C, CSS Backgrounds and Borders Module Level 3, 6.1.2 "Blurring Shadow Edges", `https://www.w3.org/TR/css-backgrounds-3/#shadow-blur` |

The TV's libevas is upstream 1.25.1 with Samsung patches: it links ThorVG and
rlottie, and its headers comment out functions the library still exports
(`rs:libevas`, `rs:usr/include/evas-1/Evas_Legacy.h`). An `src` row read in
upstream code can differ on the device in detail.

## 1. Engines, resolution, frame pacing

| Fact | Mark | Source |
| --- | --- | --- |
| Evas engine modules on the TV: `wayland_egl`, `gl_generic`, `gl_tbm`, `wayland_shm`, `software_tbm`, `buffer`, `fb` | doc | `rs:usr/lib/evas/modules/engines/` |
| `elm_win_add` picks the engine from `ELM_ACCEL`, then the Elementary config, then the window preference; an accelerated preference creates `wayland_egl`, otherwise `wayland_shm` | src | `efl:src/lib/elementary/efl_ui_win.c#L5020-L5050`, `#L5521-L5526` |
| The manifest attribute `hw-acceleration="on"\|"off"` on `<ui-application>` enables GL; when absent, "the default system policy determines" it | doc | TD-hw |
| `elm_config_accel_preference_set("opengl")` before the window is required for GL content, optionally `"opengl:depth24:stencil8"` | doc | TD-hw |
| Our manifest sets no `hw-acceleration` and `ReactHost` sets no preference, so the TV policy decides the engine. Which one it picks is unknown: log `ecore_evas_engine_name_get(ecore_evas_ecore_evas_get(evas))` at startup | unk | `shell/tizen-manifest.xml:4`, `runtime/host/src/ReactHost.cpp:104` |
| Standard app resolution is 1920x1080 on UHD TVs and 1280x720 on FHD TVs; larger content is scaled down. Design for 1920x1080 | doc | SS-res (web apps) |
| Native apps get the same UI plane size; `DeviceProfile` reads it from `http://tizen.org/feature/screen.width`/`height` | inf | SS-res, `runtime/host/src/modules/DeviceProfile.cpp:37-38` |
| Video (AVPlay) renders at 1920x1080 on its own plane | doc | SS-res |
| Every `Ecore_Evas` renders from one idle enterer, after the main loop has drained its events | src | `efl:src/lib/ecore_evas/ecore_evas.c#L209-L260` |
| Render sync is on by default (`ECORE_EVAS_RENDER_NOSYNC` turns it off): a changed canvas waits for an animator tick before it renders | src | `efl:src/lib/ecore_evas/ecore_evas.c#L74`, `#L632`, `#L237-L255` |
| On Wayland the tick is the compositor frame callback, so rendering is paced by vsync | src | `efl:src/modules/ecore_evas/engines/wayland/ecore_evas_wayland_common.c#L39-L75` |
| A global `ecore_animator_add` ticks from the windows: when every canvas's engine can register an animator, ecore_evas sets `ECORE_ANIMATOR_SOURCE_CUSTOM`, and a canvas tick calls `ecore_animator_custom_tick` once per loop iteration. Registering on Wayland adds a frame callback and, with nothing pending, commits an empty frame, so an animator keeps ticking at the display rate while nothing is drawn. Otherwise, as with `software_x11` in the simulator, the source is a timer at the animator frametime | src | `efl:src/lib/ecore_evas/ecore_evas.c#L3128-L3165`, `#L3195-L3273`; `efl:src/modules/ecore_evas/engines/wayland/ecore_evas_wayland_common.c#L58-L75` |
| Changing a Wayland window's alpha sets the engine info's `destination_alpha`, and `wayland_egl` then creates a new GL output window for the canvas | src | `efl:src/modules/ecore_evas/engines/wayland/ecore_evas_wayland_common.c#L243-L267`, `#L1944-L1963`; `efl:src/modules/evas/engines/wayland_egl/evas_engine.c#L655-L686` |
| `EVAS_CALLBACK_RENDER_PRE` fires once smart objects are calculated, as the canvas starts rendering; `RENDER_POST` once it is done, which for an asynchronous render is when `evas_render_wakeup` runs on the main loop after the render thread finished | src | `efl:src/lib/evas/canvas/evas_render.c#L3323-L3333`, `#L3870-L3892`, `#L3925-L4004` |
| `rn-tizen perf` reads frames and render time from those two callbacks, display ticks from an animator, and main-loop busy time between `ecore_idle_exiter` and `ecore_idle_enterer` callbacks (`runtime/host/src/diagnostics/PerfMonitor.cpp`) | inf | rows above |
| Evas logs each render phase (`+render_phase1` to `-render_phase8`, `render_update`, `render_output_flush`), each object it draws (`+render_object` / `-render_object`, the object as the log's pointer, nested for a smart object's members) and each mask and proxy it renders, to Eina's event log; the wayland_egl engine logs its buffer-age query as `gl_query_surf_swap_mode`. The TV's libevas carries the same event names | src | `efl:src/lib/evas/canvas/evas_render.c#L1819-L1932`, `#L2990-L3157`, `#L3323-L3887`, `efl:src/modules/evas/engines/wayland_egl/evas_wl_main.c#L404-L429`, `strings rs:libevas` |
| `eina_evlog_start`, `eina_evlog_steal` and `eina_evlog_stop` record and hand out that log: two 4 MiB buffers of items (monotonic time, thread, object, event name); a stolen buffer stays valid until the next steal. The TV's libeina exports all three with their bodies, though Samsung's `eina_evlog.h` comments their declarations out | src | `eina:eina_evlog.c#L153-L235`, `eina:eina_evlog.h#L36-L56`, `rs:libeina`, `rs:usr/include/eina-1/eina/eina_evlog.h:118-156` |
| A smart object's own render time, its members' subtracted, holds its map surface, its mask and its proxy work; texture uploads and shader binds happen inside the drawing object's time, since image `render_prepare` is compiled out (`#if 0`) | src | `efl:src/lib/evas/canvas/evas_object_image.c#L116-L129`, `efl:src/lib/evas/canvas/evas_render.c#L2450-L2684` |
| The GL engine loads its shader programs from a binary cache in `$HOME/.cache/evas_gl_common_caches/`, keyed by vendor, renderer and version, four programs at start and each other one at first use; with no cache it compiles every program at start and writes the cache. Only `gl_x11` writes new programs back while running; `wayland_egl` does at shutdown | src | `efl:src/modules/evas/engines/gl_common/evas_gl_shader.c#L549-L700`, `#L991-L1030`, `efl:src/modules/evas/engines/gl_common/evas_gl_file_cache.c#L71-L84`, `efl:src/modules/evas/engines/gl_x11/evas_engine.c#L1578-L1579` |
| Each Fabric transaction is applied in one main-loop callback (`SurfacePresenter::mountOnMainThread`), so a transaction never reaches the screen half applied, and several transactions inside one frame coalesce | inf | `runtime/host/src/fabric/SurfacePresenter.cpp:70-75` + rows above |
| Maps "reduce performance"; software is "not as fast as OpenGL" | doc | `rs:usr/include/evas-1/Evas_Legacy.h:7650-7652` |
| The software engine renders masks synchronously | src | `efl:src/lib/evas/canvas/evas_render.c#L2495-L2497` |

## 2. Smart objects

`ComponentView` builds every view as a clipped smart object whose class `move`
does nothing (`views/ViewSmartObject.cpp:9-16`): each view places its own
members and children in canvas pixels.

| Fact | Mark | Source |
| --- | --- | --- |
| Members are stacked and layered together with their smart object; stacking calls order a member only among its siblings | doc | `rs:usr/include/evas-1/canvas/efl_canvas_group.eo.h:129-135` |
| `evas_object_stack_above/below`: if `obj` is a member, the reference object must be a member of the same smart object; if not, neither | doc | `rs:usr/include/evas-1/Evas_Legacy.h:2054`, `:2100` |
| A new member is appended on top of its siblings | src | `efl:src/lib/evas/canvas/evas_object_smart.c#L295` |
| A clipped smart owns one static clipper rectangle at (-100000, -100000), 200000x200000, white | src | `efl:src/lib/evas/canvas/evas_object_smart.c#L812-L836` |
| Adding a member clips it to that clipper and shows the clipper if the smart is visible; removing a member unclips it and hides the clipper once it clips nothing | src | `efl:src/lib/evas/canvas/evas_object_smart_clipped.c#L79-L99` |
| Showing a clipped smart shows its clipper only if the clipper clips something, so a smart with no member draws no clipper | src | `efl:.../evas_object_smart_clipped.c#L44-L48` |
| `color_set` on a clipped smart sets the clipper's color, which multiplies into every member: this is how `opacity` reaches a subtree | src | `efl:.../evas_object_smart_clipped.c#L58-L63` |
| `clip_set` on the smart clips its clipper, so an outer clip reaches all members | src | `efl:.../evas_object_smart_clipped.c#L65-L77` |
| Moving a clipped smart moves every non-static member by the same delta, each member recursively; resize does nothing to members | src | `efl:src/lib/evas/canvas/evas_object_smart.c#L773-L809`; `evas_object_smart_clipped_smart_set` has no `resize` |
| That move is the clipped class's `move` hook; a class that replaces it with a no-op keeps every member in place when the smart object moves | doc + verified in simulator | `rs:usr/include/evas-1/Evas_Common.h:1976` (the `move` member of `Evas_Smart_Class`) |
| A move costs one `evas_object_move` per descendant object | inf | row above |
| Deleting a clipped smart deletes all its members | src | `efl:.../evas_object_smart_clipped.c#L27-L34` |
| Deleting an object that is a member removes it from its smart parent first; adding a member of another smart moves it | src | `efl:src/lib/evas/canvas/evas_object_main.c#L486-L487`, `efl:src/lib/evas/canvas/evas_object_smart.c#L1598-L1605`, `#L269` |
| Input events on a member propagate to its smart parent unless `propagate_events` is off; `pass_events` makes an object transparent to input | doc | `rs:usr/include/evas-1/canvas/efl_canvas_object.eo.h:517-544` |

## 3. Clipping, opacity, masks, transforms

| Fact | Mark | Source |
| --- | --- | --- |
| A clipped object's color is multiplied by its clipper's: `RESULT = OBJ * CLIP / (255 * 255)` per channel, recursively up the clip chain | doc | `efl:src/lib/evas/canvas/efl_canvas_object.eo#L87-L104` |
| An object that clips nothing is drawn like any other object; it turns invisible only while it clips something, and visible again when it stops | doc | `efl:.../efl_canvas_object.eo#L106-L110` |
| Render only considers objects with no clipees, and a new object's color is (255, 255, 255, 255): an idle clipper rectangle is a white rectangle | src | `efl:src/lib/evas/canvas/evas_render.c#L313-L323`, `efl:src/lib/evas/canvas/evas_object_main.c#L24` |
| A hidden clipper hides everything it clips | doc | `efl:.../efl_canvas_object.eo#L112-L115` |
| `no_render`: never drawn, still usable as clipper or proxy source; "useful to avoid drawing clipper objects (or masks) even when they don't clip any object" | doc | `efl:.../efl_canvas_object.eo#L442-L460` |
| `efl_canvas_object_no_render_set` is declared outside any beta guard, but `Evas.h` includes `Evas_Eo.h` only under `EFL_BETA_API_SUPPORT`; include `canvas/efl_canvas_object.eo.h` directly | doc | `rs:usr/include/evas-1/canvas/efl_canvas_object.eo.h:799`, `rs:usr/include/evas-1/Evas.h:216-217` |
| Only rectangles and images may be clippers; anything else is undefined behaviour | doc | `efl:.../efl_canvas_object.eo#L125-L126` |
| Any clipper that is not a rectangle becomes a mask | src | `efl:src/lib/evas/canvas/evas_clip.c#L365-L370` |
| A mask is rendered with its own color forced to opaque white, so tinting a mask image does nothing | src | `efl:src/lib/evas/canvas/evas_render.c#L2469-L2491` |
| GL fast path: a filled image mask with no border and no outer mask is scaled in the shader. Otherwise the mask is rendered into an 8-bit alpha surface | src | `efl:.../evas_render.c#L2521-L2560` |
| Masks nest: an inner mask is combined with the outer one | src | `efl:.../evas_render.c#L2503-L2515` |
| Object colors are premultiplied by alpha | doc | `rs:usr/include/efl-1/interfaces/efl_gfx_color.eo.h:32` |
| A map has exactly four points in canvas coordinates; `evas_object_map_set` then `evas_object_map_enable_set` | doc | `rs:.../Evas_Legacy.h:7657`, `:8533`, `:8563` |
| A map stays where it was set when the object moves, unless `evas_map_util_object_move_sync_set` is on (off by default) | doc | `rs:.../Evas_Legacy.h:8298` |
| A mapped smart object is first rendered into an offscreen surface the size of its own geometry, then the surface is mapped: children outside the view's bounds are cropped | src | `efl:src/lib/evas/canvas/evas_render.c#L1941-L1980` |
| A mapped image is drawn as a textured quad with no intermediate surface | inf | `_evas_render_can_map` in the same function |
| A map's points take canvas coordinates and image UVs in the object's own pixels: `evas_map_point_coord_set`, `evas_map_point_image_uv_set` | doc | `rs:.../Evas_Legacy.h:8386`, `:8454` |
| Growing the mapped smart object's geometry past the view by its shadows, outline and `overflowInset` keeps them in the surface: a scaled card keeps its glow and ring | verified in simulator | `views/ComponentView.cpp` (`bounds`) |
| A proxy image (`evas_object_image_source_set`) mirrors another object's rendering; `source_visible_set(EINA_FALSE)` hides the original | doc | `rs:.../Evas_Legacy.h:6128`, `:6214` |
| A proxy of a smart object, its source hidden and `source_clip_set(EINA_FALSE)`, works as an image clipper: its alpha masks what hangs from it, including text, Evas VG gradients and image gradients drawn in the source | verified in simulator | `views/MaskedView.cpp` |

## 4. Rounded corners, borders, shadows, gradients

Evas has no rounded rectangle, border, shadow or gradient primitive outside the
vector (VG) object. The options:

| Approach | API | Mark | Cost on a TV |
| --- | --- | --- | --- |
| Nine-patch image | `evas_object_image_border_set`, `evas_object_image_border_center_fill_set`; object color tints the pixels; pixels uploaded with `evas_object_image_data_copy_set` | doc `rs:.../Evas_Legacy.h:5750`, `:5835`, `:6023` | Small texture, drawn as quads on GL (inf) |
| Image clipper as mask | any image as `clip_set` target | doc + src above | GL fast path only without border; a nine-patch mask renders to a surface (src) |
| VG shape | `evas_object_vg_add`, `evas_vg_shape_append_rect(x, y, w, h, rx, ry)`, stroke width/color/cap/join | doc `rs:.../Evas_Legacy.h:3652`, `:4522`, `:4135`, `:4227` | Tizen's libevas imports `tvg_swcanvas_create`/`tvg_canvas_draw`: VG is rasterized by ThorVG on the CPU, then drawn as an image (inf, `rs:libevas`) |
| VG gradients | `evas_vg_gradient_linear_add`, `_radial_add`, `evas_vg_gradient_stop_set` | doc `rs:.../Evas_Legacy.h:4618-4660` | Same CPU raster |
| VG object as a clipper | | doc: undefined (rectangles and images only) | Do not |
| Filters (blur, shadow, grow) | `efl_gfx_filter_program_set` | doc: beta in Tizen headers (`rs:usr/include/efl-1/interfaces/efl_gfx_filter.eo.h:16`); legacy text filter deprecated (`Evas_Legacy.h:8604`) | Avoid |

What the image approach depends on:

| Fact | Mark | Source |
| --- | --- | --- |
| The GL engine reads `GL_MAX_TEXTURE_SIZE` from the driver, and an image wider or taller than it is not created, so it draws nothing. `EVAS_GL_INFO=1` prints the value | src | `efl:src/modules/evas/engines/gl_common/evas_gl_context.c#L989-L990`, `#L1093-L1116`, `efl:src/modules/evas/engines/gl_common/evas_gl_image.c#L395-L401` |
| The TV's `GL_MAX_TEXTURE_SIZE`: 16383 on the GQ75LS03DAUXZG, a Mali-G51 with OpenGL ES 3.2 `v1.r48p0` | verified on hardware | `glGetIntegerv` and `glGetString` read by a temporary probe, 2026-11-03 |
| A GL texture carries a one-pixel copy of its image's edges around it in the atlas, so a stretched image never samples its neighbour | src | `efl:src/modules/evas/engines/gl_common/evas_gl_texture.c#L1210-L1300` |
| A bordered image is drawn as nine pieces, each from its own rectangle of the image, with the object's smooth scaling; the middle piece only when the center fill is above `EVAS_BORDER_FILL_NONE` | src | `efl:src/lib/evas/canvas/evas_object_image.c#L2626-L2760` |
| Within one image the GL engine does not keep a piece to its own pixels: texture coordinates run from the piece's exact edges and smooth scaling samples with `GL_LINEAR`, so a one-pixel middle stretched across a box blends in the pixels beside it, about half of them at the box's edges and none at its center. The software engine scales each piece from its own pixels, so the simulator does not show it | src, verified on hardware | `efl:src/modules/evas/engines/gl_common/evas_gl_context.c#L2400-L2403`, `#L4091-L4113`; GQ75LS03DAUXZG, 2026-11-03 |
| `EVAS_BORDER_FILL_NONE`: "Image's center region is not to be rendered" | doc | `rs:usr/include/evas-1/Evas_Legacy.h:5812` |
| A linear gradient's line runs through the box's center at angle A (0deg up, clockwise) and is `abs(W sin A) + abs(H cos A)` long; `to top right` and the other corners point into that corner's quadrant, perpendicular to the line joining the two neighbouring corners | doc | CSS-img3, section 3.1.1 |
| A radial gradient's ray runs right from its center, so a length stop is measured against the ending ellipse's horizontal radius; sizes `closest-side`, `farthest-side`, `closest-corner`, `farthest-corner` (the default) | doc | CSS-img3, sections 3.2.2 to 3.2.4 |
| Between two stops the color is interpolated in premultiplied RGBA; a transition hint at H makes the weight at P `P^(log 0.5 / log H)`; unpositioned stops are fixed up (first 0%, last 100%, never before an earlier one, the rest spread evenly) | doc | CSS-img3, sections 3.4.2, 3.4.3 |
| `experimental_backgroundImage` reaches `BaseViewProps::backgroundImage` as `LinearGradient` (angle in degrees, or a corner keyword) and `RadialGradient` layers, the first on top; JavaScript parses the CSS (`processBackgroundImage`) unless `nativeCSSParsing` is on | src | `RC:react/renderer/components/view/BaseViewProps.cpp:280-290`, `RC:react/renderer/components/view/BackgroundImagePropsConversions.cpp:73-135`, `RN:Components/View/ReactNativeStyleAttributes.js:47-49` |
| On the `cxx` platform, as on Android, `SharedColor` 0 means no color: a `transparent` stop arrives with the same empty color as a transition hint, and the gradient loses its fade. Android draws from Java, iOS has another color type, so neither meets it. `react-native-overrides/Libraries/StyleSheet/processBackgroundImage.tizen.ts` sends 0 as `0x00000001`, which draws the same | src | `RC:react/renderer/graphics/platform/cxx/react/renderer/graphics/HostPlatformColor.h:16-19`, `RC:react/renderer/graphics/Color.h:32`, `:56-58`, `BackgroundImagePropsConversions.cpp:84-99` |
| A box-shadow's blur is not defined exactly, but must come within 5% of each pixel of a Gaussian whose standard deviation is half the blur radius | doc | CSS-bg3 |
| For a deviation s of 2 or more, three box blurs of size d = floor(s x 3 x sqrt(2 pi) / 4 + 0.5) approximate the Gaussian "to within roughly 3%": three of size d centered on the pixel when d is odd; when it is even, two of size d centered half a pixel left and right of it, then one of size d + 1 centered | doc | W3C-fe |
| A CSS `dashed` border in Chromium: dashes three times the width and gaps twice it below 3 px (twice and once from 3 px), the gap then fitted so a whole number of dashes fills the path; `dotted`: round dots one width across, fitted the same way | src | `cr:third_party/blink/renderer/platform/graphics/styled_stroke_data.cc` (`DashLengthRatio`, `DashGapRatio`, `SelectBestDashGap`, `DashEffectFromStrokeStyle`) |

## 5. Textblock

Style and markup tags (`efl:src/lib/evas/canvas/evas_object_textblock.c`, the
`evas_textblock_style_page` doc comments; index at `#L1568-L1612`).

| Tag | Syntax and behaviour | Mark | Source |
| --- | --- | --- | --- |
| `font` | fontconfig name, `Family:style=Bold` form accepted | doc | `#L2353-L2362`, TD-font |
| `font_size` | "size of font in points", parsed with `atoi`: integers only | doc + src | `#L2382-L2400` |
| point to pixel | Evas sets FreeType size at 75 DPI by default, so `font_size=N` renders an em of N x 75/72 px (4.2% larger than CSS px). `EVAS_FONT_DPI` overrides it at font init | src | `efl:src/lib/evas/common/evas_font_load.c#L22-L23`, `#L423`, `efl:src/lib/evas/common/evas_font_main.c#L45-L54` |
| `EVAS_FONT_DPI` on retail TVs | unknown; log a measured em height on a device | unk | |
| `font_weight` | `normal thin ultralight extralight light book medium semibold bold ultrabold extrabold black extrablack` | doc | `#L2420-L2447` |
| synthetic styles | if the face lacks the weight or slant, Evas emboldens or slants at runtime | src | `efl:.../evas_font_load.c#L529-L542` |
| `font_style` | `normal oblique italic` | doc | `#L2449-L2466` |
| `color` | `#rgb #rgba #rrggbb #rrggbbaa`, `rgb()`, `rgba()`, names; stored straight, and passed to the draw context without premultiplying, while every other object passes premultiplied values | src | `efl:src/lib/evas/common/evas_text_utils.c#L1381-L1445`, textblock `#L14924-L14934` |
| translucent text | whether `#ffffff80` renders too bright on the GL engine is unknown; test it | unk | |
| `align` | `auto locale center middle left right start end`, 0.0 to 1.0, or a percentage. No justify | doc | `align` doc comment |
| `valign` | `top center middle bottom baseline base`, number, percentage | doc | `valign` doc comment |
| `wrap` | `word char mixed hyphenation` or empty for no wrap; `mixed` breaks at words, then at characters | doc | `wrap` doc comment |
| `ellipsis` | 0.0 to 1.0, otherwise off; 0 cuts the leftmost text, 1 the rightmost | doc | `#L2923-L2946` |
| vertical ellipsis | with `ellipsis=1.0` and an object height > 0, a line that would cross the height is removed; never on a one-line text | src | `#L4914-L4940` |
| `linesize` | pixel line height; only grows a line (applies when ascent + descent is smaller), resets `linerelsize` | doc + src | `#L1881-L1893`, `#L3899-L3906` |
| `linerelsize` | percentage; scales ascent and descent, each truncated to an integer, so it can also shrink a line; resets `linesize` | doc + src | `#L1897-L1919`, `#L3907-L3911` |
| first and last line | whatever the line size, the first line is pushed down to the font's maximum ascent and the last padded to its maximum descent, so a one-line paragraph with a small `linerelsize` keeps its natural height in `size_formatted_get` | src | `#L4895-L4905`, `#L4766-L4793`, `#L3931-L3975` |
| font metrics | an Evas text object's ascent and descent are the `font_ascent_get` and `font_descent_get` a textblock line takes for the same format; `Family:weight=bold:slant=italic` names the face the way `font_weight` and `font_style` do | src | `efl:src/lib/evas/canvas/evas_object_text.c#L2273-L2287`, textblock `#L4012-L4013`, `efl:src/lib/evas/canvas/evas_font_dir.c#L597-L640` |
| variable fonts | Evas opens every face at index 0 and sets no variation axis, so a variable font draws its default instance; a bold wanted from a face whose OS/2 weight is below 600 is emboldened at run time, which widens it | src | `efl:src/lib/evas/common/evas_font_load.c#L237`, `#L528-L545` |
| static instances | The Tizen 9 rootstrap ships HarfBuzz 8.2.2 with `libharfbuzz-subset.so.0` and its headers; `hb_subset_input_keep_everything`, `hb_subset_input_pin_axis_location` and `hb_subset_input_pin_axis_to_default` are declared outside `HB_EXPERIMENTAL_API`. HarfBuzz is not Tizen native API, so it is loaded with `dlopen` | doc | `rs:usr/lib/libharfbuzz-subset.so.0.60822.0`, `rs:usr/include/harfbuzz/hb-subset.h:145`, `:168-176` |
| instance weight | Pinning `wght` sets the instance's `OS/2.usWeightClass` to the pinned value, so fontconfig matches it as that weight and Evas does not embolden it | src | `hb:src/hb-ot-os2-table.hh#L249-L255` |
| fonts by path | A `font` value that starts with `/` is loaded from that file, without fontconfig; Evas then asks fontconfig for the fallbacks of that face (`FcFreeTypeQueryFace`, `FcFontSort`), so missing glyphs still come from other fonts | src | `efl:src/lib/evas/canvas/evas_font_dir.c#L72-L80`, `#L791-L793`, `#L966-L991` |
| `linegap` / `linerelgap` | extra pixels / percentage added under each line; `atoi` accepts a negative value | doc + inf | `#L1925-L1960`, `#L3912-L3914` |
| letter spacing | no such tag in 1.25. A per-glyph `item` placeholder breaks shaping and adds break opportunities, so it cannot stand in for spacing that wraps right | doc + inf | style index; `item` doc comment |
| other tags | `underline strikethrough backing backing_color style (shadow, outline, glow) tabstops item linefill password` | doc | style index |
| decoration values | `underline=on\|off\|single\|double` with `underline_color` and `underline2_color`; `strikethrough=on\|off` with `strikethrough_color`; `backing=on` with `backing_color`; `wrap=none`; `linesize` in pixels | doc + verified in simulator | `rs:usr/include/evas-1/canvas/evas_textblock_legacy.h:83-119` |
| `align` inside a run's markup tag aligns the line, as it does in the style | verified in simulator | `text/TextblockFormat.cpp` |
| spaces in a value | a value ends at whitespace unless quoted with `'...'` or escaped as `\ ` | src | `#L3334-L3380` |
| system font | font name `Tizen` is an alias for the system font, e.g. `<font=Tizen:style=Regular>` | doc | TD-font |
| TV font | One UI Sans is "the default font for Samsung Tizen", in four weights | doc | SS-design |
| One UI Sans fontconfig family | unknown; log `evas_font_available_list(evas)` once on a TV | unk | `rs:usr/include/evas-1/canvas/evas_canvas_eo.legacy.h:671` |
| app fonts | `evas_font_path_global_append(dir)` adds `dir` to fontconfig's app fonts, so families inside resolve by name | doc + src | `rs:usr/include/evas-1/Evas_Common.h:3374`, `efl:src/lib/evas/canvas/evas_font_dir.c#L128-L143`, `#L1428-L1437` |

Measuring:

| Fact | Mark | Source |
| --- | --- | --- |
| `evas_object_textblock_size_formatted_get`: size after wrapping at the object's current size. `size_native_get`: unwrapped size | doc | `rs:usr/include/evas-1/canvas/efl_canvas_textblock_eo.legacy.h:105-165` |
| `evas_object_textblock_style_insets_get` returns the padding that shadows and outlines add around the text | doc | same file `:40-53` |
| `evas_object_textblock_line_number_geometry_get(obj, line, ...)` returns one line's rectangle | doc | `rs:usr/include/evas-1/canvas/evas_textblock_legacy.h:852` |
| Fabric's `TextMeasureCache` keeps 1024 measurements keyed by the attributed string (as laid out), the paragraph attributes, the exact layout constraints and the point scale; the host's `TextLayoutManager` consults it before the measurer, on the thread that lays out. Yoga also keeps the last 8 measurements of each node | doc | `RC:react/renderer/textlayoutmanager/TextMeasureCache.h:64-72`, `:104-110`, `:270-275`, `runtime/platform/react/renderer/textlayoutmanager/TextLayoutManager.cpp`, `RC:yoga/yoga/node/LayoutResults.h:25` |
| A line wraps, or takes `ellipsis=1.0`, only once its pen position plus the next item's width passes the object's width less the style padding and margins; an `ellipsis` below 1.0 cuts only when the line's advances plus its last item's overhang pass it. The formatted width is the widest line plus its margins, whatever the alignment | src | textblock `#L6726-L6731`, `#L6405-L6424`, `#L6457-L6474`, `#L4981-L4985` |
| So a paragraph laid out with nothing wrapped or cut lays out the same at any width at least as wide as its formatted width, and a formatted size equal to `size_native_get`'s says nothing was wrapped or cut. `text/UnwrappedSizes.h` measures such a paragraph once for every wider width | inf + verified in simulator | rows above |
| Because vertical ellipsis checks the object height, measuring a style that contains `ellipsis` at a height of 1 drops every line after the first; height 0 disables the check | src + inf | textblock `#L4914-L4916` |
| Evas ellipsizes a wrapped line unless one more line, at the font's maximum ascent and descent, fits under it (`c->y + ascent + descent + maxasc + maxdesc > c->h`) | src | textblock `#L6754-L6772` |
| A second path ellipsizes at once, whatever the height: when the wrap point falls just past the end of a text item, `ellipsis=1.0` cuts the line there (a `FIXME` in the source). Where items end depends on fonts and width, so a two-line paragraph can come out one line long | src + verified in simulator | textblock `#L6900-L6912`; the same code in EFL 1.28.1 |
| A textblock cursor edits the laid-out text: `evas_textblock_cursor_line_set`, `line_char_last`, `paragraph_last`, `range_delete`, `text_prepend` (plain text, inserted before the cursor's character, so inside that character's run), `char_delete`, `content_get` (a plain character, or markup for a visible format) | doc + src + verified in simulator | `rs:usr/include/evas-1/canvas/evas_textblock_legacy.h:348`, `:399`, `:473`, `:929`, `:993`, `:1032`, `:1064`; textblock `#L12349-L12380` |

## 6. Images

| Fact | Mark | Source |
| --- | --- | --- |
| Shared loaders on Tizen 9: bmp, generic, gif, ico, pmaps, psd, tga, tgv, tiff, wbmp, webp, xpm | doc | `rs:usr/lib/evas/modules/image_loaders/` |
| PNG, JPEG and EET are built into libevas | src | `efl:src/lib/evas/meson.build#L15-L38`; `rs:libevas` needs libpng16 and libjpeg.so.8 |
| AVIF has a loader upstream, disabled by default, and no module in the rootstrap: no AVIF on the TV | src + inf | `efl:meson_options.txt#L189-L193` |
| No JPEG 2000 or DDS module in the rootstrap; HEIF and JPEG XL loaders do not exist in 1.25 | doc + src | rootstrap listing, `efl:src/modules/evas/image_loaders/` |
| Animated GIF and animated WebP expose frames through `evas_object_image_animated_*` | doc + src | `rs:.../Evas_Legacy.h:5185-5223`, `efl:src/modules/evas/image_loaders/webp/evas_image_load_webp.c#L214-L243`, `.../gif/evas_image_load_gif.c#L551` |
| `evas_object_image_preload(obj, EINA_FALSE)` decodes in the background and fires `EVAS_CALLBACK_IMAGE_PRELOADED`; showing the object before that cancels the preload and decodes synchronously. Preload hidden, show in the callback | doc | `rs:.../Evas_Legacy.h:5040` |
| `evas_object_image_memfile_set` copies encoded bytes from memory; the format argument is a hint such as `"jpg"` | doc | `rs:.../Evas_Legacy.h:4857` |
| `evas_object_image_load_size_set` decodes as if the source were that size; JPEG uses libjpeg DCT scaling | doc + src | `rs:.../Evas_Legacy.h:5387`, `efl:src/lib/emile/emile_image.c#L1466-L1469`, `#L1553-L1554` |
| Smooth scaling is on by default and costs more | doc | `rs:.../Evas_Legacy.h:5554` |
| `evas_image_cache_set(evas, bytes)` sizes the canvas image cache | doc | `rs:usr/include/evas-1/canvas/evas_canvas_eo.legacy.h:27` |
| Evas loads files and memory, not URLs | inf | no URL loader in the list above |
| `evas_object_image_size_get` right after `file_set` or `memfile_set` returns the image's own size from its header, before any decode | verified in simulator | `views/ImageView.cpp` (`decode`) |
| With a load size half the image or less, a JPEG decodes smaller: a 600x400 JPEG in a 251x157 box read back 300x200 after the preload | verified in simulator | |
| The preload's `EVAS_CALLBACK_IMAGE_PRELOADED` fires for `memfile_set` and `file_set` sources; the object stays hidden until then | verified in simulator | `rs:.../Evas_Legacy.h:5040` |
| An image drawn before its pixels are loaded is decoded by `evas_cache_image_load_data` on the drawing thread, the main loop. If its preload is still running, that call cancels it, waits for the preload thread (`ecore_thread_wait`, a loop of `ecore_main_loop_thread_safe_call_wait(0.0001)`), then decodes anyway | src | `efl:src/lib/evas/cache/evas_cache_image.c#L1274-L1330`, `efl:src/lib/evas/cache/evas_preload.c#L138-L149`, `efl:src/lib/ecore/ecore_thread.c#L850-L889` |
| Samsung's WebP loader has a `file_head_with_data` step (`evas_image_load_file_head_with_data_webp`, `WebPDecodeBGRA`): the header gives the size, the data step decodes, so a preload decodes on Evas's thread. Neither upstream's loader nor Samsung's reads the load size: a WebP decodes at its own size | src | `strings rs:webp`, `efl:src/lib/evas/common/evas_image_load.c#L479-L489`, `efl:src/modules/evas/image_loaders/webp/evas_image_load_webp.c#L152-L246` |
| `EVAS_GL_PRELOAD=1` uploads a preloaded image's texture on a thread with a second GL context, after drawing a 16x16 miniature of it in its place | src | `efl:src/modules/evas/engines/gl_common/evas_gl_preload.c#L328-L343`, `efl:src/modules/evas/engines/gl_common/evas_gl_texture.c#L1414-L1540` |

## 7. Thread safety

| Fact | Mark | Source |
| --- | --- | --- |
| "The EFL is, for the most part, not thread safe": an Evas object can only be updated from the main loop thread | doc | `rs:usr/include/ecore-1/Ecore_Common.h:1665-1670` |
| In an `ecore_thread` worker "no functions from the EFL can be used, except for those from Eina that are marked to be thread-safe" | doc | same file `:1784-1789` |
| `ecore_main_loop_thread_safe_call_async/sync` is the safe way to run work on the main loop | doc | same file `:424-438`, `:455` |
| `ecore_thread_main_loop_begin/end` suspends the main loop so a thread may call EFL; both sides block, and the thread adopts the main Eo domain | doc + src | same file `:470-488`, `efl:src/lib/ecore/ecore.c#L686-L709` |
| Eo objects live in a per-thread domain; objects of different domains must not be mixed | doc | `rs:usr/include/eo-1/Eo.h:1072-1082` |
| `ecore_evas_buffer_new` registers the canvas in a global list, without a lock, that the main-loop idle enterer walks; ecore timers refuse calls off the main loop; textblock's format table uses an unlocked refcount | src | `efl:src/lib/ecore_evas/ecore_evas.c#L3378-L3388`, `efl:src/lib/ecore/ecore_timer.c#L194`, textblock `#L1394`, `#L1563-L1675` |
| The font caches every canvas shares are globals with no lock: `fonts_src`, `fonts` and `fonts_lru` hold the loaded faces; `fonts_cache` and `fonts_zero` the fonts `evas_font_load` looks up and promotes and `evas_font_free` moves. Only FreeType calls take a lock (`lock_font_draw`), for the render thread. EFL 1.28.1 has the same globals | src | `efl:src/lib/evas/common/evas_font_load.c#L25-L27`, `efl:src/lib/evas/canvas/evas_font_dir.c#L27-L28`, `#L263-L298`, `#L686-L730`, `efl:src/lib/evas/common/evas_font_main.c#L12`; `efl128:` the same files (`evas_font_dir.c` at `#L23-L24`) |
| So no canvas can lay text out off the main loop, neither `ecore_evas_buffer_new`'s nor a bare `evas_new`'s: a textblock loading or freeing a font on Fabric's thread races the main loop's paragraphs on the same lists. Measuring runs on the main loop, ahead of the work queued there and between two mutations of a mount (`threading/MainThread.h`, `runOnMainThreadSoon`) | inf | rows above |
| A software engine renders asynchronously: the main loop prepares a frame and a render thread draws it. `wayland_egl` never does ("Wayland egl engine can't async render"); `ECORE_EVAS_FORCE_SYNC_RENDER` turns it off everywhere | src | `efl:src/modules/ecore_evas/engines/wayland/ecore_evas_wayland_common.c#L3113-L3118`, `efl:src/modules/ecore_evas/engines/x/ecore_evas_x.c#L4823-L4826` |
| `evas_object_async_block` only takes and releases `lock_objects`, which no render path holds, so it does not wait for the render thread | src | `efl:src/lib/evas/include/evas_inline.x#L311-L318`; no `lock_objects` in `efl128:src/lib/evas/canvas/evas_render.c` |
| Turning a map off, setting a null map or deleting a mapped object frees the map's surface at once, while the render thread may still be drawing into it; snapshot and proxy surfaces go the same way | src + verified in simulator | `efl128:src/lib/evas/canvas/evas_map.c` (`_evas_object_map_enable_set`, `evas_object_map_set`), crash below |
| `evas_sync(evas)` waits for the render thread (`evas_render_rendering_wait`); public on Tizen 9 | doc + src | `rs:usr/include/evas-1/canvas/evas_canvas_eo.legacy.h:742`, `efl128:src/lib/evas/canvas/evas_render.c` (`_evas_canvas_sync`) |
| A nine-patch's pixels are plain memory until `setNinePatch` copies them into an image, so a shadow patch is made on any thread and handed to the main loop; only the copy touches Evas (`views/ShadowPatches.cpp`) | inf | the two rows at the top of this section |

## 8. SVG through Evas VG

| Fact | Mark | Source |
| --- | --- | --- |
| Public on Tizen 9: shapes with `append_move_to`, `line_to`, `cubic_to`, `arc_to(x, y, rx, ry, angle, large_arc, sweep)`, `close`, `rect`, `circle`; fill color; stroke width, color, cap, join; linear and radial gradients with stops and spread; node color and `transformation_set(Eina_Matrix3)` | doc | `rs:.../Evas_Legacy.h:4135-4444`, `:4618-4660`, `rs:.../canvas/efl_canvas_vg_node_eo.legacy.h:34` |
| Exported by libevas but commented out of Tizen's headers: `evas_vg_shape_append_svg_path`, `append_quadratic_to`, `append_ellipse`, `stroke_dash_set`, `fill_rule_set`, `evas_object_vg_file_set`, `evas_object_vg_viewbox_set`, `fill_mode_set`, `evas_vg_node_mask_set`, node stacking | doc + src | `rs:.../Evas_Legacy.h:3700`, `:4177`, `:4237`, `:4373`, `:4501`, `:4532`; `.../efl_canvas_vg_object_eo.legacy.h:46`, `:70`; `.../efl_canvas_vg_node_eo.legacy.h:87`; `objdump -T rs:libevas` |
| The Eo path API with SVG path data (`efl_gfx_path_append_svg_path`) is beta | doc | `rs:usr/include/efl-1/interfaces/efl_gfx_path.eo.h:16`, `:296` |
| libevas embeds SVG, Lottie JSON (rlottie), EET and TVG vector loaders | src | `rs:libevas` strings |
| Which SVG features the file loader supports | unk | |
| VG renders on the CPU through ThorVG, then uploads; geometry changes re-rasterize | inf | `rs:libevas` imports `tvg_swcanvas_create`, `tvg_canvas_draw` |
| `evas_vg_container_add(vg_object)` makes a container owned by the object; `evas_object_vg_root_node_set` makes it the drawn root; `evas_vg_shape_add(container)` appends a shape, drawn in creation order | src | `efl:src/lib/evas/canvas/efl_canvas_vg_container.c#L531-L541`, `efl:src/lib/evas/canvas/efl_canvas_vg_object.c#L129-L178`, `efl:src/lib/evas/canvas/efl_canvas_vg_node.c#L381-L386` |
| The legacy `evas_vg_shape_append_cubic_to(obj, x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1)` takes the end point first; the Eo `efl_gfx_path_append_cubic_to` takes the control points first | src | `efl:src/lib/evas/canvas/efl_canvas_vg_shape.c#L403-L407` |
| A shape's fill is its node color (`evas_vg_node_color_set`), premultiplied; a new node is white and visible, and an invisible node is not rendered | doc + src | `rs:.../Evas_Legacy.h:3930-3947`, `efl:src/lib/evas/canvas/efl_canvas_vg_node.c#L264`, `#L731-L734`, `efl:src/lib/evas/canvas/efl_canvas_vg_object.c#L422` |
| A root set by the app renders into a buffer the size of the root's path bounds, then the buffer is drawn at the object's position plus the bounds' origin: path coordinates are relative to the object's top-left corner | src | `efl:src/lib/evas/canvas/efl_canvas_vg_object.c#L717-L788` |
| Those bounds come from the untransformed path points (`efl_gfx_path_bounds_get`) of the root's children, each grown by stroke width x miterlimit. Node transformations are not counted, so content moved or scaled by a transformation can be cropped, and 1.25.1 leaves a zero-width or zero-height shape (a horizontal line) out of the union | src | `efl:src/lib/evas/canvas/efl_canvas_vg_container.c#L278-L333`, `efl:src/lib/efl/interfaces/efl_gfx_path.c#L222-L245` |
| 1.28.1 rounds the stroke margin up, keeps zero-area shapes in the union, and draws nothing while the bounds are empty | src | `efl128:src/lib/evas/canvas/efl_canvas_vg_container.c` (`bounds_get`), `efl128:src/lib/evas/canvas/efl_canvas_vg_object.c` (`_user_vg_entry_render`) |
| The renderer draws every node under the root that is not a container, gradients included, and the root's bounds take in every child's: a gradient used as a fill belongs in a container that is never the root | src | `efl:src/lib/evas/canvas/efl_canvas_vg_object.c#L417-L505`, `efl:src/lib/evas/canvas/efl_canvas_vg_container.c#L278-L333` |
| A shape with a fill node is painted from the fill alone, its node color ignored; gradient stops reach the color table as given, interpolated as premultiplied values like node colors | src | `efl128:src/lib/ector/software/ector_renderer_software_shape.c` (`_ector_renderer_software_shape_ector_renderer_draw`), `efl:src/static_libs/draw/draw_main.c#L286-L330` |
| Whether Samsung's ThorVG-backed libevas also takes premultiplied stops | unk: draw a stop with alpha 128 on a TV | |
| react-native-svg sends `<Stop>`s as `gradient`, offset and ARGB pairs in a `std::vector<Float>`, which is `float` on `cxx`; an ARGB whose alpha is neither 0 nor 255 needs more than 24 bits, so its blue can lose up to 7 bits | src | `react-native-svg/src/lib/extract/extractGradient.ts:72-92`, `RC:react/renderer/graphics/platform/cxx/react/renderer/graphics/Float.h:18` |

## Implications for react-native-tizen

### Rules

| Rule | Why |
| --- | --- |
| Touch Evas only on the main loop, through `runOnMainThread` | section 7 |
| Before deleting a mapped view, a snapshot or a proxy, turning a map off or changing a proxy's source, wait for the render thread (`views/RenderSync.h`) | section 7 |
| Reach a library outside Tizen's native API (libefl's filters, HarfBuzz's subsetter) through `dlopen` and `dlsym`, and keep the behaviour from before when it is missing | sections 5, 8 and "Backdrop blur" |
| Never leave a clipper with no clipees on screen. The view's own background hangs from its clipper so it always clips something, or set `no_render` | section 3 |
| Use premultiplied colors for objects (`setColor` in `views/EvasColor.cpp` does) | section 3 |
| Stack members only against siblings of the same smart object | section 2 |
| Prefer images (nine-patch, masks) over VG for view decoration; GL draws them as textures | sections 3, 4 |
| Append VG paths already in the object's pixels, never through a node transformation, and keep a transparent rectangle the size of the view in the root, so the render buffer always covers the view | section 8 |
| Call a VG function Tizen exports but its headers comment out (`stroke_dash_set`, `fill_rule_set`, `append_svg_path`...) only through a declaration of our own resolved at run time, and keep a fallback: a firmware can drop an undeclared symbol | section 8 |
| Use a map only for real scale, rotate or perspective; translate by moving the object | section 3 |
| Decode images off the main loop with `preload`, hidden until loaded; cap decode size with `load_size_set` | section 6 |
| Log the engine name, `EVAS_FONT_DPI` and `evas_font_available_list` once at startup until each is confirmed on a TV | sections 1, 5 |
| Set `hw-acceleration="on"` in the manifest and call `elm_config_accel_preference_set("opengl")` before `elm_win_add`, then confirm `wayland_egl` in the log | section 1 |

### Features

Paths are under `runtime/host/src/`.

| Feature | Approach | Status |
| --- | --- | --- |
| One 1920x1080 screen | The surface lays out 1920x1080 points at `pointScaleFactor` = the largest scale that fits the window (2/3 in 1280x720); every view places itself at `round(points x scale)` pixels, text is set and measured at that scale, `DeviceInfo` reports 1920x1080 at that scale (`mounting/LogicalScreen.h`) | done, verified in simulator at 1920x1080 and 1280x720 |
| Rounded background | One white alpha nine-patch per radius set, generated on the CPU and cached by key (`views/PatchCache.cpp`); `border_set` to the radii; object color = premultiplied `backgroundColor`. A rectangle when square or transparent, and when the box clips: its rounded clipper is then the only mask, so a corner pixel's coverage applies once (`views/ViewBox.cpp`, `views/BoxShape.cpp`, `views/NinePatch.cpp`) | done, verified in simulator |
| `overflow: hidden` with radius | The same nine-patch as the clipper of the content and children (an image clipper is a mask); a rectangle clipper when square | done, verified in simulator; mask cost on a TV not measured |
| Borders | One color: a tinted ring nine-patch, per-side widths and rounded corners included, its clear middle left undrawn (`EVAS_BORDER_FILL_NONE`) so the GL engine does not blend the border into the box (section 4). A color per side: four rectangles; with rounded corners the ring takes the first visible color (`views/BoxBorder.cpp`). `dashed` and `dotted` on every side: a ring the box's size, cut along its centerline, corners included, sized as Chromium does (section 4), cached by size; solid past 2048 px (`views/PatternedRing.cpp`) | done, verified in simulator; per-side colors with radius approximate; a style per side draws solid |
| Outline | A ring nine-patch `outlineOffset` outside the border box (inside when negative), corners grown by the offset as browsers draw them | done, verified in simulator |
| Shadows | `boxShadow` (outer) and the iOS `shadow*` props: a nine-patch blurred on the CPU (Gaussian, sigma = blur / 2; from sigma 3, three box blurs per axis as W3C-fe approximates it, `views/AlphaBlur.cpp`), cached by (radii, blur, spread) in the shared patch cache, tinted, under the background (`views/BoxShadows.cpp`). A patch not in the cache is made on a painting thread; the shadow keeps the patch it showed, or shows nothing, until it arrives (`views/ShadowPatches.cpp`). Inset shadows and `elevation` are ignored | done, verified in simulator and on hardware |
| Mounting | A view is placed only while it hangs from the surface's root (`mountAsRoot`): Fabric inserts a new subtree bottom-up, and it is placed once, when it joins the window. A layout change that only moves a view skips its box and transform, which depend on size, border widths, scale and direction alone. A box that neither paints a color nor clips has no background object, so a plain container is its smart object and the smart's clipper (`views/ViewBox.cpp`). Between two mutations the main loop runs any text measurement Fabric's thread is waiting on | done, verified in simulator |
| Gradients | `experimental_backgroundImage`, linear and radial, every layer an image member above the background color and below the content, over the padding box and cut to its inner corners, painted on the CPU at the point scale (`views/BoxGradients.cpp`, `views/GradientImage.cpp`). An axis-aligned linear gradient is one pixel per step along its line, the corners kept as nine-patch borders across it; any other is the whole box, scaled down past 512x512 pixels. No side passes 2048 px. A whole-box patch reads its colors from a table along the gradient line, 16 entries per pixel of the line's steepest change, so a pixel's color can differ from the ramp's by one step of 255 at most; the rounded corners keep the exact ramp. Patches are cached by layer, size and corners in the shared patch cache. Stops, hints and colors as CSS-img3 (`views/GradientRamp.cpp`, `views/GradientGeometry.cpp`). `backgroundSize`, `backgroundPosition` and `backgroundRepeat` are ignored, and a translucent border shows the background color under it, not the repeated gradient | done, verified in simulator beside Chrome |
| Generated patches | Rounded backgrounds and clippers, rings, outlines, shadows, patterned rings and gradients share one cache of 24 MiB of pixels, keyed by kind and every value the pixels depend on; past the budget the least recently used go first, and the newest always stays. A patch is uploaded with `evas_object_image_data_copy_set`, so dropping it never pulls pixels from an image that shows it (`views/PatchCache.cpp`) | done, verified in simulator |
| Transforms | Translate only: added to the position, children follow. Scale, rotate, perspective: a four-point map on the view's smart object around `transformOrigin` (center by default), its geometry grown by shadows, outline and `overflowInset` so they stay in the surface (`views/ViewTransform.cpp`) | done, verified in simulator |
| ScrollView | Clips, shifts its children by the content offset; `scrollTo`, `scrollToEnd` (animated over 250 ms with an Ecore timeline, decelerating); writes `contentOffset` into the shadow tree's state; `onScroll` throttled by `scrollEventThrottle`, `onMomentumScrollEnd` when a scroll ends (`views/ScrollView.cpp`). Commands arrive through `schedulerDidDispatchCommand`, after the commit their view came with is mounted | done, verified in simulator, vertical and horizontal |
| Images | `views/ImageView.cpp`: remote bytes from an `HttpEngine` of its own (`views/ImageFetcher.cpp`, 48 MiB in-memory cache), `data:` URIs, files; the header gives the size, `load_size_set` near the drawn size when the image is twice the box or more, the box taken from the layout so an image mounted with its subtree decodes small too, hidden `preload`, shown on `IMAGE_PRELOADED`; `resizeMode` through geometry and `fill_set` (`views/ImageFit.cpp`); clipped to rounded corners; `onLoadStart`, `onLoad` (source size in pixels / source scale, as Fabric reports it), `onError`, `onLoadEnd`. `tintColor` multiplies the pixels. PNG, JPEG, WebP, GIF only; no AVIF; no `blurRadius`, `defaultSource` or progress | done, verified in simulator |
| Modal | Its root is not a member of its parent: a top-level object on layer 1000 at the screen's origin, so no clip, opacity or map of an ancestor reaches it; a Tizen descriptor sizes it to the logical screen from the first layout (React Native's portable one starts at 0x0); `onShow` once laid out (`views/ModalHostView.cpp`). While a Modal is mounted, Back goes to the last one mounted as `onRequestClose` and never to `BackHandler`, as on Android (`tizen-input.md`, section 12) | done, verified in simulator |
| Group opacity | A proxy image of the subtree, at a surface's cost. Opacity multiplies into each member instead, as on Android | not done |
| SVG (react-native-svg) | One VG object per `<Svg>` at the view's canvas pixels, clipped by the view's own box (`setAlwaysClips`); user units are points, scaled by the point scale after the viewBox transform; painted again when a node's props change, when a node mounts, unmounts or goes (`didMount`, `willUnmount`), and when the view's pixel size changes, never on a move: section 8 and the VG rule above (`svg/SvgView.cpp`) | done, verified in simulator at 1920x1080 and 1280x720 |
| SVG gradients | `<LinearGradient>` and `<RadialGradient>` are nodes that draw nothing; a paint, collected from the whole `<Svg>`, turns `url(#id)` into an Evas VG gradient in the view's pixels, held in a container that is never the root (section 8). Bounding-box and user-space units, `gradientTransform`, fills and strokes; a linear gradient keeps its direction under any affine transform; an unevenly stretched radial one draws as a circle of the mean radius. Stops premultiplied with every opacity on the way down (`svg/SvgGradient.cpp`, `svg/SvgCanvas.cpp`) | done, verified in simulator beside Chrome |
| MaskedView | `RNCMaskedView` from @react-native-masked-view/masked-view, which has no codegen spec: a plain view's descriptor under that name (`fabric/CoreComponentDescriptors.cpp`) and its view config in JavaScript (`react-native-overrides/Libraries/ReactNative/requireNativeComponent.tizen.ts`). The first child is the mask: a proxy image of its smart object, source hidden, follows its geometry and is the clipper of every other child (`views/MaskedView.cpp`). The view's own background, border and shadow are not masked | done, verified in simulator |
| Line height | `linesize` in pixels per run when the font's own line is shorter. A `lineHeight` below it: `linerelsize` and a `linegap` that add back the truncation, from the font's ascent and descent read on an Evas text object (`text/FontMetrics.cpp`); the paragraph is measured from its first line's top to its last line's bottom, and drawn shifted so the baseline sits where CSS puts it, the shortfall split evenly (`text/LineSpan.cpp`, `views/ParagraphView.cpp`) | done, verified in simulator beside Chrome |
| Variable fonts | A font loaded through expo-font that has a `wght` axis is written as static instances at 100, 200 ... 900 within its range, every other axis at its default, by HarfBuzz's subsetter at first load (`text/FontInstances.cpp`, `text/HarfBuzzSubset.cpp`), cached under the app's cache by family and content, and the family is registered as those files alone (`expo/modules/ExpoFontLoaderModule.cpp`). Without HarfBuzz the variable file is registered as before. Optical size (`opsz`) stays at its default, where a browser follows the font size | done, verified in simulator beside Chrome |
| Max lines, ellipsis | `numberOfLines = 1`: no `wrap`, `ellipsis` 1.0 / 0.0 / 0.5 for tail, head and middle, none for clip. `numberOfLines > 1`: wrapped without `ellipsis` (its second path cuts after any line, section 5), measured to the bottom of line N - 1; the view deletes the text after line N with a textblock cursor and ends the last line with an ellipsis inside its own run (`text/LineClamp.cpp`; none for clip) | done, verified in simulator |
| Decorations, case, span background | `underline`, `strikethrough` (double underline with `underline2_color`), `backing`; `textTransform` applied to the string for ASCII, Latin-1 and Latin Extended-A (`text/TextCase.cpp`); a run's `opacity` multiplies its colors | done, verified in simulator |
| Letter spacing | A run with `letterSpacing` in a font the app registered is set in a copy of the file fontconfig would pick, its `hmtx` advances widened by `letterSpacing / fontSize` em in font units, chosen by path (section 5; `text/AppFonts.cpp`, `text/SfntTables.cpp`), so Evas measures, wraps and ellipsizes with the spacing. Copies are written once, beside the registered fonts. Spacing is added after every glyph, the last included, as Chromium does. System fonts (`Tizen`) keep no spacing | done, verified in simulator beside Chrome |
| Font size parity | `EVAS_FONT_DPI=72` before EFL initializes, so `font_size` equals pixels (verified on hardware below) | done |

### Bugs and risks in the current code

Paths are under `runtime/host/src/` unless stated.

| Where | Problem | Mark |
| --- | --- | --- |
| `views/ViewBox.cpp` (`update`) | Fixed: the background always hangs from the view's clipper, so the clipper never idles as a white rectangle | src |
| `views/ComponentView.cpp` (destructor) | Fixed: a view leaves its parent's children when destroyed, and detaches its own children before its smart object, which deletes its members, goes | src |
| `views/ComponentView.cpp` (`restack`) | Fixed: members are stacked shadows, background, content, children, border, outline, in that order | src |
| `views/ComponentView.cpp` (`updateProps`) | Opacity multiplies into each member separately; overlapping children inside a translucent view show through each other. That matches RN Android's default (`needsOffscreenAlphaCompositing` false), not iOS group opacity | inf |
| `views/ComponentView.cpp` (`place`) | Every move re-places the whole subtree, one geometry call per object: a ScrollView moves its content every animation frame. Plan long lists with windowing. A new subtree is placed once, when it joins the window | inf |
| `views/ViewTransform.cpp` | A mapped view renders its subtree into a surface: on the software engine every frame of a scale animation re-renders it, and text inside is resampled | inf |
| `views/BoxBorder.cpp` | Per-side border colors with rounded corners draw the whole ring in one color; dashed and dotted borders draw solid when the sides differ in style or the box passes 2048 px | src |
| `fabric/CoreComponentDescriptors.cpp` | ActivityIndicator still mounts as a plain view: it draws nothing | src |
| `text/TextblockMarkup.cpp` | Attachments are skipped and reported as zero-size, clipped (`runtime/platform/react/renderer/textlayoutmanager/TextLayoutManager.cpp`): inline views inside `<Text>` never show | src |
| `text/TextblockFormat.cpp` | `adjustsFontSizeToFit` and text shadows are ignored; `Justified` aligns left; `letterSpacing` in a system font is ignored | src |
| `views/ComponentView.cpp`, `views/ViewTransform.cpp` | Fixed: deleting a mapped view, or turning its map off, while the software render thread drew into the map's surface crashed in `_op_blend_pan_caa_dp_neon` (a faded detail-page backdrop scaled into a full-window map). Both now `evas_sync` first, as do the snapshot of a blur and the proxy of a mask (`views/RenderSync.h`). The GL engine renders on the main loop and never met it | verified in simulator |
| `text/TextblockFormat.cpp` | Text colors are written with straight alpha and textblock does not premultiply them. Translucent text looked right in the simulator's software engine; on the GL engine it is untested | unk |
| `text/EvasTextMeasurer.cpp` | Every uncached measurement still waits for the main loop: no longer for a whole mount, which runs it between two mutations, but for a frame render in progress (section 7: Evas cannot measure off the main loop). A text-heavy screen pays one round trip per paragraph and width it does not fit unwrapped | inf |
| `src/bundle/bundle-app.ts` (CLI) | `export:embed --assets-dest <out>/assets` writes `<out>/assets/assets/<path>/name.png`, the package copies that directory to `res/assets`, and React Native resolves a bundled image beside the bundle at `res/assets/<path>/name.png` (`Platform.OS` `tizen` takes `scaledAssetURLNearBundle`): `require('./image.png')` fails with "file does not exist" | verified in simulator |

## Verified on hardware

2024 Samsung The Frame (LS03D), Tizen 9.0, armv7 userland, 2026-11-02.

| Fact | How it was seen |
| --- | --- |
| The window renders with `wayland_egl` (GPU) when the manifest sets `hw-acceleration="on"` and the host calls `elm_config_accel_preference_set("opengl")` before `elm_win_add` | `ecore_evas_engine_name_get` logged at startup |
| `EVAS_FONT_DPI=72` takes effect when the .NET shell calls `rn_tizen_initialize()` (a `setenv`) at the top of `Main`, before `Run` starts EFL | the variable logged back at startup; text sizes look right |
| A clip rectangle that clips nothing draws as an opaque white rectangle over the view: every paragraph turned white until the clip clipped the view's own background too | seen on screen |
| Installed fonts include `One UI Sans APP`, `One UI Sans KR 300 Light` to `700 Bold`, the script families `SamsungOneUI<Script>` (Arabic, Thai, JP, SCN, TCN...) and `SamsungOneFallbackVD`. No family is named `Tizen` in `evas_font_available_list`, yet `font=Tizen` renders the system font, so it resolves as a fontconfig alias | font list logged at startup; text checked on screen |
| Retail sets deliver arrows and `Return` without a grab; `XF86Back` only once grabbed | see tizen-input.md |
| An Ecore animator ticks at 60 Hz while the app is idle (median tick interval, `rn-tizen perf`'s `hz`), 2026-11-03 | `rn-tizen perf --port 8102` on the home screen |
| `example/src/screens/rendering2` under `rn-tizen perf`, with the 1728x200 header at intensity 64 over a rail scrolled one card every 400 ms: 2.9 ms per frame and 19% CPU blurred (key 2), 2.0 ms and 17% tinted (key 3), 1.5 ms and 14% blurred beside the rail (key 5). The simulator's software engine needs 10.7, 3.0 and 2.6 ms. The log says "Evas filters resolved from libefl.so.1", 2026-11-03 | `rn-tizen perf --port 8104` |
| Painting 24 cards' `linear-gradient(160deg)` (six colors, so six patches the size of a card) took 218 ms on the main loop; the same screen mounts in under 30 ms once they are cached | key 2 then 3, `slowest to mount` |
| A focus glow's shadow patch costs 21 to 30 ms on the TV with three box blurs, about eight times the simulator's 2.6 to 3.7 ms. A screen-sized gradient backdrop costs 35 ms | temporary per-mutation log, KROMA's detail page and player |
| KROMA's first frames after launch take 170 to 280 ms each, once 536 to 597 ms, whether window alpha changes then or not. Text measurement waits behind them: about 1.4 s of the JavaScript thread's wait in the first seven seconds | `rn-tizen perf`, three cold starts, 2026-11-03 |
| Where KROMA's boot frames went, from Evas's event log and a 1 ms sample of the main thread's stack, six cold starts: expo-image views drawn before any decode, so each WebP decoded at its own size on the main loop during the frame (a 780x439 backdrop 63 to 81 ms, a 960x540 one 84 to 94 ms, a 512x512 logo in a 146x146 box 53 to 64 ms); texture uploads, 2 to 5 MB a frame in 23 to 72 ms; the first SVG icons waiting on ThorVG's worker threads inside `tvg::Paint::Impl::render` (6 to 107 ms for the first, 3 to 9 ms for each next); the buffer-age query blocking in `tpl_surface_dequeue_buffer_with_sync` and `tbm_surface_queue_can_dequeue_wait_timeout` for 15 to 141 ms while the compositor holds the buffers; up to 10 MB of render targets for a full-screen mapped view, allocated in 1 to 112 ms. No shader compiled: the four start programs came from `/home/owner/.cache/evas_gl_common_caches` (43 KB) in 1.1 to 1.6 ms, and every later one from the same cache | temporary probe: `eina_evlog`, GL calls timed through the engine modules' GOT, `tgkill` sampling; 2026-11-03 |
| Each glyph upload into the font atlas costs 0.15 to 0.27 ms: the Mali driver waits on a sync object (`osup_sync_object_wait`) before writing a texture the GPU still reads. KROMA's home, shown after its intro, uploads about 150 glyphs in its first frame, 23 to 41 ms | same probe |
| With expo-image decoding through a hidden preload (`expo/modules/image/ExpoImageView.cpp`), three cold starts each, before and after: render time in the first 7.5 s 642 to 940 ms then 305 to 549 ms; JavaScript's wait on the main loop in the first seven seconds 296 to 587 ms then 142 to 240 ms; worst commit 105 to 192 ms then 68 to 87 ms. Measured again once KROMA's home had more rows, with and without the preload: render 697 to 1049 ms then 572 to 1224 ms (medians 848 and 614), the wait 151 to 329 ms then 116 to 438 ms: from one cold start to the next the wait varies as much as the change. No WebP decode is left on the main loop | `rn-tizen perf --port 8105`, cold starts with `sdb shell 0 kill` and `tizen run` |
| Patch generation on the main loop, the same KROMA session before and after the third performance pass: the 1920x1080 backdrop gradient (a 683x384 patch) 58.8 ms then 4.8 to 8.0 ms; a 585x329 diagonal card gradient 35.1 ms then 3.7 to 3.8 ms; cut-to-corner card gradients 0.56 to 0.66 ms then 0.37 to 0.42 ms. Blur-50 focus glows 13.7 to 24.5 ms, then 8.0 to 13.9 ms on the painting thread and none on the main loop | temporary per-patch timer, `rn-tizen logs` |
| `example/src/screens/rendering2` key 4 (MaskedView: a text mask, a CSS radial mask, an Svg ramp, and an Svg edge fade over a rail that scrolls 1200 px in 250 ms every 1.5 s) over 17 s: 9.7 frames a second on average, 15 while the rail moves, none dropped; render 1.5 ms on average, 2.9 ms at the 95th percentile and worst; CPU 4.9 % on average, 6.8 % at worst; main loop busy 0.6 % | `rn-tizen perf --port 8104 --duration 20` |
| react-native-svg through Evas VG draws on the TV's renderer as in the simulator: 16 Tabler stroke icons, recoloured and resized icons, circle, ellipse, rounded rect, line caps, polyline joins, polygon, transforms, arcs and viewBox slice/none all looked right | `rn-tizen run --entry src/screens/svg/main.ts`, checked on screen |

## Verified in the simulator

Ubuntu 26.04 aarch64, EFL 1.28.1, `software_x11` engine, 2026-11-02. The
simulator runs upstream EFL, not Samsung's ThorVG-backed libevas, so each row
still needs a TV.

| Fact | How it was seen |
| --- | --- |
| The legacy VG API above draws react-native-svg: 16 Tabler outline icons (arcs in `search`, `settings`, `bell`, `user`; round caps and joins) match WebKit's rendering of the same SVG files | `example/src/screens/svg`, screenshot beside a Quick Look (WebKit) render of the icons' SVG files |
| An arc with a 30 degree x-axis rotation and radii scaled up to reach its end point, and a smooth quadratic (`T`), match WebKit | same comparison |
| Paths baked into the object's pixels and a view-sized transparent rectangle in the root draw at the right place and size at 24, 48, 64 and 96 px, with `viewBox` meet, slice and `none` | screenshot |
| A rectangle clipping the VG object crops content outside the `<Svg>` | screenshot |
| Caps butt, round and square; joins miter and round; translucent stroke over translucent fill | screenshot |

The rendering rows below come from the example app's rendering screens
(`example/src/screens/rendering/`, keys 1 to 6), at 1920x1080 and again with
the surface given a 1280x720 window.

| Fact | How it was seen |
| --- | --- |
| A smart class whose `move` is a no-op keeps its members in place, and views placed member by member render where Yoga put them | every screen |
| Nine-patch images draw rounded backgrounds, per-corner radii, rings, outlines and Gaussian shadows at any size, and a rounded image clipper clips children and images to the corners | box and image screens |
| A four-point map on a smart object renders scale, rotation and perspective around the transform origin; a geometry grown by the shadow and outline keeps both in the mapped surface | transform screen, focus scale 1.12 |
| ScrollView: `scrollTo` animated and not, `scrollToEnd`, clipping, `onScroll` offsets in points at both scales, `onMomentumScrollEnd` once per finished scroll | scroll screen |
| Images: an HTTPS JPEG, a `data:` PNG, a bundled PNG file (once placed where React Native looks, see the CLI bug above), every `resizeMode`, `onLoad` sizes, `onError` with "HTTP 404" | image screen |
| Textblock decorations, `backing`, `linesize`, `align` per run, single-line head, middle, tail and clip ellipses, Latin case transforms | text screen |
| Evas's `ellipsis=1.0` cut a wrapping two-line paragraph after its first line at 1920x1080 even with 116 px of room for 78 px of text, and not at 1280x720; clamping with a cursor draws two lines and the ellipsis at both | text screen, with a temporary log of the textblock's formatted size |
| A Modal declared inside a clipped, translucent view draws opaque and unclipped above everything; `onShow` fires once | modal screen |
| At a 1280x720 window, positions, sizes, radii, borders, shadows, text, images and scroll offsets scale by 2/3 with the layout unchanged | every screen |
| The SVG screen and the expo-image and expo-blur views, at a 1280x720 window (`RN_TIZEN_SIMULATOR_SCREEN_SIZE=1280x720`): icons, strokes, viewBox fits, image fits and tints scale by 2/3 and stay in their boxes | `--entry src/screens/svg/main.ts`, `--entry src/screens/expo/main.ts` |
| Back with a Modal open fires its `onRequestClose` once and `hardwareBackPress` not at all; Back again, with none open, reaches `BackHandler` | modal screen |
| `experimental_backgroundImage` (to bottom, `transparent` stops, 135deg in a rounded box, `to top right`, a hint, px stops, a circle, `58% 46% at 50% 50%`, `ellipse at top left`, two layers, inside a 6 px border's inner radius), `dashed` and `dotted` rounded borders, and react-native-svg linear and radial gradients as fills and a stroke, in bounding-box and user-space units, look as Chrome draws the same CSS and SVG | `--entry src/screens/gradients/main.ts`, screenshot beside Chrome's at 1920x1080 |
| KROMA's TV app ([`clients/tv-native`](https://github.com/kromatv/kroma/tree/1d270635adcc9f3a81283e04f06336845d8c5985/clients/tv-native)) beside its web Tizen shell in Chrome at 1920x1080: home, a focused rail card, films, a film and a series detail, genres, my list, settings, about and the profile picker line up (hero veils, card scrims, category washes, the avatar gradient, the dashed add tile; with `lineHeight` below the natural line, titles and the rows under them land on the same pixel rows) | screenshots of both, 2026-11-03 |

The rows below come from the example's second rendering screens
(`example/src/screens/rendering2/`, `--entry src/screens/rendering2/main.ts`,
keys 1 to 6), 2026-11-03.

| Fact | How it was seen |
| --- | --- |
| A snapshot under a `blur` filter blurs what is drawn below it, a rounded clip cuts it to the corners, the blur meets the box's edges with the pixels beyond them, and intensity 0 draws nothing | key 1, screenshot |
| Software engine, a 1728x200 dark header at intensity 64 (reach 29 px) over a rail scrolled one card every 400 ms: 10.7 ms per frame and 40% CPU blurred, 3.0 ms and 11% with the tint alone; the same header beside the moving rail: 2.6 ms and 5%, the tint's cost | `rn-tizen simulator perf`, keys 2, 3 and 5 |
| A MaskedView masks with a text, a CSS radial gradient and Svg linear gradients; an edge fade stays put while the rail under it scrolls, at 3.2 ms per frame | key 4, `rn-tizen simulator perf` |
| Hanken Grotesk, a variable font loaded from a URL: nine static weights written in 30 ms (19 ms for KROMA's copy); widths at 100 to 900 within 2.5 points of Chrome's for the same file; `letterSpacing` -0.64, 3.84 and 4.84 within 2.5 points; a spaced paragraph wraps at words and a spaced single line ends in an ellipsis | key 6, screenshot beside widths Chrome measured |
| Deleting a mapped view while the render thread drew into its map surface crashed in `_op_blend_pan_caa_dp_neon` within one restart-and-open cycle of KROMA's detail pages; with `evas_sync` first, 32 cycles ran clean | `rn-tizen simulator crash`, libevas symbolized with Ubuntu's debuginfod |

The rows below come from KROMA's TV app ([`clients/tv-native`](https://github.com/kromatv/kroma/tree/1d270635adcc9f3a81283e04f06336845d8c5985/clients/tv-native)) under
`rn-tizen simulator perf`, before and after the second performance pass,
2026-11-03.

| Fact | How it was seen |
| --- | --- |
| Shadow nine-patches cost a focused button 34 to 51 ms to mount with a direct Gaussian. Three box blurs and sampling only edge pixels take 2 to 4 ms; eight shadows from blur 3 to 120 went from 702 ms to 17 ms on a Mac. From a deviation of 3 (blur 6), the patches stay within 3.6% of the Gaussian per pixel (1.2 to 2% from blur 24); below it the direct Gaussian still runs, and a play button's glow on screen within 1 of 255 | a temporary per-mutation log, a standalone comparison of `shadowPatch`, screenshots |
| Boot mounted its 278 views in 97.6 ms, then 24 to 48 ms; a player opening its 344 in 70.3 ms, then 22.6 ms. A focus move along a rail mounts in 0.2 to 0.7 ms | `slowest to mount` |
| Placing a subtree once, when it joins the root, cut `place` calls from 1.7 per mutation to 0.45 | a temporary counter |
| The JavaScript thread waited 50.6 ms for text measurement while a player opened (worst commit 49.1 ms), then 2.3 ms (1.4 ms); at boot 57 ms, then 15 to 29 ms. Before, it waited 5.7 times the measuring work, since a measurement queued behind whole mounts; after, 1.3 times. About one measurement in nine is answered by `UnwrappedSizes` | `jsWait`, `commitMax`, temporary counters |
| `example/src/screens/rendering2` keys 1, 4 and 6 draw the same pixels as before these changes | screenshots compared pixel by pixel |

## Backdrop blur

What expo-blur's `BlurView` needs: the pixels already drawn behind it,
blurred, under its tint. `ExpoBlurView` now draws a snapshot under a `blur`
filter, with the tint above it (`expo/modules/blur/`, `views/BackdropBlur.cpp`,
`views/EvasFilter.cpp`); the research below led there. Before, it drew the tint alone, expo-blur's
own fallback for a platform without blur
(`runtime/host/expo/modules/blur/ExpoBlurViews.cpp`). Checked on 2026-11-03
against EFL 1.25.1 and the Tizen 9 rootstrap; nothing here has run on a TV.

### What Evas has

| Fact | Mark | Source |
| --- | --- | --- |
| `evas_object_image_snapshot_set(obj, EINA_TRUE)` makes an image object hold "the content below" it, to "reuse it as a source for any kind of effect". Public since 1.15, outside any beta guard on Tizen 9 | doc | `rs:usr/include/evas-1/Evas_Legacy.h:6085-6095`, `efl:src/lib/evas/canvas/efl_canvas_snapshot.eo#L1-L12` |
| The TV's libevas exports `evas_object_image_snapshot_set` and `efl_canvas_snapshot_class_get` | src | `rs:libevas` dynamic symbols |
| A filter is a Lua program run "whenever the object is rendered", "valid for Text and Image objects": `efl_gfx_filter_program_set(obj, code, name)` | doc | `rs:usr/include/efl-1/interfaces/efl_gfx_filter.eo.h:20-50` |
| That header is beta (`EFL_BETA_API_SUPPORT`, line 16), and the function's body is in libefl (`efl_interfaces_main.c` includes `efl_gfx_filter.eo.c`). The Tizen 9 and 10 rootstraps ship no `libefl.so`, though `efl.pc` asks for `-lefl` and libevas and libedje name `libefl.so.1`: an app cannot link the call, only resolve it at run time | src | `efl:src/lib/efl/interfaces/efl_interfaces_main.c#L44`, `rs:usr/lib/pkgconfig/efl.pc`, `strings rs:libevas` |
| The filter language is built into the TV's libevas: it names `liblua.so.5.1` and exports `evas_filter_program_parse` | src | `rs:libevas` |
| The legacy text filter setter `evas_object_text_filter_program_set` is 6 bytes long on the TV: a stub | src | `rs:libevas` symbol size |
| `blur ({ rx = 3, ry = nil, type = 'default', ox = 0, oy = 0, color = 'white', src = input, dst = output })`. `default` "is recommended in all situations": it picks box or Gaussian by size, and box is "4x to more than 20x faster" | doc | `efl:src/lib/evas/filters/evas_filter_parser.c#L1088-L1126` |
| EFL's own test is this feature: a snapshot over a table of moving images with the program `padding_set {0} ... blur { radius }`, the radius changed at run time with `efl_gfx_filter_data_set(snap, "radius", "tonumber(N)", EINA_TRUE)` | src | `efl:src/bin/elementary/test_evas_snapshot.c#L22-L27`, `#L95`, `#L148-L152` |
| Edje reaches it through public API only: an EDC part of type `SNAPSHOT` with `filter.code`, compiled by `edje_cc` (in Tizen Studio's `platforms/tizen-9.0/common/efl-tool`) and loaded with `edje_object_file_set`; Edje makes both calls itself. The radius is then fixed per EDC state | doc + src | `efl:src/bin/edje/edje_cc_handlers.c#L13085-L13135`, `efl:src/lib/edje/edje_load.c#L988-L990`, `efl:src/lib/edje/edje_calc.c#L2598` |

### What a frame costs

| Fact | Mark | Source |
| --- | --- | --- |
| Before a frame, a snapshot is marked dirty when any non-smart object below it changed inside its rectangle, when it has no surface yet, or when it moved or resized. An opaque object above that covers it whole skips it | src | `efl:src/lib/evas/canvas/evas_render.c#L2846-L2928` |
| A dirty snapshot re-renders every object below it, inside the update region, into its own surface before the frame's objects are drawn: one more offscreen pass per snapshot | src | same file `#L3546-L3602`, then `#L3605` |
| "FIXME: We should render snapshots only once per frame, not once per update region per output!" | src | same file `#L3563-L3564` |
| The filter's output is kept and drawn as an image while nothing changes; the filter runs again only on a state change, a content change, a snapshot redraw, an obscured-region change or a changed proxy source | src | `efl:src/lib/evas/canvas/evas_filter_mixin.c#L310-L362` |
| The GL engine runs blur when its filter table has the command, otherwise the software one does | src | `efl:src/modules/evas/engines/gl_generic/evas_engine.c#L3003-L3011`, `efl:src/lib/evas/filters/evas_filter.c#L889-L944` |
| GL blur: downscale into a temporary buffer, blur X into a second, blur Y back, upscale into the output. Only for `default` blurs, by a power of two from the radius r, capped at 4 "for perfect picture quality": none below r = 6, 2x from 6 to 17, 4x from 18 | src | `efl:src/lib/evas/filters/evas_filter.c#L746-L880` (downscale at `#L777-L822`) |
| Each pass samples about r / (2 x downscale) + 1 texels per pixel (two weights per bilinear fetch), from a weight texture uploaded when the radius changes | src | `efl:src/modules/evas/engines/gl_generic/filters/gl_filter_blur.c#L46-L81`, `efl:src/modules/evas/engines/gl_common/evas_gl_context.c#L3533-L3660` |
| The software path has no downscale: box blurs at full size on the CPU, which is what the simulator runs | src | `efl:src/lib/evas/filters/evas_filter.c#L946-L949` |
| A snapshot sees Evas objects only. The video plane lies under the window, so a `BlurView` over video blurs the transparent hole and the picture shows through unblurred | inf | tizen-media-playback.md, section 3 |
| expo-blur on Android divides `intensity` (0 to 100) by `blurReductionFactor`, 4 by default, to get a radius | doc | expo-blur 58.0.5 `build/BlurView.types.d.ts` (`blurReductionFactor`) |

### Moving content at 60 fps

A rail scrolling under a blurred header changes objects below the snapshot
every frame, so every frame pays for it. For a 1920x200 px header at
intensity 100 (radius 25 px, 4x downscale, 5 samples per pass), at a
1920x1080 window (inf, arithmetic on the rows above):

| Step | Pixels per frame |
| --- | --- |
| Draw the region below a second time, at an overdraw of 2 to 3 | 0.8 to 1.2 M |
| Downscale to 481x51, then two blur passes | 0.4 M reads, 0.25 M samples |
| Upscale, blend, tint | 1.15 M |
| Total | 2.5 to 3 M, about one more full 1080p screen, 150 to 180 M px/s at 60 fps |

Plus about five render-target switches per frame (the snapshot, two temporary
buffers, the filter output, the window). Samsung does not disclose the TV's
GPU, and on a tile-based GPU each switch also writes the tiles out and reads
them back. Whether a TV holds 60 fps with it is unknown: measure. At a
3840x2160 window every figure is four times larger (tizen-tv-platform.md,
section 7).

Cheaper ways, in order:

1. **Blur once, cache.** Already what Evas does when nothing under the
   snapshot moves (the mixin row): a header over a still backdrop costs one
   blur per change.
2. **Freeze while moving.** The host moves content itself (ScrollView offsets,
   `place`), so it can hide the snapshot and keep the tint while a scroll under
   a `BlurView` animates, and show it again at the end: one blur per scroll.
3. **Larger radius, smaller area.** From 18 px the work runs at a quarter
   size; blur only the header's height.
4. **Blur the image, not the screen.** For a poster behind a detail page,
   decode it at a quarter or an eighth (`load_size_set`), blur on the CPU with
   the Gaussian `views/BoxShadows.cpp` already has, cache by source and radius,
   and stretch it with smooth scaling. No snapshot at all; the same code serves
   `Image`'s and expo-image's `blurRadius`.

### For `ExpoBlurView`

| Rule | Why |
| --- | --- |
| A snapshot image as the view's bottom member, the tint rectangle above it as today, so expo-blur's tint colours stay right | rows above |
| Program `padding_set {0} blur { radius }`, the radius set with `efl_gfx_filter_data_set` rather than a new program | EFL's test |
| `efl_gfx_filter_program_set` and `efl_gfx_filter_data_set` through `dlopen("libefl.so.1")` and `dlsym`, declared by us; tint only when either is missing | libefl is not in the rootstrap; same rule as section 8's VG functions |
| Grow the snapshot by the radius on each side and clip it to the view, or the blur fades to transparent at the edges (`padding_set {0}` samples nothing outside) | verified in simulator |
| Freeze the snapshot while content under it scrolls, until frame times on a TV say otherwise | the estimate above; not built, see below |

What was built, and why:

| Fact | Mark | Source |
| --- | --- | --- |
| expo-blur on Android blurs by `intensity / blurReductionFactor` and lays `TintStyle.toBlurEffect(intensity)` over it, the same color its no-blur path paints: `dark` 25 at 0.69, `light` 249 at 0.78, `default` 255 at 0.44 of the intensity, and so on. `ExpoBlurView` takes that table, `blurReductionFactor` included, and blurs whatever `blurMethod` says, as iOS does | src | `xb:android/src/main/java/expo/modules/blur/ExpoBlurView.kt` (`applyBlurViewRadiusCompat`, `configureBlurView`), `xb:android/.../enums/TintStyle.kt` |
| Android blurs a radius r with a Gaussian of sigma 0.57735 r + 0.5 (`RenderEffect.createBlurEffect`) | src | `aosp:libs/hwui/jni/RenderEffect.cpp` (`createBlurEffect`), `aosp:libs/hwui/utils/Blur.cpp` (`convertRadiusToSigma`) |
| Evas's `default` blur of radius R past 6 px is three box blurs whose radii add up to R, a reach of about three sigmas; so R = 3 x sigma, in pixels at the point scale (`expo/modules/blur/BlurTint.cpp`) | src + inf | `efl:src/modules/evas/engines/software_generic/filters/evas_filter_blur.c#L8-L35`, `efl:src/lib/evas/filters/evas_filter.c#L966-L1025` |
| Edje's `SNAPSHOT` filter calls the same `efl_gfx_filter_program_set` and `efl_gfx_filter_data_set`, and libedje names `libefl.so.1`, so a TV whose libefl lacks them has no Edje blur either: the fallback was not built | src | `efl:src/lib/edje/edje_calc.c#L2527-L2598`, `strings rs:usr/lib/libedje.so.1.25.1` |
| React Native 0.88 parses `filter` (blur included) into `BaseViewProps::filter` and has no backdrop filter prop at any prefix; `filter: blur()` blurs the view's own content, not what is behind it, and is not drawn | src | `RC:react/renderer/components/view/BaseViewProps.h:89-90`, `BaseViewProps.cpp:278-279` |

### Verify on a TV

On the 2024 Samsung The Frame (LS03D), Tizen 9.0:

1. The log says "Evas filters resolved from libefl.so.1", not "Evas filters
   unavailable".
2. `rn-tizen run --entry src/screens/rendering2/main.ts` from the example,
   key 1: a screenshot through `rn-tizen` shows the four panels blurred and
   rounded as in the simulator, on `wayland_egl`.
3. Keys 2, 3 and 5 under `rn-tizen perf`: frame times with the header blurred
   over the moving rail, tinted only, and blurred beside it.
4. A `BlurView` over a playing video, and key 4 (masks) and key 6 (weights,
   letter spacing) beside the simulator's screenshots.
5. Record each result in "Verified on hardware" with the TV, year and Tizen
   version.

Steps 1 and 3 are done (2026-11-03, "Verified on hardware").
