# control-dualsense-mod
A mod for Control Ultimate Edition that adds DualSense adaptive trigger settings for all Service Weapon forms

Built with the tools and technologies:

[![CMake](https://img.shields.io/badge/-CMake-darkslateblue?logo=cmake)](https://cmake.org/)
![C](https://img.shields.io/badge/C-A8B9CC?logo=C&logoColor=white)
![C++](https://img.shields.io/badge/-C++-darkblue?logo=cplusplus)
[![Ghidra](https://tinyurl.com/yuv64wyh)](https://ghidra-sre.org/)
[![Radare2](https://tinyurl.com/52tue3ve)](https://rada.re/n/radare2.html)

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
- [Control Plugin Loader], a DLL loader that allows mods to modify low-level parts of the game using native code implemented by [2kreg].

## Installation

**Note:** For the older version of the mod that requires the [DSX] third party app to work, refer to these instructions: [![Control DualSense Mod v1.0.0](https://img.shields.io/badge/control--dualsense--mod-version%201.0.0-red)](https://github.com/tpetsas/control-dualsense-mod/blob/main/README.dsx.md)


**Todo:** New Instructions here...

## Configuration

The mod supports two configuration options as of now via an INI file stored in the `plugins` directory named `dualsense-mod.ini`

A sample content of the file is the following (also found in the current repo at `config/dualsense-mod.INI`:


```
[app]
debug=true
```

In this configuration, the `debug=true` option of the `[app]` section will make the mod to output a lot more information to its respective log file (`plugins\dualsensemod.log`). The default value of the above option (i.e., if no INI file is used) are `debug=false`.

## Issues

Please report any bugs or flaws! I recommend to grab a debug version of the mod (e.g., [**dualsense-mod-debug.dll**](https://github.com/tpetsas/control-dualsense-mod/releases/download/1.0.0/dualsense-mod-debug.dll)) and enable the `debug` option in the configuration as described above ([Configuration](#configuration)) in order to get a fully verbose log when trying to replicate the issue, which will help me a lot with debugging the issue. Feel free to open an issue [here](https://github.com/tpetsas/control-dualsense-mod/issues) on github or
reach out to the official DSX Discord Server: [![DSXDiscord](https://img.shields.io/discord/836790685784211486?logo=discord&label=DSX%20Discord)](https://discord.gg/PUThpCFByn), at the [dsx-mods](https://discord.com/channels/836790685784211486/1108637673100414977) channel!

## Credits

[2kreg] for [Control Plugin Loader] and all the knowledge and examples from all their other Control mods! Thanks! :metal:

[Tsuda Kageyu](https://github.com/tsudakageyu), [Michael Maltsev](https://github.com/m417z) & [Andrey Unis](https://github.com/uniskz) for [MinHook](https://github.com/TsudaKageyu/minhook)! :syringe:

[Paliverse](https://github.com/Paliverse)! :video_game:
