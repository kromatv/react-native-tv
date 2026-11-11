# webOS TV: the developer channel

What `rn-webos` needs to reach an LG TV in Developer Mode without LG's `ares-*`
commands: the key server, the SSH endpoint, the jail behind it, and the
`luna-send-pub` lines that install, launch, close and remove an app. Checked on
2026-11-11.

**Nothing in this file was checked on a TV by this project.** It is what LG
documents, what LG's and webosbrew's tools do in their source, and what other
people report from their own TVs.

| Tag | Meaning |
|---|---|
| **D** | Documented: a page or a file somebody publishes. The source column says who. A community page, or another project's report from its own TV, is still only documentation here, and the row names the TV when the author did |
| **S** | Read from source: file and line |
| **I** | Inferred, and the row says from what |

How sources are written:

- `cli:` is a path inside the npm package `@webos-tools/cli` 3.2.5, LG's
  `ares-*` commands (`npm pack @webos-tools/cli@3.2.5` gives the same files).
- `ssh2:` is a path inside the npm package `ssh2` 1.17.0, the version
  `cli:npm-shrinkwrap.json` pins (`cli:package.json` asks for `^1.17.0`).
- `jail:` is LG's jail configuration as served for `sdkVersion=10.0.0`
  ([lge-jail]), `jail11:` the one for `11.0.0` ([lge-jail-11]). The file
  carries a confidentiality header, so only single facts are quoted from it.
