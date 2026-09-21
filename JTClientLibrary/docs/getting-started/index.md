# Overview

SRO_DevKit uses a number of different tools you might not have dealt with,
yet. This page contains all the information you need to download the 
project, set up your build environment and compile the project successfully.

## Is this project right for me?

* This project is a work in process. It is not flawless. Things will change 
as development continues and the project evolves. If you want something ready to 
use, this project ain't for you.
* This project requires personal initiative. We don't have step by step guides for everything, 
and we won't create such any time soon. If you do not want to take the initiative and don't 
want to figure things out on your own, this project is not for you.
* This project uses C++. There is no way to get around C++. If you don't want to deal 
with C++, this project is not for you.
* Overall, this project requires you to learn new skills and work on your own. If, for whatever reason, 
you cannot do that, this project is not for you.
* The contents of this documentation have been written carefully to ensure proper setup 
of your build environment. Following all the steps will ensure the correct setup of your 
build environment. If you're unable to follow simple steps shown on screenshots and read texts 
without skipping to the end, this project is not for you.


## Setup your build environment

SRO_DevKit relies on Visual Studio 2005. Nobody will use Visual Studio 2005 by 
choice, for sure. That's why this project uses CMake to support a variety of 
Visual Studio Versions, and also other IDEs aswell.

Choose your favourite IDE from the list below.

| IDE                                                            | Compile                     | Debug                  |
|----------------------------------------------------------------|-----------------------------|------------------------|
| [Jetbrains CLion](supported-ides/clion/index.md)               | ✅ Working                  | ✅ Working             |
| Visual Studio 2008, 2010, 2012, 2013, 2015, 2017               | ❌ No longer supported      | ❌ No longer supported |
| Visual Studio 2019                                             | ❗ CMake integration bugged | ? Unknown              |
| [Visual Studio 2022](supported-ides/visual-studio/index.md)    | ✅ Working                  | ✅ Working             |
| [Visual Studio Code](supported-ides/vscode/index.md)           | ✅ Working                  | ✅ Working             |

> ✅ Everything is fine
> ❗ Known issues existing
> ❌ Requires lot of work or is just undoable

If you got any issues regarding these configurations, please create an issue so
we can improve the documentation.


## Configuration

SRO_DevKit offers several configuration options. Depending on your needs, 
you can easily enable or disable predefined features with CMake-options. 
The following options are configureable:


| Option | Description | Default value |
|--------|-------------|---------------|
| CONFIG_TRANSLATIONS_DEBUG | Print Tokens instead of translated strings | OFF |
| CONFIG_OLD_UNDERBAR | Turn the old EXP Bar ON/OFF | OFF |
| CONFIG_CHATVIEWER | Use the custom chatviewer supplied in SRO_DevKit | OFF |
| CONFIG_CHATVIEWER_BADWORDFILTER | Enable the Bad Word Filter | ON |
| CONFIG_IMGUI | Enable ImGui | ON |
| CONFIG_DEBUG_REDIRECT_PUTDUMP | Redirect the PutDump output to the console | ON |
| CONFIG_DEBUG_CONSOLE | Show the debug console | ON |
| CONFIG_DEBUG_NET_RECEIVE | Print NetProcess debug messages on receive | OFF |


To change one of these options, create a CMakeUserPresets.json and define your own CMake preset. Your 
configuration options go into the `cacheVariables` section. Don't forget to change the selected preset in your IDE to your own one.

```json
{
    "version": 6,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 19,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "My Own Config",
            "inherits": "RelWithDebInfo-cfg",
            "generator": "Ninja",
            "binaryDir": "cmake-build-relwithdebinfo",
            "cacheVariables": {
                "CONFIG_IMGUI": "OFF",
                "CONFIG_DEBUG_CONSOLE": "OFF"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "My Own Config",
            "configurePreset": "My Own Config"
        }
    ]
}
```
