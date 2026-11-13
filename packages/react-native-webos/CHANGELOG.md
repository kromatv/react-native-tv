# Changelog

## [0.1.0-alpha.3](https://github.com/kromatv/react-native-webos/compare/v0.1.0-alpha.2...v0.1.0-alpha.3) (2026-11-12)


### Features

* **expo:** a package that groups TV platforms can run this one and own the t key ([#20](https://github.com/kromatv/react-native-webos/issues/20)) ([d95480c](https://github.com/kromatv/react-native-webos/commit/d95480cd5e25e0392763961a18d8674b9baca2d9))
* **metro:** build an app whose config plugin hands Expo the Metro config ([#22](https://github.com/kromatv/react-native-webos/issues/22)) ([fe25260](https://github.com/kromatv/react-native-webos/commit/fe252607ddac70e992b855abb98e51887472935f))

## [0.1.0-alpha.2](https://github.com/kromatv/react-native-webos/compare/v0.1.0-alpha.1...v0.1.0-alpha.2) (2026-11-12)


### Features

* **metro:** add every TV platform an app depends on with one withTV call ([#19](https://github.com/kromatv/react-native-webos/issues/19)) ([6f173b2](https://github.com/kromatv/react-native-webos/commit/6f173b2e6557d38b251a7b17dbaebfc82e4851e5))


### Bug fixes

* **metro:** bundle a second checkout of an app that shares Metro's cache ([#16](https://github.com/kromatv/react-native-webos/issues/16)) ([857340b](https://github.com/kromatv/react-native-webos/commit/857340b81c2e30fd803269fde5c60453c878b8bd))
* **metro:** require the list wrapper's style by name, so a transform cache shared by two checkouts bundles in both ([857340b](https://github.com/kromatv/react-native-webos/commit/857340b81c2e30fd803269fde5c60453c878b8bd))
* **video:** show the picture of a player that replaces another on a view ([#18](https://github.com/kromatv/react-native-webos/issues/18)) ([7307826](https://github.com/kromatv/react-native-webos/commit/7307826da50c392cb8f9ba5a624f3f6dbaae928f))

## [0.1.0-alpha.1](https://github.com/kromatv/react-native-webos/compare/v0.1.0-alpha.0...v0.1.0-alpha.1) (2026-11-11)


### Features

* **accessibility:** report the TV's audio guidance as the screen reader ([89da3ce](https://github.com/kromatv/react-native-webos/commit/89da3ce2c6815819dfb10e58fff919ca6ee146da))
* **cli:** package, install and drive a webOS TV, and the simulator's pointer ([395b850](https://github.com/kromatv/react-native-webos/commit/395b85077001e9e9e49a6b469f49777f94c2dd64))
* **devsupport:** show the loading banner in the scene and run the dev tooling on the main loop ([54952e5](https://github.com/kromatv/react-native-webos/commit/54952e50193e796441a9a8fbb25f6052f5c75718))
* **diagnostics:** capture the scene and time its frames for rn-webos screenshot and perf ([aed1311](https://github.com/kromatv/react-native-webos/commit/aed13116a78edec030526f2a80a954b90682f2db))
* **dialog:** draw Alert.alert with Skia, as a layer of the scene ([7033068](https://github.com/kromatv/react-native-webos/commit/7033068edce40bac2a1996b32f3bbe14b4df3ba7))
* **expo:** keep awake through SDL's screen saver switch, fonts through the text engine ([26ba3b8](https://github.com/kromatv/react-native-webos/commit/26ba3b8e7683b77e288f63e4bdb17a15ad390cb0))
* **host:** bring back the alert dialog, the control link, the perf monitor and the crash reporter ([7b94b56](https://github.com/kromatv/react-native-webos/commit/7b94b56ce419b7bf3e54f96bfe203ce11946f222))
* **host:** register with the app manager, deliver launches and read the TV's profile ([c8011a6](https://github.com/kromatv/react-native-webos/commit/c8011a62455171207fb10d395384b0b2a29dc3d4))
* **input:** hover, press and scroll with the remote's pointer ([3e61b60](https://github.com/kromatv/react-native-webos/commit/3e61b60dc747c175a4640e13f48499ace6bba645))
* **network:** compile libcurl and Mbed TLS into the runtime ([db247ce](https://github.com/kromatv/react-native-webos/commit/db247ce6c7a8bf1d1dac6c0eae62cebcafa7d157))
* **platform:** put the Luna bus on a transport tests can replace ([00eb7e5](https://github.com/kromatv/react-native-webos/commit/00eb7e5d8e72caeb3da97436de4e3b276bdb9fe9))
* **platform:** report the main loop's waits and a frame's flush apart from its swap ([80038b7](https://github.com/kromatv/react-native-webos/commit/80038b71801f94ec8687ab792d4545db4217dc2d))
* **runtime:** add the Luna bus bridge, the simulator control channel and a test target that builds ([320fbce](https://github.com/kromatv/react-native-webos/commit/320fbce4a12006584e658240bfaf72eb0ecc5097))
* **runtime:** draw with Skia in an SDL window, from one executable ([3e6f9ae](https://github.com/kromatv/react-native-webos/commit/3e6f9ae10c93c31c562f31482692680ecd4a1117))
* **scripts:** check a TV executable against firmware symbol data ([22cc1a4](https://github.com/kromatv/react-native-webos/commit/22cc1a46105c8dfab1deb22cc5afc2e3c8fd98f1))
* **simulator:** launch and relaunch the app with parameters, and match crashes of an app started with them ([b4ecf19](https://github.com/kromatv/react-native-webos/commit/b4ecf196262c3774d750d4c3bbe0acb36dff81fa))
* **simulator:** start the app on a Full HD TV's 1280x720 screen with --screen ([986510c](https://github.com/kromatv/react-native-webos/commit/986510c85e01ed48f417d181a4bb301ada4e1020))
* **simulator:** type text into the focused field from the CLI ([9fbb5cf](https://github.com/kromatv/react-native-webos/commit/9fbb5cf30e93122df28ad16219a115ceddc1cefb))
* **svg:** draw react-native-svg with Skia ([a8c54bd](https://github.com/kromatv/react-native-webos/commit/a8c54bd6499b0211d84a32d541f5e51d87bb93a7))
* **svg:** paint with Pattern ([fdb5435](https://github.com/kromatv/react-native-webos/commit/fdb54358c97677332b91abf8254249dce8287dd3))
* **text-input:** draw the field with SkParagraph and type through SDL text input ([ad4fb1b](https://github.com/kromatv/react-native-webos/commit/ad4fb1b36533ecd3af5f3e35f62995463a3eded7))
* **text:** align text on its baseline and report its lines to onTextLayout ([ad71fd3](https://github.com/kromatv/react-native-webos/commit/ad71fd34afb4d191ae21fa584cd33e599cd02517))
* **video:** play through the TV's media pipeline, with a simulated one for the simulator ([7536340](https://github.com/kromatv/react-native-webos/commit/7536340b3bb763bd5595ad9547d74b9e2b1a3ee1))
* **views:** decode images off the main thread and draw images, masks and backdrop blur with Skia ([5ff6a89](https://github.com/kromatv/react-native-webos/commit/5ff6a89e1f9a646b40ae7538181ee188bca5c6d3))
* **views:** paint CSS gradients with Skia shaders ([9a50fe2](https://github.com/kromatv/react-native-webos/commit/9a50fe24e3f9b6e4abb90407efbde2d9ea2585e2))
* **views:** scroll with Skia and send Back to the topmost Modal ([481a713](https://github.com/kromatv/react-native-webos/commit/481a713662628ce704021826687d481f7d42e777))


### Bug fixes

* **cli:** take a command's options before, between or after what it acts on ([035a640](https://github.com/kromatv/react-native-webos/commit/035a640760c75f1392b74922133ef5132b7d5f0c))
* **devtools:** keep a TV's passphrase out of the job record ([4a5c8c7](https://github.com/kromatv/react-native-webos/commit/4a5c8c7bd7899ac8bdb6dd5d648334d1442c482f))
* **init:** keep an override that already names the app's react-native, and a manifest's escapes ([2b31798](https://github.com/kromatv/react-native-webos/commit/2b3179892297a0827a7bfb09d99c585696aa8881))
* **logs:** name the frames of a native crash on a TV ([02f89d2](https://github.com/kromatv/react-native-webos/commit/02f89d24a673e8e611139c59d1618e4074020216))
* **package:** declare the window transparent for an app that depends on expo-video ([769f7d0](https://github.com/kromatv/react-native-webos/commit/769f7d00fda0c67b1d48b5085d2b3f3f481628dd))
* **perf:** report the memory peak without Samsung's app budget ([f47b4da](https://github.com/kromatv/react-native-webos/commit/f47b4da2aa37c6b0d32e14d6ac2c9fbe9c1ede7b))
* **platform:** drop the loop's pending work before the loop dies ([e9f513f](https://github.com/kromatv/react-native-webos/commit/e9f513fed8af3a996c7e8d1ebf76e9613a9841e0))
* **simulator:** take a control command and its argument as one word or two ([3c97fe9](https://github.com/kromatv/react-native-webos/commit/3c97fe93a87ec94665a5bca5e846a276fbb60997))
* **text:** measure a text with no glyphs as zero wide ([26f7622](https://github.com/kromatv/react-native-webos/commit/26f7622188bbbe036e72dd43d1ead77f76252b69))
* **video:** unload without the main loop when the closing app drops a player ([312028c](https://github.com/kromatv/react-native-webos/commit/312028cedaaf3ea7e7c5bf82ddab80e509a65f15))
* **views:** keep hitTest as the view's own name for the layer lookup ([89fc05f](https://github.com/kromatv/react-native-webos/commit/89fc05fa200cafb03510f8e91550c844decc4898))
* **views:** turn a view about its transformOrigin ([bf05d7d](https://github.com/kromatv/react-native-webos/commit/bf05d7dfaa8ebfbf14af6896233f4b358f3ca3f1))


### Performance

* **package:** ship the executable without its symbol table ([2248b8d](https://github.com/kromatv/react-native-webos/commit/2248b8d145d29e660327a10246e0c72d02210240))
* **platform:** pace frames from when each might have started ([cd505cc](https://github.com/kromatv/react-native-webos/commit/cd505cc4cbe787baecf75bf884d24df0e77df8aa))
* **render:** keep compiled shaders between runs, bound Skia's GPU cache, and free memory while hidden ([6e72898](https://github.com/kromatv/react-native-webos/commit/6e728983bac5ad2072ec0af6c6cf993fed363b97))
* **runtime:** make the TV executable 1.6 MB smaller ([33a0681](https://github.com/kromatv/react-native-webos/commit/33a06819c846a7adfb01573977a39d82530b1929))
* **svg:** keep the recorded drawing when only the Svg view's own style changes ([746b092](https://github.com/kromatv/react-native-webos/commit/746b0922bf8ebce53484d0c1303828f62b20419c))
* **svg:** play a recorded picture, and keep a still one as an image ([256b403](https://github.com/kromatv/react-native-webos/commit/256b40380a77ef8ecf610acfac078e77a6e387f8))
* **text:** lay a paragraph out once, on the layout thread, and play it in the view ([da361c7](https://github.com/kromatv/react-native-webos/commit/da361c7d2cb870421b422db926f38eee6d487461))
* **views:** cull by the bounds a subtree really paints, and test the ported views ([1db4a1c](https://github.com/kromatv/react-native-webos/commit/1db4a1c0dd4dc6fa2c58deba39559bafc682e6a4))


### Documentation

* **benchmarks:** measure the example in the simulator, and say what that does not show ([863db60](https://github.com/kromatv/react-native-webos/commit/863db6025eb5cde08f14b5e6f1cf4d2afc4d0981))
* **cli:** show perf's memory column without a budget ([43f1c1a](https://github.com/kromatv/react-native-webos/commit/43f1c1a63c72b16a4b1c808cd028f391829be8c2))
* count the C++ tests as they are ([e8c1329](https://github.com/kromatv/react-native-webos/commit/e8c132910634220f6cedadce68cd300854ff9ae9))
* describe contributing, security scope and code analysis for webOS ([4b95d25](https://github.com/kromatv/react-native-webos/commit/4b95d25e070901916abeea79ce69f9b4d641cee4))
* **notices:** list Skia, SDL and the libraries they bring, with their licence texts ([06e69ef](https://github.com/kromatv/react-native-webos/commit/06e69efa6912427d68337bbee7b4ab5d0bb925c9))
* record the evening's TV runs, a player swap that shows no picture, and what the lab asks of a seller ([#15](https://github.com/kromatv/react-native-webos/issues/15)) ([4df4169](https://github.com/kromatv/react-native-webos/commit/4df416956d413905c3cb559ff1c092395ec34a12))
* record what ran on two TVs of LG's Cloud Test Lab ([b01722a](https://github.com/kromatv/react-native-webos/commit/b01722aa2f455c802037f7d195a1af9ffa32b22d))
* **reference:** record how LG's webOS TV Simulator starts an app ([1094e2b](https://github.com/kromatv/react-native-webos/commit/1094e2b7e42c9d3c5e24e31b4306be8f56855674))
* **reference:** record how the media client reaches the bus and what the video module builds ([1cbcbae](https://github.com/kromatv/react-native-webos/commit/1cbcbaeeb37076a26c4c7678da1bceef886a3b46))
* **reference:** record LG's Cloud Test Lab, the store's native file type and other ways to reach a real TV ([9f6b118](https://github.com/kromatv/react-native-webos/commit/9f6b1182d57262907096c5eb32414310c67ed9b3))
* **reference:** record the app registration, Back at the root and the deep link parameter ([bc1c917](https://github.com/kromatv/react-native-webos/commit/bc1c91726322f91a28e601b352af7c377c26147d))
* **reference:** record what Seller Lounge's upload form does with a native package ([4edd744](https://github.com/kromatv/react-native-webos/commit/4edd7443964e4154b0a7b1c105d20aa5598e184b))
* **reference:** say how screenshots and perf samples are taken, and what the simulator showed ([18f6e12](https://github.com/kromatv/react-native-webos/commit/18f6e1269716cc6b9376ecf17cf103cd20c84077))
* **reference:** say which observations came from react-native-tizen ([430693e](https://github.com/kromatv/react-native-webos/commit/430693ea791d864e7271cb0fd5ea9737f0815e77))
* rewrite the README for webOS, with what is and is not verified ([8124f62](https://github.com/kromatv/react-native-webos/commit/8124f626b93c642554f5be11090718a09fae865c))
* say that LG's own simulator cannot run the runtime, and what it did with the package ([4529eb6](https://github.com/kromatv/react-native-webos/commit/4529eb6d94b379bd198296fb7c7d7ac1facb3401))
* say what ran on a TV, and drop the Tizen text left in the reference ([#8](https://github.com/kromatv/react-native-webos/issues/8)) ([962324c](https://github.com/kromatv/react-native-webos/commit/962324c8194725e46392d207ba86fd322eb858e2))
* **svg:** record what react-native-svg draws on Skia, and the rules behind it ([a2352d8](https://github.com/kromatv/react-native-webos/commit/a2352d880ab3a41dd78b6e49b63b1f3ac47cb3cc))
* **text-input:** describe the Skia field and SDL text input, and what SDL cannot say ([959f525](https://github.com/kromatv/react-native-webos/commit/959f5259fa9fed4988b148a8b650f528cd24eb2c))

## Changelog
