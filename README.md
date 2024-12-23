# control-dualsense-mod
A mod for Control Ultimate Edition that adds Dualsense adaptive trigger settings for all Service Weapon forms

Built with the tools and technologies:

[![CMake](https://img.shields.io/badge/-CMake-darkslateblue?logo=cmake)](https://cmake.org/)
![C](https://img.shields.io/badge/C-A8B9CC?logo=C&logoColor=white)
![C++](https://img.shields.io/badge/-C++-darkblue?logo=cplusplus)
[![DSX++](https://img.shields.io/badge/DSX++-blue.svg?logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZlcnNpb249IjEiIHdpZHRoPSI2MDAiIGhlaWdodD0iNjAwIj48cGF0aCBkPSJNMTI5IDExMWMtNTUgNC05MyA2Ni05MyA3OEwwIDM5OGMtMiA3MCAzNiA5MiA2OSA5MWgxYzc5IDAgODctNTcgMTMwLTEyOGgyMDFjNDMgNzEgNTAgMTI4IDEyOSAxMjhoMWMzMyAxIDcxLTIxIDY5LTkxbC0zNi0yMDljMC0xMi00MC03OC05OC03OGgtMTBjLTYzIDAtOTIgMzUtOTIgNDJIMjM2YzAtNy0yOS00Mi05Mi00MmgtMTV6IiBmaWxsPSIjZmZmIi8+PC9zdmc+)](https://github.com/tpetsas/DSXpp)
[![Ghidra](https://tinyurl.com/yuv64wyh)](https://ghidra-sre.org/)
[![Radare2](https://tinyurl.com/52tue3ve)](https://rada.re/n/radare2.html)

<img src="https://github.com/user-attachments/assets/d997ea9b-d3e0-4d07-8688-553caa2e3fd5" alt="dualsense-mod" width="400"/>

## Overview

This mod allows players with a Playstation 5 Dualsense controller to play Control Ultimate Edition with adaptive triggers. The adaptive triggers are assigned based on the current weapon form.

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

- Make sure you have a version of DSX app running

> ### Set the DSX version
> Make sure to switch to either DSX v2:
> * There should be a button on the top left of the app
>
>or DSX v3.1 (BETA at the time of writing) — **recommended**
>* Follow the instructions here: [DSX — How to Switch Branch on Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3330635677)
>
> **Note:** *DSX v3.0 is not supported!*

- **Important:** Make sure that Incoming UDP setting is enabled
To enable this setting navigate to ⚙️ Settings **>** Networking in your DSX app
*TODO:* _Add arrows and boxes to show the exacts settings that need to be changed!_

<img src="https://github.com/user-attachments/assets/49766968-f24b-4839-918f-4fea32781c5a" alt="enableUDP" width="400"/>

Make sure to click on "Fix Firewall Rules" and enable the "Incoming UDP" option. After doing that the Networking tab should show the following:

<img src="https://github.com/user-attachments/assets/011eb449-56a3-4304-82e3-95583e33fdcf" alt="UDPEnabled" width="400"/>

- Download the latest release of the dualsense-mod: XXX and unzip it in Control's main directory. This should be in the following path: `C:\Program Files (x86)\Steam\steamapps\common\Control`

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

## Credits

[2kreg] for [Control Plugin Loader] and all the knowledge and examples from all their other Control mods! Thanks! :metal:

[Paliverse](https://github.com/Paliverse) for making [DSX]!
