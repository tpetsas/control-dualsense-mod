# control-dualsense-mod
A mod for Control Ultimate Edition that adds DualSense adaptive trigger settings for all Service Weapon forms

Built with the tools and technologies:

[![CMake](https://img.shields.io/badge/-CMake-darkslateblue?logo=cmake)](https://cmake.org/)
![C](https://img.shields.io/badge/C-A8B9CC?logo=C&logoColor=white)
![C++](https://img.shields.io/badge/-C++-darkblue?logo=cplusplus)
[![Ghidra](https://tinyurl.com/yuv64wyh)](https://ghidra-sre.org/)
[![Radare2](https://tinyurl.com/52tue3ve)](https://rada.re/n/radare2.html)

<img width="512" alt="logo" src="https://github.com/user-attachments/assets/dc0e5cd3-47fa-4c78-a27d-d0524a01b67d" />

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

## Installation

[Control DualSense Mod v1.0.0]: https://github.com/tpetsas/control-dualsense-mod/releases/tag/1.0.0

> **Note:** For the older version of the mod ([Control DualSense Mod v1.0.0]) that requires the [DSX] third party app to work, refer to these instructions: [![Control DualSense Mod Old](https://img.shields.io/badge/control--dualsense--mod-version%201.0.0-red)](https://github.com/tpetsas/control-dualsense-mod/blob/main/README.dsx.md)

[installer-link]: http://TBD
[Control DualSense Mod v2.2.0]: https://github.com/tpetsas/control-dualsense-mod/releases/tag/2.2.0

### :exclamation: Windows SmartScreen or Antivirus Warning

If Windows or your antivirus flags this installer or executable, it’s most likely because the file is **not digitally signed**.

This is a known limitation affecting many **open-source projects** that don't use paid code-signing certificates.

#### :white_check_mark: What you should know:
- This mod is **open source**, and you can inspect the full source code here on GitHub.
- It **does not contain malware or spyware**.
- Some antivirus programs may incorrectly flag unsigned software — these are known as **false positives**.

You may safely proceed by clicking:

> **More info → Run anyway** (for SmartScreen)  
> or temporarily allow the file in your antivirus software.


**1. Download the **[Control-DualSensitive-Mod_Setup.exe][installer-link]** from the latest version ([Control DualSense Mod v2.2.0])**



**2. Double click the installer to run it:**

<img width="491" alt="setup" src="https://github.com/user-attachments/assets/02d96c32-27cb-49e4-baf3-782187208b0b" />



**3. Accept the disclaimer and follow the prompts until the setup is complete:**

<img width="491" height="382" alt="disclaimer" src="https://github.com/user-attachments/assets/ace9f161-88f4-4af0-8ea0-02a027240634" />



**Once all steps are completed, you will reach the final screen indicating that the setup is finished:**

<img width="491" height="383" alt="finished" src="https://github.com/user-attachments/assets/c0d28e1d-006c-43d7-a5de-f6f17c43655f" />



Now, you can experience the mod by just running the game.

## Uninstallation

To unisntall the game, simply go to **Settings > Add or remove programs**, locate the mod, choose uninstall and follow the prompts:

<img width="491" alt="uninstall" src="https://github.com/user-attachments/assets/22c12d2a-42f6-47ab-935b-f314701b09bb" />


## Usage & Configuration
[tray-options]: https://github.com/tpetsas/dualsensitive/blob/main/README.md#tray-application-options

The mod will start as soon as the game is started. You can enable/disable the adaptive triggers feature any time from the tray app. For more information, check the [DualSensitive Tray Application Options section][tray-options]. The tray app closes automatically when the game exits.

The mod supports two configuration options as of now via an INI file stored in the `plugins` directory named `dualsense-mod.ini`

A sample content of the file is the following (also found in the current repo at `config/dualsense-mod.INI`:


```
[app]
debug=true
```

In this configuration, the `debug=true` option of the `[app]` section will make the mod to output a lot more information to its respective log file (`plugins\dualsensemod.log`). The default value of the above option (i.e., if no INI file is used) are `debug=false`.

## Issues

Please report any bugs or flaws! I recommend to grab a debug version of the mod (e.g., [**dualsense-mod-debug.dll**](https://github.com/tpetsas/control-dualsense-mod/releases/download/1.0.0/dualsense-mod-debug.dll)) and enable the `debug` option in the configuration as described above ([Configuration](#configuration)) in order to get a fully verbose log when trying to replicate the issue, which will help me a lot with debugging the issue. Feel free to open an issue [here](https://github.com/tpetsas/control-dualsense-mod/issues) on github.

## Credits

[2kreg] for [Control Plugin Loader] and all the knowledge and examples from all their other Control mods! Thanks! :metal:

[Tsuda Kageyu](https://github.com/tsudakageyu), [Michael Maltsev](https://github.com/m417z) & [Andrey Unis](https://github.com/uniskz) for [MinHook](https://github.com/TsudaKageyu/minhook)! :syringe:
