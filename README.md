# mono-nx

This is an unofficial port of the mono runtime to the switch homebrew toolchain. It can run dotnet 9.0 applications using the mono interpreter by loading the dll or exe files directly on console. It can also build .NET assemblies as static libraries using mono AOT.

https://github.com/user-attachments/assets/ab57d77b-67ed-4c05-8017-9a1706f8c645

This is a clip of a [C# file explorer](managed/explorer_demo) with a GUI powered by SDL and imgui running on console.
More clips: [pad_demo](https://github.com/user-attachments/assets/ca9f7be8-cdde-4dae-8391-abbc80c1953b) and [guess_number](https://github.com/user-attachments/assets/1588dbc6-5fd3-4991-8f53-dc53a64074a6)

If you're curious about the process of porting mono to a weird platform I documented some of the challenges I faced and my workarounds in this [write-up](notes/writeup.md).

While a few things do work this at the proof-of-concept stage with minimal testing, you probably don't want to use this to actually make homebrew. While I would love to continue working on this project, it requires too much effort for a single person. For the time being I don't plan further development.

## What works

- Common BCL classes such as `List`, `StringBuilder` and so on
- P/Invoke, but only with libraries that were statically linked beforehand
- Threads and async
- Most of filesystem APIs
- Sockets and http-only support for `HttpClient`
- .NET wrappers for SDL2 and [dear imgui](https://github.com/ocornut/imgui) which are included as static libraries
- The interpreter seems rather stable even when running more complex programs, I did not test the AOT builds as much

## What does not work

- HTTPS and most of `System.Security` doesn't work because we have no openssl port on switch
- Arbitrary P/Invoke doesn't work due to the lack of dynamic linking, all native function entrypoints must be defined beforehand and statically linked
- Any other OS-dependant API that was not mentioned previously will likely not work because it was not explicitly implemented. Examples are `Console.Read`, `Console.Clear`, `Process`, `WinForms` and many more.

Also, exiting the interpreter and launching another dll or sometimes homebrew in the same hbmenu session will eventually crash. I'm not sure why this happens, historically mono has had problems [cleaning up resources](https://github.com/mono/mono/issues/20191) but it could also be an issue related to the homebrew environment. I tried to do [some debugging](https://github.com/exelix11/mono-nx/blob/master/notes/writeup.md#the-smoke-test) but ultimately my workaround is to just terminate the process on exit.

## Included demos

I prepared a few prebuilt examples in the releases tab:
- aot_example.nro is a fully self-contained AOT-complied C# application. It generates a random number and asks you to guess it
- pad_input.dll is a C# program that reads the controls with the native libnx api
- example.dll is a C# program that shows most of the APIs that are currently implemented
- explorer_demo.dll is a C# program that shows a file explorer-like GUI using imgui and SDL2. This will also work on windows and linux with no code changes if you use [my cimgui fork](https://github.com/exelix11/CimguiSDL2Cross/releases/tag/r2)
- guess_number.exe is the source from aot_example built with plain `csc.exe` on a windows vm to show off loading exe files as well. I'm not aware of any way to build this on linux so there's no build script for it.

These last four are included in the `mono-interpreter.zip` release. It will add the dll and exe file association to the hbmenu allowing you to launch them directly.

## Trying your own programs

If you want to try making a switch homebrew in C# you can follow the examples in the `managed/` folder, you can build them using the normal dotnet 9.0 sdk for linux or windows and then copy the final dll files to your console.

You will probably want to setup one of the logging options in the [config.ini](sd_files/mono/config.ini) file to catch unhandled exceptions.

AOT requires [additional steps](notes/aot.md)

> [!IMPORTANT]  
> Reminder for when you **will** hit things that do not work. **this is an unsupported port, do NOT open issues on the real dotnet/runtime.**. If you want to help document what is broken you can open an issue in this repo, but as of now there is no support.

# Building the runtime

The build steps are a little convoluted because of the number of moving parts, I tried to split everything to its own somewhat documented bash script so it should be easier to understand what's needed for what.

On a default ubuntu image you will need to manually install devkita64, CMake 3.20 or higher, libclang and dotnet-sdk-9.0. On other distros refer to the documentation on the dotnet/runtime repo. For your convenience I prepared a docker file which already has all the required components. You can enter the docker environment using:

```
docker build . -t monobuild
docker run --rm -it -v $(pwd):/mono-nx monobuild
```

First you will need to build icu, mono and the framework libraries. This is needed only once unless you want to add new features.

1) Ensure the DEVKITPRO env var is set
2) Run `source env.sh` to define the environment variables needed. This is done automatically in the docker image
3) Build libicu with `cd icu && ./build_icu.sh` 
4) Clone the `libnx` branch from [my fork of the dotnet/runtime](https://github.com/exelix11/dotnet_runtime/tree/libnx) repo `git clone --depth=1 -b libnx https://github.com/exelix11/dotnet_runtime.git`
4) Build mono with `./build_mono.sh` 
    - Fix all the inevitable dependency errors that appear and try again until it works
    - You can clean the state of the dotnet build system using `cd dotnet_runtime && ./buld.sh --clean`

If you want to modify mono or the runtime you should check out the build steps in `build_mono.sh` and only build the subsets you're working on.

Now, depending on what you want to do you can:

- Build the C# assemblies for the interpreter in `managed` with `./managed_build.sh`
- Build the interpreter in `native/interpreter` with `make`
- Build the AOT example in `native/aot` with `./build_aot.sh` and then `make`

Finally prepare the sd card release `copy_sd_files.sh`
