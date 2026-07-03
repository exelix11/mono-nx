# mono-nx

This is an unofficial port of the mono runtime to the switch homebrew toolchain. It can run dotnet 9.0 applications using the mono interpreter by loading the dll or exe files directly on console. It can also build .NET assemblies as static libraries using mono AOT.

https://github.com/user-attachments/assets/ab57d77b-67ed-4c05-8017-9a1706f8c645

This is a clip of a [C# file explorer](managed/explorer_demo) with a GUI powered by SDL and imgui running on console.
More clips: [pad_demo](https://github.com/user-attachments/assets/ca9f7be8-cdde-4dae-8391-abbc80c1953b) and [guess_number](https://github.com/user-attachments/assets/1588dbc6-5fd3-4991-8f53-dc53a64074a6)

If you're curious about the process of porting mono to a weird platform I documented some of the challenges I faced and my workarounds in this [write-up](notes/writeup.md).

While a few things do work this at the proof-of-concept stage with minimal testing, you probably don't want to use this to actually make homebrew. While I would love to continue working on this project, it requires too much effort for a single person. For the time being I don't plan active development.

## What works

- Common BCL classes such as `List`, `StringBuilder` and so on
- P/Invoke, but only with libraries that were statically linked beforehand
- Threads and async
- Most of filesystem APIs
- Sockets and http-only support for `HttpClient`
- .NET wrappers for SDL2 and [dear imgui](https://github.com/ocornut/imgui) which are included as static libraries
- The interpreter seems rather stable even when running more complex programs, I did not test the AOT builds as much
- Unit tests with XHarness (some dotnet repo tests are even passing!)

## What does not work

- HTTPS and most of `System.Security` doesn't work because we have no openssl port on switch
- Arbitrary P/Invoke doesn't work due to the lack of dynamic linking, all native function entrypoints must be defined beforehand and statically linked
- Any other OS-dependant API that was not mentioned previously will likely not work because it was not explicitly implemented. Examples are `Console.Read`, `Console.Clear`, `Process`, `WinForms` and many more.

Also, exiting the interpreter and launching another dll or sometimes homebrew in the same hbmenu session will eventually crash. I'm not sure why this happens, historically mono has had problems [cleaning up resources](https://github.com/mono/mono/issues/20191) but it could also be an issue related to the homebrew environment. I tried to do [some debugging](https://github.com/exelix11/mono-nx/blob/master/notes/writeup.md#the-smoke-test) but ultimately my workaround is to just terminate the process on exit.

## Release content

The `mono-interpreter.zip` release contains the mono interpreter, all the dotnet runtime libraries and the file .dll/.exe file association for hbmenu allowing you to copy .net dll programs to your switch folder and running them directly. 

The following pre-built examples are included:
- aot_example.nro is a fully self-contained AOT-complied C# application. It generates a random number and asks you to guess it
- pad_input.dll is a C# program that reads the controls with the native libnx api
- example.dll is a C# program that shows most of the APIs that are currently implemented
- explorer_demo.dll is a C# program that shows a file explorer-like GUI using imgui and SDL2. This will also work on windows and linux with no code changes if you use [my cimgui fork](https://github.com/exelix11/CimguiSDL2Cross/releases/tag/r2)
- guess_number.exe is the source from aot_example built with plain `csc.exe` on a windows vm to show off loading exe files as well. I'm not aware of any way to build this on linux so there's no build script for it.

## Trying your own programs

If you want to try making a switch homebrew in C# you can follow the examples in the `managed/` folder, you can build them using the normal dotnet 9.0 sdk for linux or windows and then copy the final dll files to your console.

You will probably want to setup one of the logging options in [config.ini](sd_files/mono/config.ini) to catch unhandled exceptions. DllImport/PInvoke for functions that are not statically defined in [dl_shim.c](native/shared/dl_shim.c) requires to use a custom build of the interpreter or it will not work.

AOT requires [additional steps](notes/aot.md)

> [!IMPORTANT]  
> Reminder for when you **will** hit things that do not work. **this is an unsupported port, do NOT open issues on the real dotnet/runtime.**. If you want to help document what is broken you can open an issue in this repo, but as of now there is no support.

# Building

Mono gets statically linked to our interpreter and aot NROs, this means before you're able to build them you need your own local dotnet_runtime build. The build steps are a little convoluted, I tried to split everything to its own bash script so it should be easier to understand what's needed for what.

On a default ubuntu image you will need to manually install devkita64, CMake 3.20 or higher, libclang and dotnet-sdk-9.0. On other distros refer to the documentation on the dotnet/runtime repo. For your convenience I prepared a docker container which already has all the required components. You can enter the docker environment using:

```
docker build . -t monobuild
docker run --rm -it -v $(pwd):/mono-nx monobuild
```

## Prebuilt SDK

Now, if you feel lazy and bold at the same time you can try the [prebuilt sdk release](https://github.com/exelix11/mono-nx/releases/tag/rel-1). This is produced by the `gather_sdk.sh` script by grabbing only the final headers and static libraries needed for building the nros. Extract it in the repo root and lets you skip building the dotnet runtime repo. However I don't think it will work across toolchain upgrades, this means you must use the same docker container I used when I built the zip since it pins the toolchain version.

If you get unexplainable issues, try to build the runtime yourself.

## Building the dotnet runtime

1) Ensure the DEVKITPRO env var is set
2) Run `source env.sh` to define the environment variables needed. This is done automatically in the docker image
3) Build libicu with `cd icu && ./build_icu.sh` , this is a dependency for dotnet_runtime
4) Clone the `libnx` branch from [my fork of the dotnet/runtime](https://github.com/exelix11/dotnet_runtime/tree/libnx) repo `git clone --depth=1 -b libnx https://github.com/exelix11/dotnet_runtime.git`
4) Build mono with `./build_mono.sh` 
    - If you're not using the docker container, fix all the inevitable dependency errors that appear and try again until it works
    - You can clean the state of the dotnet build system using `cd dotnet_runtime && ./buld.sh --clean`

If you want to modify mono or the runtime you should check out the build steps in `build_mono.sh` and only build the subsets you're working on.

## Building mono-nx

You'll most likely want to start working with the interpreter in `native/interpreter`, build it with `make`.
You can build the C# examples in `managed` with `./managed_build.sh`
For AOT `cd native/aot` then in order, `./build_aot.sh` and finally `make`.

The sd card zip in releases is built with `copy_sd_files.sh`
