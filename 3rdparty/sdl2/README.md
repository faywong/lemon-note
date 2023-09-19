# SDL2

This is a copy of the SDL source code with the premake build system added. It was loosely based on the
[premake4 build system for SDL written by renpy][1], however I have only learned premake5 and almost all
of that knowledge is not applicable to premake4.

The official source release of SDL can be found [here][2]. 

## Building SDL2 with premake5
So far, the premake files I have written only support Windows. As such I have only included the 
batch files for building on Windows. I will add support for Linux and MacOS if and when I need it or if there
is sufficient demand.
### Windows
To build the solution and project files for SDL2:

 1. Install [Git][3] (or [Github Desktop][4] if you prefer)
    and clone the repository in the command prompt with
    ```bat
    git clone https://github.com/SpookyScaryStudios/SDL2 target/directory
    ```
 2. Run `GenerateProjectFiles.bat` and open the `SDL.sln` file in the root directory.

To link SDL2 to another project:

 1. Clone the repository into your project as a git submodule in the command prompt with
    ```bat
    git submodule add https://github.com/SpookyScaryStudios/SDL2 target/directory
    ```
 2. Include `SDL2.lua` and `SDL2main.lua` into your project's premake file. These can be found in the `Build`
    directory, in folders of the same name.

## Structure
The project has a directary structure similar to that of Unreal Engine:

| Directory   | Description                                               |
|-------------|-----------------------------------------------------------|
| Binaries    | Binary directory.                                        |
| Build       | Where premake files, batch files and binaries are stored. |
| docs        | SDL documentation (copied from the [SDL source][2]).      |
| include     | SDL include path  (copied from the [SDL source][2]).      |
| Intermediate | Intermediates and project files.                           |
| src         | SDL source code   (copied from the [SDL source][2]).      |

## Contributing
Please feel free to contribute to this repository by reporting errors or even submitting pull requests. I
really appreciate it if you do, and will try to resolve the issue or merge the pull request as soon as
possible.

## License
This project is licenced under the [zlib licence][5], since it is a modified version of SDL2.


[1]: https://github.com/renpy/SDL2
[2]: https://www.libsdl.org/download-2.0.php
[3]: https://git-scm.com/downloads
[4]: https://desktop.github.com
[5]: https://www.zlib.net/zlib_license.html
