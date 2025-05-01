# Installing
Expected usage of Visual Code and Cmake for running builds.

Install plugins for VS Code [C/C++](https://marketplace.visualstudio.com/items/?itemName=ms-vscode.cpptools) and [CMakeTools](https://marketplace.visualstudio.com/items/?itemName=ms-vscode.cmake-tools)

Cntrl+Shift+P - running commands.

1. Select configure preset `CMake: Select Configure preset`
- win - for windows. It uses msbuild v143 toolchain
- mac - for Darwin. It uses Ninja
2. Select build toolset `CMake: Select Build preset`. There are presets for debug and release. After that you can build with F7
3. Run application with one of the configurations.
