# mono-nx

This is an unofficial port of the mono runtime to the switch homebrew toolchain. It can run dotnet 9.0 applications using the mono interpreter by loading the dll or exe files directly on console. It can also build .NET assemblies as static libraries using mono AOT.

![](notes/assets/explorer_demo.mp4)
This is a clip of the [explorer_demo](managed/explorer_demo) running on console. A C# app rendering a GUI with SDL2 and Imgui.
More clips: [pad_demo](notes/assets/pad_demo.mp4) and [guess_number](notes/assets/number_demo.mp4)

While a few things do work this is only an experiment. I don't plan on continuing development or fix bugs, take this project just as a proof of concept. There is minimal versioning and testing, you probably don't want to use this to actually make homebrew.

If you're curious about the process of porting mono to a weird platform I wrote a [write-up](notes/writeup.md) with some of the challenges I faced and my workarounds.

## What works

- Common BCL classes such as `List`, `StringBuilder` and so on
- P/Invoke, but only with libraries that were statically linked beforehand
- Threads and async
- Most of filesystem APIs
- Sockets and http-only support for `HttpClient`
- .NET wrappers for SDL2 and [dear imgui](https://github.com/ocornut/imgui) which are included as static libraries
- The interpreter seems rather stable even when running more complex programs, I did not test the AOT builds as much

## What does not work

- HTTPS and most of `System.Security` doesn't work due to the lack of openssl
- Abitrary P/Invoke doesn't work due to the lack of dynamic linking, all native function entrypoints must be defined beforehand and statically linked
- Any other OS-dependant API that was not mentioned previously will likely not work because it was not explicitly implemented. Examples are `Console.Read`, `Console.Clear` and many more.

Also, after running and exiting the interpreter launching another nro in the same hbmenu session will consistently crash in malloc down the mono call stack. It seems that that historically mono has had problems cleaning up resources, see [here](https://github.com/mono/mono/issues/20191) and [here](https://stackoverflow.com/questions/10651230/multiple-mono-jit-init-mono-jit-cleanup-issue). My workaround is to just terminate the process.

## Included demos

I prepared a few prebuilt examples in the releases tab:
- aot_example.nro is a fully self-contained AOT-complied C# application
- pad_input.dll is a C# program that reads the controls with the native libnx api
- example.dll is a C# program that shows most of the APIs that are currently implemented
- explorer_demo.dll is a C# program that shows a file explorer-like GUI using imgui and SDL2. This will also work on windows and linux with no code changes if you use [my cimgui fork](https://github.com/exelix11/CimguiSDL2Cross/releases/tag/r2)
- guess_number.exe is the source from aot_example built with plain `csc.exe` on a windows vm to show off loading exe files as well. I'm not aware of any way to build this on linux so there's no build script for it.

These last four are included in the `mono-interpreter.zip` release. It will add the dll and exe file association to the hbmenu allowing you to launch them directly.

## Trying your own programs

If you want to try making a switch homebrew in C# you can follow the examples in the `managed/` folder, you can build them using the normal dotnet 9.0 sdk for linux or windows and then copy the final dll files to your console.

You will probably want to setup one of the remote logging options in the [config.ini](sd_files/mono/config.ini) file to catch unhandled exceptions

> [!IMPORTANT]  
> Reminder for when you will hit things that do not work: **this is an unsupported port, do NOT open issues on the real dotnet/runtime repo about this**. If you want to help document what is borken you can open an issue in this repo but currently I do not plan on investigating and fixing more unsupported features.

# Building the runtime

The build steps are a little convoluted because of the number of moving parts, I tried to split everything to its own somewhat documented bash script so it should be easier to understand what's needed for what.

On a default ubuntu image you will need to manually install devkita64, CMake 3.20 or higher, libclang and dotnet-sdk-9.0. On other distros refer to the documentation on the dotnet/runtime repo. For your convenience I prepared a docker file which already has all the required components. You can enter the docker environment using:

```
docker build . -t monobuild
docker run --rm -it -v $(pwd):/mono-nx monobuild
```

First you will need to build icu, mono and the framework libraries. This is needed only once unless you want to try modifying mono.

1) Ensure the DEVKITPRO env var is set
2) Run `source env.sh` to define a few of the environment variables needed. This is done automatically in the docker image
3) Build libicu with `cd icu && ./build_icu.sh` 
4) Clone my fork of the dotnet/runtime repo `git clone -b libnx https://github.com/exelix11/dotnet_runtime.git`
4) Build mono with `./build_mono.sh` 
    - Fix all the inevitable depenency errors that appear and try again until it works
    - You can clean the state of the dotnet build system using `cd dotnet_runtime && ./buld.sh --clean`

If you want to modify mono or the runtime you should check out the build steps in `build_mono.sh` and only build the subsets you're trying to modify.

Now, depending on what you want to do you can:

- Build the C# assemblies for the interpreter in `managed` with `./managed_build.sh`
- Build the interpreter in `native/interpreter` with `make`
- Build the AOT example in `native/aot` with `./build_aot.sh` and then `make`

Finally prepare the sd card release `copy_sd_files.sh`