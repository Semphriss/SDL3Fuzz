# SDL3Fuzz

Fuzz SDL3 projects with random user input.

This project is currently bare-bones and lacks many features. Nevertheless, it
can be useful to experiment on and to fuzz real apps, although inefficiently.

SDL3Fuzz is a personal project, and is not affiliated with SDL.

## Overview

SDL3Fuzz is compiled as a shared library that can be dynamically loaded with
any application using SDL3. Access to the application's source code is not
required, but it must be able to load a patched SDL3, either directly or through
the [dynamic API](https://wiki.libsdl.org/SDL3/README-dynapi).

Loading an application with the SDL3Fuzz library will automatically generate
random inputs (such as mouse movements, clicks, and keyboard presses) at
irregular intervals. Like traditional fuzzing, the fuzzer will run until a
crash happens.

## Building

Currently, only POSIX environments are supported.

Compile and patch SDL3 using [SDL3_3.4.8.patch](./SDL3_3.4.8.patch). As the name
suggest, the patch is based on SDL 3.4.8. Other versions of SDL3 should work
fine, including older versions, but they have not been tested.

```sh
$ git clone -b release-3.4.8 https://github.com/libsdl-org/SDL.git SDL3
$ cd SDL3
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
$ cmake --build . --parallel
$ sudo cmake --install .
```

Then, build SDL3Fuzz:

```sh
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build . --parallel
```

This should give you a library called `libsdl3fuzz.so` in the build folder.

SDL3Fuzz currently does not come with an installation target, as it does not
need to be installed.

## Usage

Identify an application you wish to fuzz. The only requirement is that it uses
SDL. You can use [sdl2-compat](https://github.com/libsdl-org/sdl2-compat) if the
application uses SDL2. If it links dynamically to SDL3, replace the SDL3 library
with the patched version; otherwise, you can probably use the `SDL3_DYNAMIC_API`
to make it load the patched version.

*Please run SDL3Fuzz under a virtual machine.* Depending on what your app does,
it can mess with your system in many ways.

To run the application with SDL3Fuzz, run:

```
LD_PRELOAD=/path/to/SDL3Fuzz/build/libsdl3fuzz.so ./my_application
```

Or, preferably, under a debugger:

```
LD_PRELOAD=/path/to/SDL3Fuzz/build/libsdl3fuzz.so gdb ./my_application -ex run
```

SDL3Fuzz will then continuously generate input for your application.

If you see an issue about missing symbols, make sure that the dynamic linker
uses the patched version of SDL.

Some environment variables can control how SDL3Fuzz behaves:
- `SDLFUZZ_MAX_DELAY_MS` specifies the maximum delay between two events. No
  maximum is set by default. The minimum value is 0.
- `SDLFUZZ_SEED` specifies a seed as a 64-bit unsigned integer.
- `SDLFUZZ_LOGLEVEL` specifies what SDL3Fuzz should print. Current values are 0
  for no logging (default) and 1 to log basic information (currently, only the
  seed).

## Examples

Example projects to test fuzzing are located in the [examples](./examples/)
folder. Currently, only one example is provided. To build it, add the
`-DSDL3FUZZ_BUILD_EXAMPLES=ON` option when running CMake, then build or rebuild.
The programs can then be found in the `examples` folder within the build
directory.

The basic program can be run directly from the build folder with
`./examples/basic`. Can you find how to crash the program without looking the
source code? Can you make the program crash after looking at the source code?

Run with the fuzzer: `LD_PRELOAD=./libsdl3fuzz.so gdb ./examples/basic`, then
instruct GDB to run. **This will generate flashing lights!** After a moment, GDB
should stop on a crash. You can experiment with the `SDLFUZZ_MAX_DELAY_MS`
environment variable to make the process quicker.

## Limitations and roadmap

The project is currently bare-bones and will receive a lot of improvements, for
example:
- Fuzzing more input methods (joysticks, text input, clipboard, etc.)
- A basic SDL-compatible stub library, for better performance and no side
  effects like a visible window or organic events coming from the environment
- More flexibility over the operations
- Recording and replaying organic usage of the apps to base fuzzing tests on
- An orchestrator to record and keep track of event sequences that result in a
  crash

## License

Like SDL, SDL3Fuzz is licensed under the zlib license. See
[LICENSE.txt](./LICENSE.txt) for details.