- Everything else is a link in [Sources](#sources), followed by line numbers.

## 1. Developer Mode on the TV

| Fact | Tag | Source |
|---|---|---|
| The Developer Mode app is installed from LG's store on the TV and "requires you to sign in to the LG Developer site". It exists "From webOS TV 3.0". | D | [lg-devmode], [lg-cli] |
| The TV and the computer must be on the same network. | D | [lg-devmode] |
| Turning on **Dev Mode Status** reboots the TV. | D | [lg-devmode] |
| The session is limited. The app shows what is left as **Remain Session**; **EXTEND** needs the TV on a network; "If the session time runs out, you cannot extend the session time." | D | [lg-devmode] |
| LG's page gives no duration. webosbrew: "Developer Mode has a time limit of 1000 hours." | D | [wb-devmode] |
| Developer Mode is disabled "After the TV reboot ten times while the TV is not connected to a network" and "After the TV reboot when the remaining session time has run out". Then "the installed apps that you were using on Developer Mode are uninstalled". | D | [lg-devmode] |
| One TV per developer account: logging in on another TV logs the first one out and disables its Developer Mode. | D | [wb-devmode] |
| The session token is the content of `/var/luna/preferences/devmode_enabled` on the TV, letters and digits only (`^[0-9a-zA-Z]+$`). The `prisoner` login reads it with `cat`. | S | [dmd-devmode] 82, 98; [dmd-renew] 60-65; [wb-reset] 10 |
| `GET https://developer.lge.com/secure/ResetDevModeSession.dev?sessionToken=<token>` resets the timer. The answer is one JSON object whose `result` is `"success"`. dev-manager-desktop's renewal script and webosbrew's `devmode-reset.sh` both call it. | S | [dmd-renew] 109-110; [wb-reset] 22 |
| `GET https://developer.lge.com/secure/CheckDevModeSession.dev?sessionToken=<token>` answers `result`, `errorCode`, `errorMsg`. When `result` is `"success"`, dev-manager-desktop shows `errorMsg` as the time left. | S | [dmd-devmode] 47-61 |
| For an empty token the same endpoint answered `{"result":"fail","errorCode":"ERR_006","errorMsg":"Check sessionToken"}`. At boot `/etc/init/devmode.conf` runs the app's `start-devmode.sh`, which calls `CheckDevModeSession.dev`; on `fail` it deletes `devmode_enabled`, and `devmode.conf` then runs `rm -rf /media/developer`. | D | [lglow] 13-32 (a rooted TV, model not given) |
| **Key Server** in the app starts the HTTP server of section 2. It "only needs to be enabled when you add the TV" and is off again after every restart. | D | [wb-devmode] |
| The passphrase "consists of 6 characters", is "case sensitive" and is shown at the bottom left of the app. | D | [lg-devmode] |

## 2. The key server and the private key

| Fact | Tag | Source |
|---|---|---|
| The key is at `http://<tv>:9991/webos_rsa`. ares sends `HEAD`, then `GET`; any status but 200 is its "Failed to get ssh private key". | S | cli:lib/base/novacom.js:370-408 |
| ares writes the body unchanged to `~/.ssh/<device name, whitespace as _>_webos`, mode 0600. | S | cli:lib/base/novacom.js:31, 371-373, 413 |
| It then prompts `input passphrase:` (or takes `--passphrase`) and sets on the device entry: `privateKey: {"openSsh": "<file name>"}`, `passphrase`, `files: "sftp"`, `port: "9922"`, `username: "prisoner"`. The passphrase stays in clear text in the device list (section 9). | S | cli:bin/ares-novacom.js:109-152 |
| ares-cli-rs speaks HTTP/1.1 on a plain socket: `GET /webos_rsa`, headers `Host`, `Accept: */*`, `Connection: close`. It follows redirects on the same host, reads chunked or length-framed bodies and refuses more than 65,536 bytes. | S | [acr-setup] 12-59, 198-202 |
| The file a webOS 23 TV served starts with `-----BEGIN RSA PRIVATE KEY-----`, `Proc-Type: 4,ENCRYPTED`, `DEK-Info: AES-128-CBC,<32 hex digits>`, an empty line, then base64 in 64-character lines. `GET /` on the same port gave 404. | D | [dmd-163-key] (a user's paste, TV model not given) |
| That header is OpenSSL's traditional PEM encryption of a PKCS#1 `RSAPrivateKey`: the IV is the 16 bytes of `DEK-Info`; the AES-128 key is `MD5(passphrase \|\| IV[0..8])`; longer keys append `MD5(previous \|\| passphrase \|\| IV[0..8])`. ssh2 decrypts it this way. | S | ssh2:lib/protocol/keyParser.js:810-878 |
| Node needs no library for it: `crypto.createPrivateKey({ key: pem, passphrase })` returns the key. A hand-made `MD5` plus `aes-128-cbc` gave the same DER. A wrong passphrase threw `ERR_OSSL_BAD_DECRYPT`. | I | From a key with the same three header lines, made with `openssl genrsa -aes128` (OpenSSL 1.1.1t) and read with Node 26.5.0 (OpenSSL 3.6.3). Not a TV's key |
| With a wrong passphrase ares fails only when it connects: "Cannot parse privateKey: Malformed OpenSSH private key. Bad passphrase?" ssh2 turns the padding check off, so the error comes from the ASN.1 parser. | D, S | [dmd-163-key]; ssh2:lib/protocol/keyParser.js:870, 887-889 |
| A key and its passphrase can be checked with `ssh-keygen -y -P "<passphrase>" -f <file>`. | D | [wb-sdk] |
| LG staff: "Key Server must be ON before getting the key file." Users report port 9991 closed with Key Server on, fixed for some by unchecking and accepting the TV's user agreements again, for others by keeping the Developer Mode app open. | D | [forum-getkey], [dmd-163] |

## 3. The SSH endpoint

| Fact | Tag | Source |
|---|---|---|
| Port 9922, user `prisoner`, no password: "When using the Developer Mode app, a password is not required." | D, S | [lg-devmode]; cli:files/conf-base/profile/config-tv.json:14-22 |
| A TV answered `remote software version OpenSSH_6.1`. Negotiated: key exchange `ecdh-sha2-nistp256`, host key `ssh-rsa`, cipher `aes128-ctr`, MAC `umac-64@openssh.com`, no compression. Public key authentication succeeded. | D | [forum-ssh35] (a user's `ssh -vv` from OpenSSH 8.6p1; webOS TV 3.5, HE_DTV_W17P_AFADABAA; 2023) |
| A newer OpenSSH client stops with "no matching host key type found. Their offer: ssh-rsa": that TV offered a single host key algorithm. | D | [forum-ssh34] (webOS 3.4.0, 2023) |
| LG's jail configuration names three "openssh sshd host keys", `/var/lib/devmode/sshd/ssh_host_rsa_key`, `ssh_host_ecdsa_key` and `ssh_host_dsa_key`, and takes OpenSSH from the Developer Mode app's own directory, `/media/cryptofs/apps/usr/palm/services/com.palmdts.devmode.service/binaries-armv71/opt/openssh`, linked to `/opt/openssh`. The same lines are in all ten files fetched, `sdkVersion` 1.0.0 to 12.0.0. | D | jail:160-171, 399-418 |
| So the server is the OpenSSH that ships inside the Developer Mode app, not one from the firmware. Its version on anything newer than webOS 3.5 is not documented. | I | From the three rows above |
| OpenSSH 6.1's built-in lists (the TV's `sshd_config` is not public). Key exchange: `ecdh-sha2-nistp256`, `-nistp384`, `-nistp521`, `diffie-hellman-group-exchange-sha256`, `diffie-hellman-group-exchange-sha1`, `diffie-hellman-group14-sha1`, `diffie-hellman-group1-sha1`. Host keys: ECDSA, `ssh-rsa`, `ssh-dss`. Ciphers: `aes128-ctr`, `aes192-ctr`, `aes256-ctr`, the `arcfour` family, `aes*-cbc`, `3des-cbc`, `blowfish-cbc`, `cast128-cbc`. MACs: `hmac-md5`, `hmac-sha1`, `umac-64@openssh.com`, `hmac-sha2-256`, `hmac-sha2-512`, `hmac-ripemd160`, `hmac-sha1-96`, `hmac-md5-96`. | S | [openssh-61] 56-93 |
| Later than 6.1, so absent: AES-GCM and the `-etm` MACs (6.2), `curve25519-sha256` and Ed25519 (6.5), `rsa-sha2-256` and `rsa-sha2-512` (7.2). | D | [openssh-62], [openssh-65], [openssh-88] |
| OpenSSH 8.8 and later refuse SHA-1 RSA signatures unless told `HostkeyAlgorithms +ssh-rsa` and `PubkeyAcceptedAlgorithms +ssh-rsa`. dev-manager-desktop's renewal script passes `-o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedKeyTypes=+ssh-rsa -o StrictHostKeyChecking=no` to every TV. | D, S | [openssh-88]; [dmd-renew] 60-65 |
| ares hands ssh2 the whole device entry (`host`, `port`, `username`, `privateKey` as the key file's bytes, `passphrase`, `password`) plus `readyTimeout: 30000` and `algorithms.kex = ["diffie-hellman-group1-sha1", "ecdh-sha2-nistp256", "ecdh-sha2-nistp384", "ecdh-sha2-nistp521", "diffie-hellman-group-exchange-sha256", "diffie-hellman-group14-sha1"]`. Nothing else is set: no host key check, no keepalive. | S | cli:lib/base/novacom.js:192-204, 617-641 |
| ssh2's defaults already cover the rest of what OpenSSH 6.1 speaks: host key `ssh-rsa`; ciphers `aes128-ctr`, `aes192-ctr`, `aes256-ctr`; MACs `hmac-sha2-256`, `hmac-sha2-512`, `hmac-sha1`. Its default key exchange list also holds the three `ecdh-sha2-nistp*` and `diffie-hellman-group-exchange-sha256`, so ares' override only adds the two SHA-1 groups. | S | ssh2:lib/protocol/constants.js:35-61, 64-71, 83-106, 129-136 |
| ssh2 signs the login with `ssh-rsa` (SHA-1) when the server sends no `server-sig-algs`. A 6.1 server predates the RSA SHA-2 signatures that list would name. | S, I | ssh2:lib/client.js:2155-2173; from [openssh-88] |
| ares-cli-rs (libssh) sets: key exchange `curve25519-sha256`, `curve25519-sha256@libssh.org`, the three `ecdh-sha2-nistp*`, `diffie-hellman-group18-sha512`, `-group16-sha512`, `-group-exchange-sha256`, `-group14-sha256`, `-group1-sha1`, `-group14-sha1`; MACs `hmac-sha2-256-etm@openssh.com`, `hmac-sha2-512-etm@openssh.com`, `hmac-sha2-256`, `hmac-sha2-512`, `hmac-sha1-96`, `hmac-sha1`, `hmac-md5`; host and public key types `ssh-ed25519`, `ecdsa-sha2-nistp521`, `-nistp384`, `-nistp256`, `rsa-sha2-512`, `rsa-sha2-256`, `ssh-rsa`. Timeout 10 s. Known hosts point at the null device: "a device has no stable host key". | S | [acr-session] 125-172 |
| It also turns on TCP keepalive (30 s idle, 10 s interval, 3 probes), because a TV that drops off Wi-Fi sends no FIN and a waiting install would hang. | S | [acr-session] 55-72 |
| ssh2 takes each algorithm list as an array, or as `{append, prepend, remove}` applied to its defaults. `diffie-hellman-group14-sha1` and `diffie-hellman-group1-sha1` are in its supported key exchanges and not in its default ones. | S | ssh2:lib/utils.js:210-300; ssh2:lib/protocol/constants.js:35-61 |
| ssh2 runs over a socket the caller hands it as `sock`, and starts the handshake at once when that socket is already connected; `readyTimeout` still bounds the handshake. So TCP keepalive is the caller's to set, on its own `net` socket. | S | ssh2:lib/client.js:290, 1111-1120, 1160-1173 |
| Before SSH, ares opens a bare TCP connection to the port with a 2 s timeout and reports a timeout or refusal from that. | S | cli:lib/base/novacom.js:567-594 |

## 4. What the `prisoner` session can do

| Fact | Tag | Source |
|---|---|---|
| Every tool works through `exec` requests. ares runs each command as `ssh.exec(cmd)` with no PTY, reads stdout and stderr, and takes the exit status as the result. | S | cli:lib/base/novacom.js:932-1020 |
| A shell request is accepted, a PTY request is not: "PTY allocation request failed on channel 0". webosbrew: the "prisoner user does not have access to PTS, and thus `bash` will behave strangely". | D | [forum-ssh35]; [wb-sdk] |
| LG's CLI gives the `tv` profile no `ares-shell`, `ares-push`, `ares-pull` or `ares-log`. It has `ares-novacom --run <command>`: one exec wired to the terminal's stdin, stdout and stderr. | S | cli:files/conf/ares.json:20-47; cli:bin/ares-novacom.js:154-167 |
| LG staff: "We do not provide access the webOS TV system. Please use the Developer Mode app for testing your apps." | D | [forum-ssh35] |
| The session runs in a jail. LG serves the jail's configuration per `sdkVersion`, and one file configures both the Developer Mode app's sshd, whose app id is `com.palm.devmode.openssh`, and native apps installed in Developer Mode. The jails live under `/var/palm/jail`. | D | jail:12-16; jail11:30-41 |
| In it, read-only: `/bin`, `/usr/bin`, `/lib`, `/usr/lib`, `/usr/share`, `/etc/ssl`, `/var/luna/preferences`, `/media/internal`. Read-write: `/media/developer`, `/tmp` (mode 1777), `/proc`, `/var/run`, `/var/run/luna-service2` (the comment says "ls2 (public bus only)"), `/dev/shm`. `HOME` ends as `/media/developer`. Groups: `video,audio,luna,compositor,crashd,se`. | D | jail:23-50, 116-122, 153-156, 181, 206, 420-422, 681-682 |
| The published file is what a TV holds as `/media/developer/jail_app.conf`: the MD5 a third party took of that file on a webOS 4.5 TV, `410cb71a593449bcaccf000134592c87`, is the MD5 of the file LG serves for `sdkVersion=4.5.0`. | D, I | [plx] 429-436; the MD5 of the download, taken here |
| From the 11.0.0 file on, the SSH session loses the shared `/tmp`: for `com.palm.devmode.openssh`, `/tmp` is an empty root-owned directory of mode 0111, with `/tmp/var/log` mounted read-only and linked to `/var/log`, and no USB directories. Native apps keep the shared, writable `/tmp`. | D | jail11:30-41, 134, 376-378 |
| Programs ares runs as `prisoner`, by these paths: `/usr/bin/test`, `/bin/mkdir`, `/bin/ls`, `/usr/bin/tail`, `/usr/bin/md5sum`, `/bin/rm`, `/bin/cat`, `/usr/bin/luna-send-pub`. Its resource monitor also runs `date`, `grep`, `sed`, `awk`, `ps`, `free` and reads `/proc`. | S | cli:lib/install.js:86-141, 247; cli:lib/base/novacom.js:761, 849; cli:lib/device.js:391, 733 |
| ares-cli-rs runs `cat`, `mkdir -p`, `rm -rf` and `sha256sum` without a path, and treats a failing `sha256sum` as "this device has none". dev-manager-desktop runs `id` on every new connection. | S | [acr-transfer] 101-103, 262, 299, 323, 339-348; [dmd-conn] 106-121 |

## 5. Pushing a file

| Fact | Tag | Source |
|---|---|---|
| ares tries SFTP first, whatever the device entry says: it opens the `sftp` subsystem and pipes the file into `sftp.createWriteStream(<remote path>)`. | S | cli:lib/base/novacom.js:727-751, 771-808 |
| If the subsystem answers `sftp-server ... not found` (ares makes that code 4), or the request ends with code 127, ares falls back to one exec, `/bin/cat > "<remote path>"`, with the file as the channel's stdin. Code 14 is reported as no free space. | S | cli:lib/base/novacom.js:700-719, 736-745, 759-763 |
| The device entry's `files` field (`"stream"` for a new TV, `"sftp"` after `--getkey`) is read into `options.op` and never used. | S | cli:lib/install.js:90; cli:files/conf-base/profile/config-tv.json:19; cli:bin/ares-novacom.js:138 |
| ares-cli-rs uses SFTP unless the entry says `"files": "stream"` or the SFTP session does not start. Over SFTP it opens the file write-only, create, truncate, mode 0644. Otherwise it execs `cat > <quoted path>`, writes the file, sends EOF and reads the exit status. | S | [acr-transfer] 122-133, 244-274 |
| Neither tool uses `scp`. | S | the two files above |
| ssh2 1.17 reports an SFTP session that does not start through the callback of `sftp()`: `Unable to start subsystem: sftp` when the server refuses the request, `Received exit code <n> while establishing SFTP session` or `Received unexpected SFTP session termination` when the channel exits or closes before the session is ready. | S | ssh2:lib/client.js:1555-1630, 1903 |
| The package goes to `/media/developer/temp/<local file name>` (ares) or `/media/developer/temp/ares_install_<first 10 hex digits of its SHA-256>.ipk` (ares-cli-rs). ares first runs `/usr/bin/test -d /media/developer/temp \|\| /bin/mkdir -p /media/developer/temp`. | S | cli:lib/install.js:52-56, 68, 86; [acr-install] 107, 122 |
| After the push ares runs `/bin/ls -l "<path>"`, then compares the MD5 of the last 200 bytes with `/usr/bin/tail -c 200 "<path>" \| /usr/bin/md5sum`; a mismatch stops the install. ares-cli-rs compares `sha256sum <path>` of the whole file. | S | cli:lib/install.js:99-178; [acr-install] 245-256 |
| ares reads the TV's `md5sum` answer as the text before the first `-`, so the digest starts the line. When either digest is missing it only warns, "Cannot verify transmitted file", and installs all the same. | S | cli:lib/install.js:141-175 |
| When the install has answered, ares runs `/bin/rm -f "<path>"`. | S | cli:lib/install.js:246-248 |
| The installer wants a path that exists and whose first `.ipk` is its last four characters. | S | [ai2-service] 311-329 (webOS OSE source, not the TV's build) |

## 6. luna-send and the calls

### 6.1 How `luna-send-pub` is driven

| Fact | Tag | Source |
|---|---|---|
| `luna-send-pub` is `luna-send` compiled with `PUBLIC_HUB_ONLY`. | S | [ls2-cmake] 24-26 (webOS OSE) |
| `-n <x>` exits after x replies. `-i` stays until stdin hangs up: it watches fd 0 for `G_IO_ERR \| G_IO_HUP`. With neither, it prints "you must always call luna-send with \"-i\" or \"-n\"" and fails. `-f` pretty-prints, `-a` sets an app id, `-m` a service name. Each reply is printed as one line and flushed. | S | [ls2-send] 206-213, 295-318, 482-495 (webOS OSE) |
| ares builds every call as `<lunaSend> <mode>luna://<service>/<folder>/<method> '<JSON.stringify(param)>'`, with `lunaSend` = `/usr/bin/luna-send-pub` and `<mode>` = `-n 1 ` or `-i `. | S | cli:lib/base/luna.js:31-53; cli:files/conf/command-service.json:2 |
| The JSON sits between single quotes and is not escaped, so a `'` in a parameter breaks the command. A call without a folder gets a double slash, as in `luna://com.webos.applicationManager//launch`. | S | cli:lib/base/luna.js:38, 53 |
| ares gives the exec an stdin that never sends anything and ends it to stop an `-i` call: "the only way to terminate an interactive request". ares-cli-rs sends EOF, then the signal `TERM`. | S | cli:lib/base/luna.js:33-37; cli:lib/util/eof.js; [acr-sub] 66-73 |
| A client that closes stdin at once ends an `-i` call before the first status line. A third party driving `luna-send -i` over plain `ssh` found it "needs a tty" and wrapped it in `script`. | I, D | From the two rows above; [plx] 1270-1272 |
| ares splits stdout on newlines and keeps appending lines until `JSON.parse` succeeds. `returnValue === false` is an error whose text is `errorText`, or `errorMessage`. | S | cli:lib/base/luna.js:63-105 |
| 3.2.5 has one table of services for every profile and webOS version. There is no switch between `luna-send` and `luna-send-pub`, and no per-version endpoint. | S | cli:lib/base/cli-appdata.js:37-40, 187-189; cli:files/conf/command-service.json |
| The one probe: before launch, close, running and list it calls `luna://com.webos.service.sessionmanager//getSessionList` and ignores a failure. Only a device that answers gets `-c <sessionId>` added to later calls. | S | cli:lib/session.js:18-68; cli:lib/base/luna.js:48-50 |
| ares-cli-rs sends the same calls, always with `luna-send-pub`, `-n 1` or `-i`, the URI and the JSON shell-quoted. | S | [acr-luna] 19-25, 44-50 |

### 6.2 The calls

Each line follows `/usr/bin/luna-send-pub `. They were rebuilt from ares'
own expressions and its `command-service.json` (S: cli:lib/install.js:204-239,
312-352, 409-441; cli:lib/launch.js:182-215, 300-333, 365-396, 581-602;
cli:lib/device.js:272-290).

| Purpose | Arguments | ares waits for |
|---|---|---|
| Install | `-i luna://com.webos.appInstallService/dev/install '{"id":"com.ares.defaultName","ipkUrl":"/media/developer/temp/<file>.ipk","subscribe":true}'` | a line whose `details.state` matches `/installed\|^SUCCESS/i`. `/FAILED/i` is the failure, with `details.reason` as its text |
| Remove | `-i luna://com.webos.appInstallService/dev/remove '{"id":"<app id>","subscribe":true}'` | `details.state` matching `/removed\|^SUCCESS/i`; `/FAILED/i` fails |
| List installed | `-n 1 luna://com.webos.applicationManager/dev/listApps '{"subscribe":false}'` | an `apps` array. ares drops entries whose `visible` is false, prints `id`, and reads `type` and `folderPath` |
| Launch | `-n 1 luna://com.webos.applicationManager//launch '{"id":"<app id>","subscribe":false,"params":{"displayAffinity":0}}'` | `returnValue` true |
| Close | `-n 1 luna://com.webos.applicationManager/dev/closeByAppId '{"id":"<app id>","subscribe":false,"params":{"displayAffinity":0}}'` | `returnValue` true |
| Running apps | `-n 1 luna://com.webos.applicationManager/dev/running '{"subscribe":false}'` | `returnValue` true and a `running` array of objects with an `id` |
| TV information | `-n 1 luna://com.webos.service.tv.systemproperty//getSystemInfo '{"keys":["modelName","sdkVersion","firmwareVersion","boardType","otaId"],"subscribe":false}'` | one object; ares prints the keys it finds |

| Fact | Tag | Source |
|---|---|---|
| `"com.ares.defaultName"` is a constant. The real id comes from the package: in a Developer Mode install the installer does not compare the two. | S | cli:lib/install.js:67; [ai2-parse] 114-121 (webOS OSE) |
| Launch parameters given to `ares-launch --params` are merged into `params`; ares adds `"displayAffinity":0` when the TV has no session manager. | S | cli:bin/ares-launch.js:198-228; cli:lib/launch.js:581-602 |
| `dev/install`, `dev/remove` are registered only while `/var/luna/preferences/devmode_enabled` exists. They need `id` and `ipkUrl`, respectively `id`. | S | [ai2-service] 58-66, 295-330; [ai2-settings] 51, 62-63; [ai2-schema] (webOS OSE) |
| The first reply to a subscribed install or remove is `{"returnValue":true,"subscribed":true}`, without `details`. ares survives it because the exception from reading `details.state` is swallowed by the `catch` meant for partial JSON. | S | [ai2-service] 366-371; cli:lib/base/luna.js:80-104 |
| Removing an unknown id answers `{"returnValue":false,"errorCode":-2,"errorText":"No such id","subscribed":false}`. | S | [ai2-service] 392-413 (webOS OSE) |
| `dev/closeByAppId`: error 2 "is not running", and error 2 "Only Dev app should be closed using /dev category_API". `launch` answers `returnValue`, `appId`, `instanceId`, `launchPointId`, `displayId`. | D | [ose-sam] (webOS OSE; the TV's answers are not documented) |
| ares-cli-rs calls `luna://com.webos.applicationManager/launch` with one slash. | S | [acr-launch] 16-24 |

### 6.3 The install stream

One public recording was found: `luna-send -i .../dev/install` run as root on
a rooted OLED55CX5LB, webOS 5.6.2 (D, [hearth-4]). After
`{"subscribed":true,"returnValue":true}` every line was
`{"id":"com.ares.defaultName","statusValue":<n>,"details":{...}}`, and the
command never returned by itself after the last one.

| `statusValue` | `details.state` |
|---|---|
| 268, 269, 270 | `ipk verifying` |
| 35, 36, 37, then 262, 263, 264 | `ipk parsing` (`packageId` and `unpackFileSize` are filled from 37 on) |
| 32, 33, 34 | `app closing` |
| 11, 12 | `installing` |
| 13 | `installing : start` |
| 17 | `installing : installing` |
| 27, 28, 29 | `installing : service installing` |
| 30 | `installed` |

`details` also carried `client` (`com.webos.lunasend-<pid>`), `ipkFile`,
`installBasePath` (`/media/developer`), `verified` (false), `progress`,
`simpleStatus`, `update`.

| Fact | Tag | Source |
|---|---|---|
| The open installer names the same states: `ipk parsing`, `installing`, `installing : start`, `app closing`, `installed`, `install failed` with `errorCode` and `reason`, and for removal `remove start`, `removing`, `removing ipk`, `removing service files`, `removing data`, `removing jail`, `removed`, `remove failed`. | S | [ai2-task] 330-425 (webOS OSE) |
| LG documents the numbers: 30 "All install operations succeeded", 24 install error, 31 removal done, 25 removal error, and the ranges seen above. | D | [ose-install] (webOS OSE) |
| Failure texts the open installer produces: `FAILED_IPKG_INSTALL`, `FAILED_PACKAGEFILE_CORRUPT`, `FAILED_NOT_ENOUGH_INSTALL_SPACE`, `FAILED_VERIFY`, "Failed to extract package", "Failed to parse control", "There is no available space to install App", "Cannot find appinfo.json". | S | [ai2-install] 48-52, 121-156; [ai2-parse] 92-106; [ai2-svcutil] 77-80 (webOS OSE) |
| A TV answered `error_code -5`, "Failed to extract package", to a package whose `ar` member names ended in `/`. | D | [plx] 1488-1496 (webOS 4.x TV) |

### 6.4 The order of an ares install, launch and close

`ares-install <file>` on a `tv` device (S: cli:lib/install.js:74-257):

1. `/usr/bin/test -d /media/developer/temp || /bin/mkdir -p /media/developer/temp`
2. the push of section 5 to `/media/developer/temp/<file>`
3. `/bin/ls -l "/media/developer/temp/<file>"`
4. `/usr/bin/tail -c 200 "/media/developer/temp/<file>" | /usr/bin/md5sum`
5. the Install call of 6.2, until `installed`
6. `/bin/rm -f "/media/developer/temp/<file>"`

`ares-launch <id>` sends the session probe, then Launch. `ares-launch --close
<id>` sends the probe, then Close. `ares-install --remove <id>` sends Remove
alone (S: cli:lib/launch.js:57-69, 264-269; cli:lib/install.js:260-361).

## 7. Ports and forwarding

| Port | What | Tag | Source |
|---|---|---|---|
| 9922 | SSH, section 3 | D | [lg-devmode] |
| 9991 | the key server, HTTP, section 2 | S | cli:lib/base/novacom.js:370 |
| 9998 | the web inspector of web apps; ares-inspect reaches it through a forward | S | cli:lib/inspect.js:22-25, 123-132 |
| 8080, 5885 and up | Node's inspector for JS services: 8080 before Node 8, else 5885 or the next free port | S | cli:lib/inspect.js:22-25, 134-164, 370-378 |

| Fact | Tag | Source |
|---|---|---|
| ares forwards with a `direct-tcpip` channel: a local `net` server, and per client `ssh.forwardOut("127.0.0.1", <client port>, "127.0.0.1", <device port>)`. `ares-novacom --forward --port <device>[:<host>]` exposes the same thing on the `tv` profile. | S | cli:lib/base/novacom.js:1106-1200; cli:bin/ares-novacom.js:169-197 |
| LG's tools therefore expect the TV's sshd to allow `direct-tcpip` to its own loopback. A device entry may say `noPortForwarding`; ares-inspect then uses `http://<tv>:<port>` directly. | I, S | From the row above; cli:lib/inspect.js:469-474 |
| ares-cli-rs also offers the reverse: `tcpip-forward` on the TV's `localhost`, "so something running on the device ... can reach a server on your machine". Its README adds that a `prisoner` login cannot bind ports below 1024. | S, D | [acr-novacom] 138-176; [acr-novacom-readme] 33-46 |
| The jail file is a list of mounts, copies and device nodes. It has no line about network isolation, and it copies `/etc/services` and `/etc/nsswitch.conf` and mounts `/sys/class/net`. So a jailed native app shares the TV's network stack with sshd: a port it opens on `127.0.0.1` is what a forward reaches. | I | From jail:1-685 |
| The TV reaching the computer over the LAN is part of LG's own workflow: `ares-launch --hosted` starts a web server on the computer and launches an app with `hostedurl` = `http://<computer's LAN address>:<port>/`. That is a web app, not a jailed native one. | S | cli:lib/launch.js:137-171 |

## 8. Logs, crashes and paths on the TV

| Fact | Tag | Source |
|---|---|---|
| LG's CLI has no log command for TVs: `ares-log` is for the `ose` and `apollo` profiles and needs root. Its `logFilePath`, `/media/developer/log/devlog`, is in the TV profile's config and used nowhere. | S | cli:files/conf/ares.json:20-23; cli:bin/ares-log.js:105; cli:lib/log.js:436-437; cli:files/conf-base/profile/config-tv.json:23 |
| Where the app manager sends a native app's stdout and stderr on a TV is not documented. | | Open question |
| A Developer Mode install lands in `/media/developer/apps`: the app in `/media/developer/apps/usr/palm/applications/<id>`, its package record in `/media/developer/apps/usr/palm/packages/<id>`, services in `/media/developer/apps/usr/palm/services/<id>`. | S | [ai2-settings] 30-38; [aiu-main] 65, 71-73; cli:lib/device.js:733 |
| A third party saw its running native app report `appdir: /media/developer/apps/...` from `/proc/self/exe`, with `Uid: 6910`, `Gid: 5000`, no capabilities, chrooted to `/var/palm/jail/<app id>`. | D | [plx] 424-427, 502 (49SM9000PLA, webOS 4.5) |
| The installer leaves the app directory `root:root 775` there. An app running as another uid, outside group `root`, cannot write into it. | D, I | [plx] 518-520 (same TV); from the uid and gid in the row above |
| What a Developer Mode native app can write: `/media/developer` (also its `HOME`) and `/tmp`. Both are shared by all Developer Mode apps. `/media/internal` is read-only. | D | jail:29-33, 420-422, 681-682 |
| The SSH session has `/media/developer` read-write in every version of the file, and the shared `/tmp` only up to the 10.0.0 file (section 4). | D | jail:420-422; jail11:30-41 |
| The file sets `LD_LIBRARY_PATH=/usr/lib:$APPDIR/lib` with `APPDIR` = `/media/cryptofs/apps/usr/palm/applications/$APPID`, the path of store apps. Whether the jailer overrides `APPDIR` for a Developer Mode app is not shown. | D | jail:14, 371-372 |
| The file sets `setrlimit CORE INF INF`. A third party's crash "writes a ~200 MB core into the app directory", on a partition of about 616 MB shared by `/media/developer` and `/media/cryptofs`. | D | jail:377; [plx] 602-607 (same TV) |
| The Developer Mode app ships a `gdbserver` at `.../com.palmdts.devmode.service/binaries-armv71/opt/devmode/usr/bin/gdbserver`; the jail links that directory to `/opt/devmode`. | S, D | [wb-gdb]; jail:399-418 |
| On removal the open installer deletes the app's jail and data (`removing jail`, `removing data`). When Developer Mode ends, `/media/developer` goes (section 1). | S | [ai2-task] 405-425 |

## 9. The device list ares keeps

`~/.webos/tv/novacom-devices.json` (S: cli:files/conf/config.json:29;
cli:lib/base/cli-appdata.js:31, 71-73) is a JSON array. A TV entry after
`ares-setup-device` and `ares-novacom --getkey` has (S:
cli:lib/base/setup-device.js:34-45, 326-337; cli:bin/ares-novacom.js:131-142;
cli:lib/base/novacom.js:292-326; cli:files/schema/NovacomDevices.schema):

| Field | Value |
|---|---|
| `profile` | `"tv"` |
| `type` | `"starfish"` |
| `name` | the user's name for the TV; may not start with `$` or `%` |
| `host` | an IPv4 address or `localhost` |
| `port` | `9922` |
| `username` | `"prisoner"` |
| `privateKey` | `{"openSsh": "<file name in ~/.ssh>"}` |
| `passphrase` | the six characters, in clear |
| `files` | `"sftp"` or `"stream"` |
| `default` | boolean; the entry ares uses when no device is named |
| `description` | free text |

The file starts with four `emulator` entries (`127.0.0.1:6622`, user
`developer`, key `webos_emul`, `indelible: true`), one per profile.
ares-cli-rs keeps a list of the same shape in another place,
`~/.webos/ose/novacom-devices.json` (S: [acr-io] 14, 95-111).

## Open questions

- Which OpenSSH the Developer Mode app ships on webOS 4 to 25 and later, and
  what it offers. The only banner in public is `OpenSSH_6.1` from webOS 3.5.
- Whether a TV offers an ECDSA host key (the jail file names one) or only
  `ssh-rsa` (the one error message says so).
- The size of the RSA key, and whether key or passphrase change when
  Developer Mode is toggled, the session is extended, the app is reinstalled
  or the TV reboots.
- What the Developer Mode app's own EXTEND button calls.
- Whether `sftp-server` exists for `prisoner`. ares tries it and falls back;
  nobody states the outcome on a TV.
- The uid and gid of `prisoner`, and whether it can read a file a jailed app
  wrote under `/media/developer` or, up to the 10.0.0 jail, `/tmp`.
- Which webOS release the 11.0.0 jail file belongs to. LG's server also
  answers unknown versions with some file (`99.0.0` got the 9.0.0 one).
- Where a native app's stdout and stderr go, and whether anything of them is
  readable from the SSH session.
- Whether the TV's sshd allows `direct-tcpip` and `tcpip-forward` for
  `prisoner` today. LG's tools assume the first, ares-cli-rs documents the
  second.
- Whether a port a jailed app listens on is reachable from the LAN directly.
- The exact answers of `launch`, `dev/closeByAppId`, `dev/running` and
  `getSystemInfo` on a TV, and how `params` reach a native app.
- Whether `dev/install` takes a package outside `/media/developer/temp`.
- How many sessions and channels the TV's sshd allows on one connection.
- Whether the Developer Mode app must be open for port 9922 to answer. LG's
  forum lists a 2026 thread, "Developer Mode enabled and key server working,
  but nothing listens on 9922", for a webOS 25 build of another licensee
  (title seen on [forum-ssh34]; the thread itself was not read).

## What this means for react-native-webos

- The channel is standard: HTTP for the key, SSH-2, `exec`, and JSON lines
  from `luna-send-pub`. There is no LG protocol to reimplement, unlike sdb.
- Fetching the key is one `GET http://<tv>:9991/webos_rsa`; decrypting it is
  one `crypto.createPrivateKey` call with the six-character passphrase. A
  wrong passphrase must be reported as such, not as a broken key.
- An npm SSH library is not unavoidable, but the alternative is real work.
  The choices are: `ssh2` (what ares uses: JavaScript, the dependencies
  `asn1` and `bcrypt-pbkdf`, and an install script that tries to build an
  optional native binding, ssh2:package.json); the system's `ssh` with
  `HostKeyAlgorithms=+ssh-rsa` and
  `PubkeyAcceptedAlgorithms=+ssh-rsa` and a decrypted key file; or an own
  SSH-2 client on `node:crypto`. For the last one a 6.1-era server needs
  `ecdh-sha2-nistp256`, an `ssh-rsa` host key check (SHA-1), `aes128-ctr`,
  `hmac-sha2-256` or `hmac-sha1`, public key login signed `ssh-rsa`, and
  channels of type `session` (exec, stdin, EOF, exit status) and
  `direct-tcpip`. Node has every primitive (section 2, I).
- Whatever the client, it must not offer only modern algorithms, must not
  pin the host key before the question above is answered, and should set TCP
  keepalive.
- One mechanism pushes a file on every TV both tools support: exec
  `cat > <path>` with the file on stdin. SFTP is optional.
- Name the remote file ourselves, as ares-cli-rs does: a fixed pattern
  ending in `.ipk` under `/media/developer/temp`, with no characters that
  need quoting. Verify it with `md5sum` or `sha256sum` before installing.
- `rn-webos` runs `/usr/bin/md5sum <path>` over the whole file and reads the
  32 hexadecimal digits that start the answer. That `md5sum` prints the digest
  first for a file argument, as it does for standard input, is inferred (I),
  not seen on a TV. Like ares it installs when the TV gives no digest, and
  stops only on a different one.
- It tries SFTP and falls back to `cat` whenever the SFTP session does not
  start, whatever the reason, as ares-cli-rs does.
- It keeps ssh2's default algorithms and appends the two SHA-1 key exchanges
  ares adds, so a newer sshd still gets the modern ones first.
- `rn-webos install --launch` takes the app to launch from `details.packageId`
  of the install stream, seen in the one recording of 6.3, else from a file
  named `<id>_<version>_<arch>.ipk`.
- Keep stdin open while an `-i` call runs, read lines until `details.state`
  is `installed` or contains `failed`, then close stdin. The stream does not
  end by itself.
- ares does not escape what it puts in a command. `rn-webos` validates app
  ids against `[a-z0-9.+-]` and quotes JSON for the shell.
- For the app's log stream, the route that does not depend on the jail
  version is the app connecting out to the computer. A `direct-tcpip`
  forward to a loopback port, and a file under `/media/developer` read with
  `tail -f`, are the alternatives; `/tmp` is not, from the 11.0.0 jail on.
- A native app must not assume it can write next to its executable, and
  should link with an rpath relative to itself instead of relying on
  `LD_LIBRARY_PATH`.
- `rn-webos` can read the session token over SSH and warn before Developer
  Mode expires; extending it is one HTTPS request to LG.

## Sources

[lg-devmode]: https://webostv.developer.lge.com/develop/getting-started/developer-mode-app
[lg-cli]: https://webostv.developer.lge.com/develop/tools/cli-dev-guide
[lge-jail]: https://developer.lge.com/common/file/DownloadFile.dev?sdkVersion=10.0.0&fileType=conf
[lge-jail-11]: https://developer.lge.com/common/file/DownloadFile.dev?sdkVersion=11.0.0&fileType=conf
[wb-devmode]: https://webosbrew.org/devmode
[wb-sdk]: https://webosbrew.org/pages/sdk-configuration
[wb-reset]: https://github.com/webosbrew/dev-utils/blob/b25ad97e737cb22181a8d962c10966c6a5327810/scripts/devmode-reset.sh
[wb-gdb]: https://github.com/webosbrew/ares-cli-ext/blob/f1c87d79ebfd86dfadffbaef4740a4b1dbd2769c/lib/gdbserver.js
[dmd-devmode]: https://github.com/webosbrew/dev-manager-desktop/blob/0251413b90eddbba64cee0b59f09070567d2a501/src-tauri/src/plugins/devmode.rs
[dmd-renew]: https://github.com/webosbrew/dev-manager-desktop/blob/0251413b90eddbba64cee0b59f09070567d2a501/src/app/info/renew-script/renew-script.sh.ts
[dmd-conn]: https://github.com/webosbrew/dev-manager-desktop/blob/0251413b90eddbba64cee0b59f09070567d2a501/src-tauri/src/conn_pool/connection.rs
[acr-io]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/common/device/src/io.rs
[dmd-163]: https://github.com/webosbrew/dev-manager-desktop/issues/163
[dmd-163-key]: https://github.com/webosbrew/dev-manager-desktop/issues/163#issuecomment-2331620340
[acr-session]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/common/connection/src/session.rs
[acr-setup]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/common/connection/src/setup.rs
[acr-transfer]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/common/connection/src/transfer.rs
[acr-luna]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/common/connection/src/luna/luna.rs
[acr-sub]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/common/connection/src/luna/subscription.rs
[acr-install]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-install/src/install.rs
[acr-launch]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-launch/src/launch.rs
[acr-novacom]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-novacom/src/main.rs
[acr-novacom-readme]: https://github.com/webosbrew/ares-cli-rs/blob/ed48a05897e1b646d2b86f473a2c2d5280daae61/ares-novacom/README.md
[ai2-service]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/service/AppInstallService.cpp
[ai2-settings]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/settings/Settings.cpp
[ai2-schema]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/files/schema/appInstallService.dev.install.schema
[ai2-parse]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/step/IpkParseStep.cpp
[ai2-install]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/step/IpkInstallStep.cpp
[ai2-task]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/installer/Task.cpp
[ai2-svcutil]: https://github.com/webosose/appinstalld2/blob/938ffd716c0c250bdb860100e9a41d3bd5b0b452/src/installer/ServiceInstallerUtility.cpp
[aiu-main]: https://github.com/webosose/applicationinstallerutility/blob/eb1cddc222cf8c64bba52ef9ed597fc6f2c99d49/src/Main.cpp.in
[ls2-send]: https://github.com/webosose/luna-service2/blob/5bc38aa348b35f5799f1bf9ed7c6cb513bc5f702/src/luna-send/luna-send.cpp
[ls2-cmake]: https://github.com/webosose/luna-service2/blob/5bc38aa348b35f5799f1bf9ed7c6cb513bc5f702/src/luna-send/CMakeLists.txt
[ose-install]: https://www.webosose.org/docs/reference/ls2-api/com-webos-appinstallservice/
[ose-sam]: https://www.webosose.org/docs/reference/ls2-api/com-webos-service-applicationmanager/
[openssh-61]: https://github.com/openssh/openssh-portable/blob/V_6_1_P1/myproposal.h
[openssh-62]: https://www.openssh.com/txt/release-6.2
[openssh-65]: https://www.openssh.com/txt/release-6.5
[openssh-88]: https://www.openssh.com/txt/release-8.8
[forum-ssh34]: https://forum.webostv.developer.lge.com/t/ssh-with-webos-3-4-0/1982
[forum-ssh35]: https://forum.webostv.developer.lge.com/t/how-to-ssh-into-lg-webos-tv/2134
[forum-getkey]: https://forum.webostv.developer.lge.com/t/ares-novacom-err-tips-failed-to-get-ssh-private-key/4482
[hearth-4]: https://github.com/rudra-avatar/Hearth/issues/4
[plx]: https://github.com/GLinnik21/plx-native/blob/f8f6dbb75c783de7c790608653e212a14c2ec66c/docs/distribution.md
[lglow]: https://github.com/IrvingGuerra/lglow/blob/a8c5ec75972e0cf617070221f9e593a1ae75b8dc/docs/en/02-developer-mode-trap.md
