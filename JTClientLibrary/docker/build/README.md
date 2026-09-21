# Building with Visual Studio 2005 in Docker on Wine

This part creates a docker container with staging wine from winehq. You have to supply a wine prefix in `wine_prefix` with the following tools installed:

* Visual Studio 2005
    * Only needs the C++ part
* CMake
* Ninja

In a future update, I plan to automate the installation process. It already works for CMake and Ninja, but I had no luck with installing Visual Studio in Docker (.NET 2.0 fails).

