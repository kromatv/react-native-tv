# Changelog

## [0.1.0-alpha.4](https://github.com/kromatv/react-native-tv/compare/react-native-webos-v0.1.0-alpha.3...react-native-webos-v0.1.0-alpha.4) (2026-11-13)


### Documentation

* point the changelog's links at this repository ([#10](https://github.com/kromatv/react-native-tv/issues/10)) ([c6a2825](https://github.com/kromatv/react-native-tv/commit/c6a282524de244731100f428b8084f248cdb9666))

## [0.1.0-alpha.3](https://github.com/kromatv/react-native-tv/compare/react-native-webos-v0.1.0-alpha.2...react-native-webos-v0.1.0-alpha.3) (2026-11-12)


### Features

* **expo:** a package that groups TV platforms can run this one and own the t key (kromatv/react-native-webos#20) ([b800bc5](https://github.com/kromatv/react-native-tv/commit/b800bc55f30b9dfa460de684a61d2c497a260a53))
* **metro:** build an app whose config plugin hands Expo the Metro config (kromatv/react-native-webos#22) ([c9649f8](https://github.com/kromatv/react-native-tv/commit/c9649f8aa881e28d46a903b870abb95b1dd5cbf0))

## [0.1.0-alpha.2](https://github.com/kromatv/react-native-tv/compare/react-native-webos-v0.1.0-alpha.1...react-native-webos-v0.1.0-alpha.2) (2026-11-12)


### Features

* **metro:** add every TV platform an app depends on with one withTV call (kromatv/react-native-webos#19) ([7815a7a](https://github.com/kromatv/react-native-tv/commit/7815a7a965b21ff7322c95396700372de0bdc3bd))


### Bug fixes

* **metro:** bundle a second checkout of an app that shares Metro's cache (kromatv/react-native-webos#16) ([1330748](https://github.com/kromatv/react-native-tv/commit/133074850242a699c6df8479e0d50f8375c53096))
* **metro:** require the list wrapper's style by name, so a transform cache shared by two checkouts bundles in both ([1330748](https://github.com/kromatv/react-native-tv/commit/133074850242a699c6df8479e0d50f8375c53096))
* **video:** show the picture of a player that replaces another on a view (kromatv/react-native-webos#18) ([e30a25e](https://github.com/kromatv/react-native-tv/commit/e30a25ee064b8bcce79dbe7d1915423ef31b53c6))

## [0.1.0-alpha.1](https://github.com/kromatv/react-native-tv/releases/tag/react-native-webos-v0.1.0-alpha.1) (2026-11-11)


### Features

* **accessibility:** report the TV's audio guidance as the screen reader ([93b90d7](https://github.com/kromatv/react-native-tv/commit/93b90d7fb4d8b3ef217363744c1e29d8795ee99e))
* **cli:** package, install and drive a webOS TV, and the simulator's pointer ([fbe63ef](https://github.com/kromatv/react-native-tv/commit/fbe63ef6057298620ba6a317f4de93c5739b6e61))
* **devsupport:** show the loading banner in the scene and run the dev tooling on the main loop ([ee7685d](https://github.com/kromatv/react-native-tv/commit/ee7685d4563776d742f92f55ab562bc44fcebc45))
* **diagnostics:** capture the scene and time its frames for rn-webos screenshot and perf ([ccd769c](https://github.com/kromatv/react-native-tv/commit/ccd769ced3f545d31866c6ca03d0115948da63b5))
* **dialog:** draw Alert.alert with Skia, as a layer of the scene ([0c6f40d](https://github.com/kromatv/react-native-tv/commit/0c6f40d2ad82ac1881df31bd9b8f0d4e4e179445))
* **expo:** keep awake through SDL's screen saver switch, fonts through the text engine ([5009de7](https://github.com/kromatv/react-native-tv/commit/5009de73c2e7d7077795c638bd08776d3a75ecc2))
* **host:** bring back the alert dialog, the control link, the perf monitor and the crash reporter ([d5629a7](https://github.com/kromatv/react-native-tv/commit/d5629a741bcc14ee3a2fb46159bf4ba0f9141189))
* **host:** register with the app manager, deliver launches and read the TV's profile ([da57526](https://github.com/kromatv/react-native-tv/commit/da575262e6510283f2d5f401bcb4c99a99195049))
* **input:** hover, press and scroll with the remote's pointer ([352a8a7](https://github.com/kromatv/react-native-tv/commit/352a8a70fdd430322179d190037a9a76cfea0c9e))
* **network:** compile libcurl and Mbed TLS into the runtime ([b15073d](https://github.com/kromatv/react-native-tv/commit/b15073d1438e0d8e9258fe9e5d338963c13c071a))
* **platform:** put the Luna bus on a transport tests can replace ([afb961f](https://github.com/kromatv/react-native-tv/commit/afb961ff1d33ddf2d3ca9d3253f9e3321d8dc0f8))
* **platform:** report the main loop's waits and a frame's flush apart from its swap ([d281b41](https://github.com/kromatv/react-native-tv/commit/d281b4155ed34e1acf377e0457f26b50dbde048b))
* **runtime:** add the Luna bus bridge, the simulator control channel and a test target that builds ([a3b6aba](https://github.com/kromatv/react-native-tv/commit/a3b6aba75b3f5eb25842c0207593a9ff2506f5ca))
* **runtime:** draw with Skia in an SDL window, from one executable ([ec6283c](https://github.com/kromatv/react-native-tv/commit/ec6283cec6f0b11480bb8329d2e32cb28127af58))
* **scripts:** check a TV executable against firmware symbol data ([e3efce4](https://github.com/kromatv/react-native-tv/commit/e3efce43ec1563f1122a7557794599f1f5fc01e3))
* **simulator:** launch and relaunch the app with parameters, and match crashes of an app started with them ([280f67f](https://github.com/kromatv/react-native-tv/commit/280f67f847b855f9d4791f529a42eb588317ba5d))
* **simulator:** start the app on a Full HD TV's 1280x720 screen with --screen ([f72c88b](https://github.com/kromatv/react-native-tv/commit/f72c88ba938220bc11448b84e5cac5c21bd1c956))
* **simulator:** type text into the focused field from the CLI ([c752934](https://github.com/kromatv/react-native-tv/commit/c75293468b82db261f135f5a753cbe5cfb8e805c))
* **svg:** draw react-native-svg with Skia ([4b55845](https://github.com/kromatv/react-native-tv/commit/4b558453faa1d67c7076a7f4f5355a68662120e7))
* **svg:** paint with Pattern ([17f3624](https://github.com/kromatv/react-native-tv/commit/17f3624d786d84add583fc1a105d9dac952246de))
* **text-input:** draw the field with SkParagraph and type through SDL text input ([ff2ca61](https://github.com/kromatv/react-native-tv/commit/ff2ca61b9818a2e8d196aa6bd84f8699e636a6b0))
* **text:** align text on its baseline and report its lines to onTextLayout ([af97239](https://github.com/kromatv/react-native-tv/commit/af97239b6b578c94b389f9f244c9437b038182e9))
* **video:** play through the TV's media pipeline, with a simulated one for the simulator ([68d7717](https://github.com/kromatv/react-native-tv/commit/68d7717c6bbad199a9cb880cc903d7e250b08de5))
* **views:** decode images off the main thread and draw images, masks and backdrop blur with Skia ([6526029](https://github.com/kromatv/react-native-tv/commit/6526029e51af58d82760d15f25fead818a165a61))
* **views:** paint CSS gradients with Skia shaders ([52980da](https://github.com/kromatv/react-native-tv/commit/52980dac312e5e0af5a7e691ad07dbbf15735b76))
* **views:** scroll with Skia and send Back to the topmost Modal ([d425557](https://github.com/kromatv/react-native-tv/commit/d4255570a53f316c52cffefcc0d2ecd29e30a53d))


### Bug fixes

* **cli:** take a command's options before, between or after what it acts on ([7f33c30](https://github.com/kromatv/react-native-tv/commit/7f33c30bf3f9bf6b12b63861bd73475bec25dce4))
* **devtools:** keep a TV's passphrase out of the job record ([e986466](https://github.com/kromatv/react-native-tv/commit/e986466476af07f995000d3d32e4969b6695e401))
* **init:** keep an override that already names the app's react-native, and a manifest's escapes ([763c8bb](https://github.com/kromatv/react-native-tv/commit/763c8bbe2501524b11195ecdfb671f8d541b409c))
* **logs:** name the frames of a native crash on a TV ([8ebaa02](https://github.com/kromatv/react-native-tv/commit/8ebaa02e122210471e2546b86a8250ab752d0038))
* **package:** declare the window transparent for an app that depends on expo-video ([ccf29a4](https://github.com/kromatv/react-native-tv/commit/ccf29a441907e07b9284cbc6430a7c4ecd850c22))
* **perf:** report the memory peak without Samsung's app budget ([13aecc0](https://github.com/kromatv/react-native-tv/commit/13aecc08746ec2da99ad1632191af33ffb761583))
* **platform:** drop the loop's pending work before the loop dies ([2387b42](https://github.com/kromatv/react-native-tv/commit/2387b42623dc43723025672859899fd69a060c6a))
* **simulator:** take a control command and its argument as one word or two ([52077a3](https://github.com/kromatv/react-native-tv/commit/52077a359ef25aad638254be066a126667878f25))
* **text:** measure a text with no glyphs as zero wide ([805fb8c](https://github.com/kromatv/react-native-tv/commit/805fb8cbdb9a0dbfaaf22e27684e7cf69c211deb))
* **video:** unload without the main loop when the closing app drops a player ([99a6c11](https://github.com/kromatv/react-native-tv/commit/99a6c1118403e2db12f9cbfd7d51a3149ddb2426))
* **views:** keep hitTest as the view's own name for the layer lookup ([2bff0ea](https://github.com/kromatv/react-native-tv/commit/2bff0eaa6001e21935f47fbdf9dcfea021bf16e8))
* **views:** turn a view about its transformOrigin ([9d1ad76](https://github.com/kromatv/react-native-tv/commit/9d1ad76282f437a4da67dd3d0107f117666e021d))


### Performance

* **package:** ship the executable without its symbol table ([b495b8f](https://github.com/kromatv/react-native-tv/commit/b495b8fcd254222be8bc94a271a46d46e66c6512))
* **platform:** pace frames from when each might have started ([48fb232](https://github.com/kromatv/react-native-tv/commit/48fb232f4608aebc4e1c68a16ac9364d85ae661c))
* **render:** keep compiled shaders between runs, bound Skia's GPU cache, and free memory while hidden ([8830706](https://github.com/kromatv/react-native-tv/commit/8830706cb3129b14635d96dc392071cd26bd6b95))
* **runtime:** make the TV executable 1.6 MB smaller ([f1555b5](https://github.com/kromatv/react-native-tv/commit/f1555b5f0904af9991ac2b16dccbf41850c53862))
* **svg:** keep the recorded drawing when only the Svg view's own style changes ([66e63b9](https://github.com/kromatv/react-native-tv/commit/66e63b975ef7e96aba00fd0e7ed9fa532d81acac))
* **svg:** play a recorded picture, and keep a still one as an image ([168cbd3](https://github.com/kromatv/react-native-tv/commit/168cbd3a78cdea8d9f47377735a21d4c059f9a83))
* **text:** lay a paragraph out once, on the layout thread, and play it in the view ([1eb2061](https://github.com/kromatv/react-native-tv/commit/1eb2061972810e35bbcf52dca8ac91f8de1337e2))
* **views:** cull by the bounds a subtree really paints, and test the ported views ([bae8b75](https://github.com/kromatv/react-native-tv/commit/bae8b7554806b00fefb0fe878f475f780890aef8))


### Documentation

* **benchmarks:** measure the example in the simulator, and say what that does not show ([51d3c74](https://github.com/kromatv/react-native-tv/commit/51d3c74fdfff39f695f95f5c9640e1873e9890a8))
* **cli:** show perf's memory column without a budget ([cb1b4d2](https://github.com/kromatv/react-native-tv/commit/cb1b4d2e5f7aceff36bb5d3afb1ece1cd297ba2b))
* count the C++ tests as they are ([81cb5e6](https://github.com/kromatv/react-native-tv/commit/81cb5e6ebd1ab8ccbc79821c798cb6df6a289dd2))
* describe contributing, security scope and code analysis for webOS ([2006090](https://github.com/kromatv/react-native-tv/commit/2006090da4f80191f00f572fdddc20b18c77ef63))
* **notices:** list Skia, SDL and the libraries they bring, with their licence texts ([b261db4](https://github.com/kromatv/react-native-tv/commit/b261db41bb89ce9d29233f66a481d5615c0b2d00))
* record the evening's TV runs, a player swap that shows no picture, and what the lab asks of a seller (kromatv/react-native-webos#15) ([91d5684](https://github.com/kromatv/react-native-tv/commit/91d5684dc84c5bd88d8454750428d91e7d695479))
* record what ran on two TVs of LG's Cloud Test Lab ([f08b1fa](https://github.com/kromatv/react-native-tv/commit/f08b1facdfcdc1247e28c456d267b1be09523636))
* **reference:** record how LG's webOS TV Simulator starts an app ([84011ae](https://github.com/kromatv/react-native-tv/commit/84011aebb222099de2abec94b5e44a3f9a27e3a9))
* **reference:** record how the media client reaches the bus and what the video module builds ([0c4cfed](https://github.com/kromatv/react-native-tv/commit/0c4cfed9bb39b0709f7a05fc021a9b5323b6f11d))
* **reference:** record LG's Cloud Test Lab, the store's native file type and other ways to reach a real TV ([0f1c5a2](https://github.com/kromatv/react-native-tv/commit/0f1c5a2965002289f451207a3b224900e45eed17))
* **reference:** record the app registration, Back at the root and the deep link parameter ([6e10693](https://github.com/kromatv/react-native-tv/commit/6e10693cb15f3375445090a03183cbf7b9f3c23e))
* **reference:** record what Seller Lounge's upload form does with a native package ([645023f](https://github.com/kromatv/react-native-tv/commit/645023f3bfeb17bd25b8d5848c6fefc892c0bb0c))
* **reference:** say how screenshots and perf samples are taken, and what the simulator showed ([e60dd1e](https://github.com/kromatv/react-native-tv/commit/e60dd1e1e8619c5601e4399a4982366ce72c7760))
* **reference:** say which observations came from react-native-tizen ([37cce8b](https://github.com/kromatv/react-native-tv/commit/37cce8bac664f34bf8f32dfa2c8968323cd567e8))
* rewrite the README for webOS, with what is and is not verified ([c9e4f17](https://github.com/kromatv/react-native-tv/commit/c9e4f17a0d2e7bbf391261715e1356303b35157d))
* say that LG's own simulator cannot run the runtime, and what it did with the package ([69e938c](https://github.com/kromatv/react-native-tv/commit/69e938c1b23b7c09fee908f7b7dfab702f175dd6))
* say what ran on a TV, and drop the Tizen text left in the reference (kromatv/react-native-webos#8) ([3d785ec](https://github.com/kromatv/react-native-tv/commit/3d785ecbde92a98a494e19bc7cbb6b390a4c24fc))
* **svg:** record what react-native-svg draws on Skia, and the rules behind it ([3e9d407](https://github.com/kromatv/react-native-tv/commit/3e9d407d2b77252f40689ced7c488de00b545649))
* **text-input:** describe the Skia field and SDL text input, and what SDL cannot say ([d51185e](https://github.com/kromatv/react-native-tv/commit/d51185e27a6f5ba16dbaec73ddb0f82fdae13be3))

## Changelog
