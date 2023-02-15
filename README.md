# CK (Food's Official Compiler)
Welcome to the official repository of the CK compiler.
## How to build
CK uses [premake5](https://github.com/premake) for its building system. The binaries
are already supplied in the `premake5/` directory. premake generates build scripts for other
build systems (e.g. `make`, `msbuild`/Microsoft Visual Studio, etc.)  
The repository root contains a file called `premake5.lua`. That is the build script.
To generate a build script for your system, execute premake while specifying the target build
system. You also have to specify the configuration to use with `config=name` (two are available
at the moment, `Debug` and `Release`.) Then, a build script for your system will be generated
and CK will be ready to compile.