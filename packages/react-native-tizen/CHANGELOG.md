# Changelog

## [0.1.0-alpha.6](https://github.com/kromatv/react-native-tv/compare/react-native-tizen-v0.1.0-alpha.5...react-native-tizen-v0.1.0-alpha.6) (2026-11-13)


### Documentation

* point the changelog's links at this repository ([#9](https://github.com/kromatv/react-native-tv/issues/9)) ([5f61f6a](https://github.com/kromatv/react-native-tv/commit/5f61f6a4f963b3f45bee55fb5a58f86317a45c93))

## [0.1.0-alpha.5](https://github.com/kromatv/react-native-tv/compare/react-native-tizen-v0.1.0-alpha.4...react-native-tizen-v0.1.0-alpha.5) (2026-11-13)


### Features

* **expo:** a package that groups TV platforms can run this one and own the t key (kromatv/react-native-tizen#90) ([4996310](https://github.com/kromatv/react-native-tv/commit/4996310da7338fa4511319c2b99c2df97f98a991))
* **metro:** build an app whose config plugin hands Expo the Metro config (kromatv/react-native-tizen#92) ([be41682](https://github.com/kromatv/react-native-tv/commit/be41682c249127d1ff4c04448e1f6d611c29d0a9))


### Bug fixes

* **expo-video:** a video view stays black until its player has a picture (kromatv/react-native-tizen#95) ([2f60350](https://github.com/kromatv/react-native-tv/commit/2f603501fc6482579bf4b043b74dc12734d4b684))
* **rendering:** draw a color per border side on rounded corners (kromatv/react-native-tizen#94) ([3c3cbff](https://github.com/kromatv/react-native-tv/commit/3c3cbff3cdc42962d097ab2722bc87a10b1f392b))
* **views:** masks, focus rings and frame pacing while a page scrolls (kromatv/react-native-tizen#93) ([d0ec5d3](https://github.com/kromatv/react-native-tv/commit/d0ec5d3895e5b9c07477954fcdc6e42fc217a2fa))

## [0.1.0-alpha.4](https://github.com/kromatv/react-native-tv/compare/react-native-tizen-v0.1.0-alpha.3...react-native-tizen-v0.1.0-alpha.4) (2026-11-12)


### Features

* **metro:** add every TV platform an app depends on with one withTV call (kromatv/react-native-tizen#85) ([ad15571](https://github.com/kromatv/react-native-tv/commit/ad15571036498f0bbf94fd56e712d17ff1137c2d))

## [0.1.0-alpha.3](https://github.com/kromatv/react-native-tv/compare/react-native-tizen-v0.1.0-alpha.2...react-native-tizen-v0.1.0-alpha.3) (2026-11-12)


### Features

* **speech:** search by voice with expo-speech-recognition and the remote's mic button (kromatv/react-native-tizen#82) ([f665969](https://github.com/kromatv/react-native-tv/commit/f665969b0af6e0b8125c1a65128f72b59d512440))


### Bug fixes

* **metro:** bundle a second checkout of an app that shares Metro's cache (kromatv/react-native-tizen#84) ([a215286](https://github.com/kromatv/react-native-tv/commit/a2152860f711ffd0d3ed7240e2613f703d3d0bfb))

## [0.1.0-alpha.2](https://github.com/kromatv/react-native-tv/compare/react-native-tizen-v0.1.0-alpha.1...react-native-tizen-v0.1.0-alpha.2) (2026-11-04)


### Features

* **devtools:** open the app on Tizen with t in expo start (kromatv/react-native-tizen#63) ([55f2996](https://github.com/kromatv/react-native-tv/commit/55f29961cc23ff0859b4f6754d3a74c18e3beb1c))

## [0.1.0-alpha.1](https://github.com/kromatv/react-native-tv/releases/tag/react-native-tizen-v0.1.0-alpha.1) (2026-11-04)


### Features

* **cli:** make platform a floor, 6.0 by default, with run-time version checks (kromatv/react-native-tizen#41) ([3f0421a](https://github.com/kromatv/react-native-tv/commit/3f0421aa3d657a63ae6b059f0b153be34aefbeb1))
* **cli:** rn-tizen init installs one react-native-tvos, and builds refuse a second copy (kromatv/react-native-tizen#55) ([39bfb95](https://github.com/kromatv/react-native-tv/commit/39bfb95311669175f252ab9a99c6f0b6596c1998))
* **cli:** rn-tizen init sets an Expo app up, with ESM wherever users look (kromatv/react-native-tizen#43) ([bb760de](https://github.com/kromatv/react-native-tv/commit/bb760de3708fd9735c0c70bbe6eb5ec03276f3b0))
* **devsupport:** Fast Refresh, reload, logs and LogBox from expo start (kromatv/react-native-tizen#31) ([5e0ef7b](https://github.com/kromatv/react-native-tv/commit/5e0ef7b9a50bd55bdbf11e13911658281aa8adc1))
* **devsupport:** open React Native DevTools on the TV with j (kromatv/react-native-tizen#34) ([aa6bea0](https://github.com/kromatv/react-native-tv/commit/aa6bea00d85ded1bad6411250322f0b656ec22ec))
* **expo:** a config plugin, and Tizen commands in expo start (kromatv/react-native-tizen#30) ([aaefa41](https://github.com/kromatv/react-native-tv/commit/aaefa41cc9640e277a8e23a49267633d84c0e1d4))
* **perf:** show the app's memory against Samsung's 380 MB budget (kromatv/react-native-tizen#54) ([e2cff83](https://github.com/kromatv/react-native-tv/commit/e2cff83dcae212bb2a99f4369930ac5c207e7f5f))
* **sdb:** reach the TV over sdb's protocol instead of Tizen Studio's sdb (kromatv/react-native-tizen#5) ([cb505da](https://github.com/kromatv/react-native-tv/commit/cb505dac70a7064ea3ccf9e61f66ea9aafd3f455))
* **signing:** sign packages in TypeScript instead of with the tizen CLI (kromatv/react-native-tizen#10) ([44f9319](https://github.com/kromatv/react-native-tv/commit/44f93197809fdf944ef873d7773a1b31626cdbfe))
* **tizen:** find TVs, manage certificates and deploy any package (kromatv/react-native-tizen#18) ([3f8f231](https://github.com/kromatv/react-native-tv/commit/3f8f2318a7b66d76f1aa0fb4be5ac0a58fe48ce7))
* **toolchain:** build the runtime against any floor's rootstrap from 6.0 (kromatv/react-native-tizen#40) ([911d8aa](https://github.com/kromatv/react-native-tv/commit/911d8aa08f55bce73c5f2f70618cd3154a35a0e1))


### Bug fixes

* clear the SonarCloud issues in src, example and the overrides (kromatv/react-native-tizen#6) ([a68a7d6](https://github.com/kromatv/react-native-tv/commit/a68a7d6b2b0a3c9e5cc407c1a2f40d847980f94a))
* **cli:** name the rootstrap package and the package manager's command in init, doctor and the docs (kromatv/react-native-tizen#48) ([19c0e52](https://github.com/kromatv/react-native-tv/commit/19c0e52b6abb0030633dd5645c514a21bd6e1e2e))
* **cli:** read one app by package id in logs, perf and screenshot (kromatv/react-native-tizen#53) ([4bf8a23](https://github.com/kromatv/react-native-tv/commit/4bf8a236035b765182ce6e6e2f5aabe243989137))
* **expo:** keep the fallback app directory private, clear the core's Sonar issues (kromatv/react-native-tizen#12) ([ca077a9](https://github.com/kromatv/react-native-tv/commit/ca077a97c3f2d0c990f827052793905317b12ed7))
* guard the app's own files, explain every failure, and press keys under Node (kromatv/react-native-tizen#17) ([3d50286](https://github.com/kromatv/react-native-tv/commit/3d502865992c869fd7c8fdfcb83d8bbdbbf8a7ee))
* **host:** tear down a ReactHost whose start throws (kromatv/react-native-tizen#23) ([b1dfef0](https://github.com/kromatv/react-native-tv/commit/b1dfef086ce6e8a85b9fb6351e29b60ea23fa63f))
* **input,tv,text:** decode UTF-8 per WHATWG, finish held presses, reject bad voice indices, clear the Sonar issues (kromatv/react-native-tizen#24) ([e8f8b11](https://github.com/kromatv/react-native-tv/commit/e8f8b11c31505b2aaf4576eb4fd2317544adc1d9))
* **lists:** give react-native-tvos's list wrapper the list's flex (kromatv/react-native-tizen#57) ([e82b486](https://github.com/kromatv/react-native-tv/commit/e82b486764b63b2fd4184974435c251609339ef1))
* **modules:** draw blob ids from the OS random source, clear the Sonar issues (kromatv/react-native-tizen#15) ([f0c55c3](https://github.com/kromatv/react-native-tv/commit/f0c55c389dcb9226ffa4fcf7d17f92055cfbcdab))
* **networking:** send form data and file bodies (kromatv/react-native-tizen#26) ([90c4478](https://github.com/kromatv/react-native-tv/commit/90c447879fd888e3cf61c3352ae7723ff32ead28))
* **network:** refuse header injection, send GET without a body, require TLS 1.2 (kromatv/react-native-tizen#19) ([0c2b50a](https://github.com/kromatv/react-native-tv/commit/0c2b50aef6f5c8840908bf84c951f9173aba7372))
* **package,tizen:** ship the runtime flavour built, and stop sdb refusals after Install (kromatv/react-native-tizen#47) ([9a0c736](https://github.com/kromatv/react-native-tv/commit/9a0c736d8ef71abcc94ab2aa1e76fc28b0a7bac9)), closes kromatv/react-native-tizen#44
* **signing:** refuse every wrong PKCSkromatv/react-native-tizen#12 password as one (kromatv/react-native-tizen#36) ([c67112f](https://github.com/kromatv/react-native-tv/commit/c67112f79aeb208b11cc44cf3f46a3db55002f6b))
* **simulator:** clear the SonarCloud issues in the shims, launcher and guest scripts (kromatv/react-native-tizen#8) ([a0cfa1d](https://github.com/kromatv/react-native-tv/commit/a0cfa1d6ec2d80b1cf68c2bf7675a304dd2f3894))
* **threading:** fire timers by deadline, and never hang or end the app on a throwing task (kromatv/react-native-tizen#11) ([1225e57](https://github.com/kromatv/react-native-tv/commit/1225e570b2f739e07aa4327b3c31a08d7fbf2cce))
* **threading:** queue a main-loop task as an Ecore job when the main loop posts it (kromatv/react-native-tizen#33) ([c2e5459](https://github.com/kromatv/react-native-tv/commit/c2e5459dae640ac77b71fc12ce0394ad9b486e84))
* **views:** leave a ring's clear middle undrawn, so the TV does not shade the box (kromatv/react-native-tizen#35) ([6817a63](https://github.com/kromatv/react-native-tv/commit/6817a631842725c163038a39c3b4a560e10aff93))


### Documentation

* a README for the first release, and benchmarks measured on a TV (kromatv/react-native-tizen#42) ([ed9c7c5](https://github.com/kromatv/react-native-tv/commit/ed9c7c52cd10b8c671b80692c4ee39a3be033d42))
* add contributing guide, security policy and templates ([4237635](https://github.com/kromatv/react-native-tv/commit/4237635373e1360edf21267df48dbfedfe14d4c5))
* drop the FlatList flex gap, fixed by kromatv/react-native-tizen#57 (kromatv/react-native-tizen#58) ([a551eee](https://github.com/kromatv/react-native-tv/commit/a551eee047419ce1c2065e64dbb1b275d5d1b891))
* **reference:** record what the 2024 Frame's compositor offers an app on a 4K panel (kromatv/react-native-tizen#61) ([3cea367](https://github.com/kromatv/react-native-tv/commit/3cea367e122cddb87f81e4eac0ad50e182ff7aa1))
* **reference:** Tizen 4.0 to 10.0 per version, and what the runtime lacks below 6.0 (kromatv/react-native-tizen#39) ([932ad92](https://github.com/kromatv/react-native-tv/commit/932ad9216aeb439f48fef7c68123cad66800fec1))
