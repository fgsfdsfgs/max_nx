## Max Payne Mobile Nintendo Switch port

This is a wrapper/port of the Android version of Max Payne Mobile. It loads the original game binary, patches it and runs it.
It's basically as if we emulate a minimalist Android environment in which we natively run the original Android binary as is.
This is probably not entirely stable yet, so expect issues.

This is a port of the [PS Vita version](https://github.com/fgsfdsfgs/max_vita) adapted to work on the Switch with AArch64 `.so` files.

### How to install

This requires access to the following system calls:
* `svcMapProcessCodeMemory` (0x77)
* `svcUnmapProcessCodeMemory` (0x78)
* `svcSetProcessMemoryPermission` (0x73)

I actually have no idea what that entails except the fact that you have to be running a CFW. Good luck.

You're going to need:
* `.apk` file for version 1.7 (latest version at the time of writing) that includes `arm64-v8a` libraries;
* `.obb` file for version 1.6 or 1.7 (usually located at `/sdcard/android/obb/com.rockstar.maxpayne/main.3.com.rockstar.maxpayne.obb`).

Both files [can be obtained](https://stackoverflow.com/questions/11012976/how-do-i-get-the-apk-of-an-installed-app-without-root-access) from your phone if you have a copy of the game installed.
Both files can be opened or extracted with anything that can extract `.zip` files.

To install:
1. Create a folder called `maxpayne` in the `switch` folder on your SD card.
2. Extract **the contents of** the `assets` folder from your `.apk` to `/switch/maxpayne/`.
3. Extract `lib/arm64-v8a/libMaxPayne.so` from your `.apk` to `/switch/maxpayne/`.
4. Extract the contents of the `.obb` file into `/switch/maxpayne/`. You can skip all the `.msf` files except for `MaxPayneSoundsv2.msf` and any extra languages that you want.
5. Extract the contents of the `.zip` file from the latest release into `/switch/maxpayne/`. Replace everything.

### Notes

This **will not work** in applet/album mode: even though `MEMORY_MB` can very well be lowered to 232MB in `config.h`, there's still 0MB left after the newlib heap is allocated.
This can probably be fixed later. For now use a game override or NSP hbmenu.

The port has an extra config file, located at `/switch/maxpayne/config.txt`. It is created when you first run the game and allows you to tweak some internal settings.
For more detailed descriptions of said settings check the [wiki article](https://github.com/fgsfdsfgs/max_nx/wiki/Config-variables).

The game will show an error message if it detects that anything is wrong. Please read those and check the relevant parts of your setup.

If the error says `Could not find symbol ...`, that likely means you have the wrong `.apk` or `.so` file. You need version 1.7, others **will not work**.
Make sure also that you've extracted `libMaxPayne.so` from the `arm64-v8a` subfolder.

If the game crashes on startup, please post an issue with your last crash report attached.

### How to build

You're going to need devkitA64 and the following libraries:
* `switch-mesa`
* `switch-libdrm_nouveau`
* `switch-sdl2`
* `devkitpro-pkgbuild-helpers`
* [openal-soft](https://github.com/fgsfdsfgs/openal-soft)

After you've obtained all the dependencies and ensured devkitA64 is properly installed and the `DEVKITPRO` environment variable is set,
build this repository using the commands:
```
git clone https://github.com/fgsfdsfgs/max_nx.git && cd max_nx
source $DEVKITPRO/switchvars.sh
make
```

### Credits

* TheOfficialFloW for the method and work on the PS Vita port;
* Rinnegatamante, Bythos, frangarcj, CBPS for help with the PS Vita port;
* Freakler for providing the icon;
* Switchbrew for libnx.

### Legal

This project has no direct affiliation with Take-Two Interactive Software, Inc., Rockstar Games, Inc. or Remedy Entertainment Oyj and/or the "Max Payne" brand. "Max Payne" is a Take-Two Interactive Software, Inc. brand. All Rights Reserved.

No assets or program code from the original game or its Android port are included in this project. We do not condone piracy in any way, shape or form and encourage users to legally own the original game.

The video game "Max Payne" is copyright © 2001 Remedy Entertainment Oyj and/or Take-Two Interactive Software, Inc. The Android version, "Max Payne Mobile", is copyright © 2012 Rockstar Games, Inc. and/or Take-Two Interactive Software, Inc. "Max Payne" and "Max Payne Mobile" are trademarks of their respective owners. All Rights Reserved.

Unless specified otherwise, the source code provided in this repository is licenced under the MIT License. Please see the accompanying LICENSE file.
