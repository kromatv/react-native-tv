# Changelog

## [0.1.0-alpha.1](https://github.com/kromatv/react-native-tizen/compare/v0.1.0-alpha.0...v0.1.0-alpha.1) (2026-11-04)


### Features

* **cli:** make platform a floor, 6.0 by default, with run-time version checks ([#41](https://github.com/kromatv/react-native-tizen/issues/41)) ([bba6e7e](https://github.com/kromatv/react-native-tizen/commit/bba6e7e5a095938f8925a9e6ee0349b86b2f1520))
* **cli:** rn-tizen init installs one react-native-tvos, and builds refuse a second copy ([#55](https://github.com/kromatv/react-native-tizen/issues/55)) ([b567857](https://github.com/kromatv/react-native-tizen/commit/b567857a5b4e88b72dfc29de3443417a2646347c))
* **cli:** rn-tizen init sets an Expo app up, with ESM wherever users look ([#43](https://github.com/kromatv/react-native-tizen/issues/43)) ([ba11fa8](https://github.com/kromatv/react-native-tizen/commit/ba11fa8b92ed094e5f164b81909a514b1bd71a47))
* **devsupport:** Fast Refresh, reload, logs and LogBox from expo start ([#31](https://github.com/kromatv/react-native-tizen/issues/31)) ([d88a8c7](https://github.com/kromatv/react-native-tizen/commit/d88a8c7eceb1cf14f075b2667051ea531ad2761d))
* **devsupport:** open React Native DevTools on the TV with j ([#34](https://github.com/kromatv/react-native-tizen/issues/34)) ([c65bd54](https://github.com/kromatv/react-native-tizen/commit/c65bd546cfddcb9a87123c2b2d0c40037f8e22c8))
* **expo:** a config plugin, and Tizen commands in expo start ([#30](https://github.com/kromatv/react-native-tizen/issues/30)) ([1858e62](https://github.com/kromatv/react-native-tizen/commit/1858e62ae26af5de436377df244e7eea4169700a))
* **perf:** show the app's memory against Samsung's 380 MB budget ([#54](https://github.com/kromatv/react-native-tizen/issues/54)) ([601a692](https://github.com/kromatv/react-native-tizen/commit/601a692b4fd4b8685ee75b093fdac4632fd5a968))
* **sdb:** reach the TV over sdb's protocol instead of Tizen Studio's sdb ([#5](https://github.com/kromatv/react-native-tizen/issues/5)) ([89464bd](https://github.com/kromatv/react-native-tizen/commit/89464bd0cd81197cc01edfd1c62e88badf15866c))
* **signing:** sign packages in TypeScript instead of with the tizen CLI ([#10](https://github.com/kromatv/react-native-tizen/issues/10)) ([b24332a](https://github.com/kromatv/react-native-tizen/commit/b24332a195241b5c8ca1ec4c142dca005e3d7ef0))
* **tizen:** find TVs, manage certificates and deploy any package ([#18](https://github.com/kromatv/react-native-tizen/issues/18)) ([1b9d212](https://github.com/kromatv/react-native-tizen/commit/1b9d212b5d0bb3090db92cdaaf3bef6c93edf03e))
* **toolchain:** build the runtime against any floor's rootstrap from 6.0 ([#40](https://github.com/kromatv/react-native-tizen/issues/40)) ([ad7d70f](https://github.com/kromatv/react-native-tizen/commit/ad7d70fa81e427f65c3244e0db501f7f81e94fed))


### Bug fixes

* clear the SonarCloud issues in src, example and the overrides ([#6](https://github.com/kromatv/react-native-tizen/issues/6)) ([ec21e6d](https://github.com/kromatv/react-native-tizen/commit/ec21e6da338deab8940c840f011d6eafea987280))
* **cli:** name the rootstrap package and the package manager's command in init, doctor and the docs ([#48](https://github.com/kromatv/react-native-tizen/issues/48)) ([10514f9](https://github.com/kromatv/react-native-tizen/commit/10514f9b4e5640610f9a47399cacdb206b7e19f9))
* **cli:** read one app by package id in logs, perf and screenshot ([#53](https://github.com/kromatv/react-native-tizen/issues/53)) ([60a62d7](https://github.com/kromatv/react-native-tizen/commit/60a62d77e59eaf7a3e3c4332913c857f70655826))
* **expo:** keep the fallback app directory private, clear the core's Sonar issues ([#12](https://github.com/kromatv/react-native-tizen/issues/12)) ([82e8ce6](https://github.com/kromatv/react-native-tizen/commit/82e8ce68f5645c4aa03356f7b53bfef3cf6155eb))
* guard the app's own files, explain every failure, and press keys under Node ([#17](https://github.com/kromatv/react-native-tizen/issues/17)) ([cf42c3e](https://github.com/kromatv/react-native-tizen/commit/cf42c3eba83bae1abb854652603221f7c018de07))
* **host:** tear down a ReactHost whose start throws ([#23](https://github.com/kromatv/react-native-tizen/issues/23)) ([b4b6da0](https://github.com/kromatv/react-native-tizen/commit/b4b6da0b39d66ccfe3c8e668a910e16d38804d95))
* **input,tv,text:** decode UTF-8 per WHATWG, finish held presses, reject bad voice indices, clear the Sonar issues ([#24](https://github.com/kromatv/react-native-tizen/issues/24)) ([86f025e](https://github.com/kromatv/react-native-tizen/commit/86f025ee3f3fa11d47bd5355ab8811b4fbdb9a13))
* **lists:** give react-native-tvos's list wrapper the list's flex ([#57](https://github.com/kromatv/react-native-tizen/issues/57)) ([54d82f9](https://github.com/kromatv/react-native-tizen/commit/54d82f9648f6a388bac892cb2bd240bcbfbf15ef))
* **modules:** draw blob ids from the OS random source, clear the Sonar issues ([#15](https://github.com/kromatv/react-native-tizen/issues/15)) ([c401aeb](https://github.com/kromatv/react-native-tizen/commit/c401aebf124f497796fb50897f1f68ed16a91ba9))
* **networking:** send form data and file bodies ([#26](https://github.com/kromatv/react-native-tizen/issues/26)) ([1d5ce54](https://github.com/kromatv/react-native-tizen/commit/1d5ce547dddbaaf68356307255c4e337cbe4045c))
* **network:** refuse header injection, send GET without a body, require TLS 1.2 ([#19](https://github.com/kromatv/react-native-tizen/issues/19)) ([e718644](https://github.com/kromatv/react-native-tizen/commit/e718644fe22f78c72190a3243497cc04a3a5b8ef))
* **package,tizen:** ship the runtime flavour built, and stop sdb refusals after Install ([#47](https://github.com/kromatv/react-native-tizen/issues/47)) ([080fe53](https://github.com/kromatv/react-native-tizen/commit/080fe534df288aa99b7d04e35f977b841283204c)), closes [#44](https://github.com/kromatv/react-native-tizen/issues/44)
* **signing:** refuse every wrong PKCS[#12](https://github.com/kromatv/react-native-tizen/issues/12) password as one ([#36](https://github.com/kromatv/react-native-tizen/issues/36)) ([a740246](https://github.com/kromatv/react-native-tizen/commit/a740246313c29f1475abe1d59a80c7e28afeaa25))
* **simulator:** clear the SonarCloud issues in the shims, launcher and guest scripts ([#8](https://github.com/kromatv/react-native-tizen/issues/8)) ([891242d](https://github.com/kromatv/react-native-tizen/commit/891242d8f72166363786f7d34f7a96a12eadc555))
* **threading:** fire timers by deadline, and never hang or end the app on a throwing task ([#11](https://github.com/kromatv/react-native-tizen/issues/11)) ([91fac9b](https://github.com/kromatv/react-native-tizen/commit/91fac9b8124ebe85144dedf507f045fde1a710e7))
* **threading:** queue a main-loop task as an Ecore job when the main loop posts it ([#33](https://github.com/kromatv/react-native-tizen/issues/33)) ([f6a4af1](https://github.com/kromatv/react-native-tizen/commit/f6a4af1431eb13285afcf52fa21ac77f84d4d7ed))
* **views:** leave a ring's clear middle undrawn, so the TV does not shade the box ([#35](https://github.com/kromatv/react-native-tizen/issues/35)) ([894918b](https://github.com/kromatv/react-native-tizen/commit/894918b0db1c0301081cd1c12588e525c8c2154f))


### Documentation

* a README for the first release, and benchmarks measured on a TV ([#42](https://github.com/kromatv/react-native-tizen/issues/42)) ([689206f](https://github.com/kromatv/react-native-tizen/commit/689206f0313d2e46354cdbe88c54691204b378a7))
* add contributing guide, security policy and templates ([32f9992](https://github.com/kromatv/react-native-tizen/commit/32f9992420f58938b9cdc535ab8bd6b98ae67c13))
* drop the FlatList flex gap, fixed by [#57](https://github.com/kromatv/react-native-tizen/issues/57) ([#58](https://github.com/kromatv/react-native-tizen/issues/58)) ([e329b0f](https://github.com/kromatv/react-native-tizen/commit/e329b0f096cf4b02c9263088b494f872495d4a3f))
* **reference:** record what the 2024 Frame's compositor offers an app on a 4K panel ([#61](https://github.com/kromatv/react-native-tizen/issues/61)) ([8bc8ae5](https://github.com/kromatv/react-native-tizen/commit/8bc8ae55ea0311b748c40e66a7b844cfa0bf1cd3))
* **reference:** Tizen 4.0 to 10.0 per version, and what the runtime lacks below 6.0 ([#39](https://github.com/kromatv/react-native-tizen/issues/39)) ([b769bdf](https://github.com/kromatv/react-native-tizen/commit/b769bdf44c80fec257441143284f1e0d58c283ab))
