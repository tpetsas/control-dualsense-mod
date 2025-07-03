
[DSX]: https://store.steampowered.com/app/1812620/DSX/

# control-dualsense-mod

Built with the tools and technologies:

[![CMake](https://img.shields.io/badge/-CMake-darkslateblue?logo=cmake)](https://cmake.org/)
![C](https://img.shields.io/badge/C-A8B9CC?logo=C&logoColor=white)
![C++](https://img.shields.io/badge/-C++-darkblue?logo=cplusplus)
[![DSX++](https://img.shields.io/badge/DSX++-blue.svg?logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZlcnNpb249IjEiIHdpZHRoPSI2MDAiIGhlaWdodD0iNjAwIj48cGF0aCBkPSJNMTI5IDExMWMtNTUgNC05MyA2Ni05MyA3OEwwIDM5OGMtMiA3MCAzNiA5MiA2OSA5MWgxYzc5IDAgODctNTcgMTMwLTEyOGgyMDFjNDMgNzEgNTAgMTI4IDEyOSAxMjhoMWMzMyAxIDcxLTIxIDY5LTkxbC0zNi0yMDljMC0xMi00MC03OC05OC03OGgtMTBjLTYzIDAtOTIgMzUtOTIgNDJIMjM2YzAtNy0yOS00Mi05Mi00MmgtMTV6IiBmaWxsPSIjZmZmIi8+PC9zdmc+)](https://github.com/tpetsas/DSXpp)
[![Ghidra](https://tinyurl.com/yuv64wyh)](https://ghidra-sre.org/)
[![Radare2](https://tinyurl.com/52tue3ve)](https://rada.re/n/radare2.html)

> [!IMPORTANT]  
> Note those instructions are applicable only for the first version of the Control DualSense Mod version 1.0.0!

<img src="https://github.com/user-attachments/assets/d997ea9b-d3e0-4d07-8688-553caa2e3fd5" alt="dualsense-mod" width="400"/>

## Overview

This mod allows players with a Playstation 5 DualSense controller to play Control Ultimate Edition with adaptive triggers. The adaptive triggers are assigned based on the current weapon form.

Mod Page: [**Nexus Mods — Control DualSense Mod**](https://www.nexusmods.com/control/mods/108/)

### Mod Showcase

<a href="https://www.youtube.com/watch?v=ubN_qF-uWRU">
    <img src="https://github.com/user-attachments/assets/ba60759b-e5c1-421c-ac23-d36a8a7841b0"
        width="600"
        title="Control Ultimate Edition DualSense Mod Showcase — Click to watch it!"
    />
</a>

## Features

[DSX]: https://store.steampowered.com/app/1812620/DSX/
[2kreg]: https://next.nexusmods.com/profile/registrator2000?gameId=2936
[Control Plugin Loader]: https://www.nexusmods.com/control/mods/16

This mod adds the following features:
- Adaptive Triggers for both L2 and R2 for each weapon form
    - **Note:** I don't own a PS5 and I don't have access to the PS5 version of the game; I tried to replicate the adaptive trigger effects based on some videos I found on the web
- Adaptive triggers get disabled when the player is on an inner Menu or the game is paused

## Requirements

This mod requires the following:
- A thirdparty app: [DSX]. You can purchase this app from Steam. There is also a older version of the app you can [download for free](https://github.com/Paliverse/DualSenseX/releases/tag/1.4.9), though I haven't tested the mod there, so it is not certain if this version it is fully compatible with the mod.
- [Control Plugin Loader], a DLL loader that allows mods to modify low-level parts of the game using native code implemented by [2kreg].

## Installation

### Install the DSX app

Make sure you have a version of DSX app running

> **Set the DSX version**
> Switch to either DSX v3.1 (BETA at the time of writing) — **recommended**
> * Follow the instructions here: [DSX — How to Switch Branch on Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3330635677)
> or to DSX v2
> * There should be a button on the top left of the app
>
> **Note:** *DSX v3.0 is not supported!*

- **Important:** Make sure that Incoming UDP setting is enabled
To enable this setting, navigate to ⚙️ Settings **>** Networking in your DSX app:

<img src="https://github.com/user-attachments/assets/8e2cc201-2635-498d-9140-d09e0190c0b6" alt="enableUDP" width="400"/>

Click on "Fix Firewall Rules" and enable the "Incoming UDP" option. After doing that the Networking tab should look like this:

<img src="https://github.com/user-attachments/assets/92f90b9d-91f4-4338-8c67-8ca1778e8250" alt="UDPEnabled" width="400"/>

### Extract the Mod

Download the latest release of the dualsense-mod: [**control-dualsense-mod-v1.0.0.zip**](https://github.com/tpetsas/control-dualsense-mod/releases/download/1.0.0/control-dualsense-mod-v1.0.0.zip) and unzip it in Control's main directory. This should be in the following path: `C:\Program Files (x86)\Steam\steamapps\common\Control`

After doing that, a new directory should have been created in the Control's main directory with the name `plugins`and a new file should be added there too with the name `xinput1_4.dll` (this is our plugin loader). The content of Control's directory should look like this (the new content is highlighted in light blue color):

<img src="https://github.com/user-attachments/assets/ce8e9ecb-0890-406a-b023-ff26fc9996a2" alt="new-content" width="400"/>

Also, the content of `plugins` directory should look like this:

<img src="https://github.com/user-attachments/assets/e0075a26-4dd2-4fcb-adfd-5bbd06dc57ea" alt="plugins-content" width="400"/>

Mind that `modlog.log` is the file that the mod creates when the game launches and stores all the mod logs. It shouldn't be there right after unzipping the release zip, but you should be able to see it as soon as you launch the game.

## Configuration

The mod supports two configuration options as of now via an INI file stored in the `plugins` directory named `dualsense-mod.ini`

A sample content of the file is the following (also found in the current repo at `config/dualsense-mod.INI`:


```
[app]
debug=true

[dsx]
version=2.5
```

In this configuration, the `debug=true` option of the `[app]` section will make the mod to output a lot more information to its respective log file (`plugins\modlog.log`), while the `version=2.5` option of the `[dsx]` section will enable the legacy DSX version adaptive triggers. The default values of the above options (i.e., if no INI file is used) are `debug=false` & `vertion=3.1`.

## Issues

Please report any bugs or flaws! I recommend to grab a debug version of the mod (e.g., [**dualsense-mod-debug.dll**](https://github.com/tpetsas/control-dualsense-mod/releases/download/1.0.0/dualsense-mod-debug.dll)) and enable the `debug` option in the configuration as described above ([Configuration](#configuration)) in order to get a fully verbose log when trying to replicate the issue, which will help me a lot with debugging the issue. Feel free to open an issue [here](https://github.com/tpetsas/control-dualsense-mod/issues) on github or
reach out to the official DSX Discord Server: [![DSXDiscord](https://img.shields.io/discord/836790685784211486?logo=discord&label=DSX%20Discord)](https://discord.gg/PUThpCFByn), at the [dsx-mods](https://discord.com/channels/836790685784211486/1108637673100414977) channel!

## Credits

[2kreg] for [Control Plugin Loader] and all the knowledge and examples from all their other Control mods! Thanks! :metal:

[Tsuda Kageyu](https://github.com/tsudakageyu), [Michael Maltsev](https://github.com/m417z) & [Andrey Unis](https://github.com/uniskz) for [MinHook](https://github.com/TsudaKageyu/minhook)! :syringe:

[Paliverse](https://github.com/Paliverse) for making [DSX]! :video_game:
