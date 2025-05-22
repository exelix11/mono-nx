# Porting mono to the switch homebrew toolchain

Something that I wanted to try for long time now was attempting to run C# on my modded switch.

Languages like lua, javascript, python and more recently web assembly have portable implementations that can be ran on embedded systems with minimal requirements. Other languages like Java and C# often have heavier requirements and are considered harder to port, but how hard exactly? 

I've not been able to find a clear answer to this. This is why I'm writing this sort-of write up about my experience porting mono: this is the blog post I wished I could find when I started this experiment.

If you haven't checked it out yet you can find a more high-level description of my mono port in the [main readme](https://github.com/exelix11/mono-nx/tree/master).

Throughout this article I assume the reader is familiar with C and C# programming concepts and common terms of VM-based languages such as what is JIT or the garbage collector.

I started this project about three months ago but I'm only writing this article now for the public release, while I wanted to make it a porting reference unfortunately I didn't write everything down in my notes so at times it might be not accurate or skip over issues I forgot about.

You should also keep in mind that while I feel confident enough about my C# knowledge, I have never worked with dotnet internals before, a lot of the content in this article is based on my assumptions and experiments hacking on the runtime.

## Setting expectations

What can we expect to run on a switch? What operating system does it run anyway? 

Most of my knowledge on this is gathered from sources such as [SwitchBrew](https://switchbrew.org/wiki/Main_Page), reading code on github and occasionally talking to other community members. I'll limit my description only to the things relevant to the porting work, if you're interested you should definitely check out the proper sources.

Switch runs a custom OS codenamed Horizon (abbreviated to HOS), unlike Windows or Linux it follows a microkernel design where most of functionality is implemented in user-mode services that an application can use through inter-process communication (IPC).

Since the goal is to run C# on a modded switch everything must be done with the unofficial [devkita64 toolchain](https://devkitpro.org/wiki/Getting_Started) which produces `.nro` executable homebrew applications that can be loaded with [nx-hbmenu](https://github.com/switchbrew/nx-hbmenu)

This toolchain comes a series of limitations, I assume due to the fact that early homebrew needed to run in a post-exploitation environment. Nowadays we have stable ways to load code in clean processes but these design decisions still remain.

In particular the bulk of the available API is implemented in [libnx](https://github.com/switchbrew/libnx), it provides a C standard library based on newlib, service wrappers to call the native IPC-based API of the OS and some posix APIs to aid application porting. In fact, switch does not run a posix OS making it a somewhat weird environment to port code: most common APIs such as pthreads are available (or at least, most functions are), while others like signal handling, `mmap` and `epoll` and even `dlopen` are not. As we'll see, these examples were not picked at random.

My initial goal was to get a real C# environment to run with proper garbage collection and at least a subset of the standard library. Another thing I assumed from the start was the need for an interpreter, that's because even on a modded console HOS does not allow RWX dynamic memory allocation which is often an assumption in Linux and Windows software.

I started by thinking about what would be the best approach, I've considered multiple ones before ultimately settling on mono.

- [Native AOT](https://learn.microsoft.com/en-us/dotnet/core/deploying/native-aot) was my first thought, this new AOT runtime can compile managed code to arm64 static libraries. Quickly inspecting the intermediate .a files for a linux-arm64 build revealed they depend on the PAL (platform abstraction library) and the AOT runtime. I started poking around in the source and soon became overwhelmed by the complex build system on the official dotnet/runtime repo, but I wasn't too concerned about that yet since I wanted to first evaluate the code.<br>The first blocking issue I noticed was exception handling. It seems that with NativeAOT even managed `NullReferenceExceptions` will trigger an access violation and go through signals or [VEH](https://learn.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling) on Windows. Handling a data abort is [probably possible](https://switchbrew.org/wiki/SVC#Exception_handling) on libnx but I'm not aware of any reference that restores the context instead of terminating after catching an exception, doing all the research and experimenting for this felt way too much prep work for a proof of concept that in the end might not even work. Also, from my native AOT android experiments I knew that the unix PAL requires `mmap`-ing a huge chunk of memory, adding a risk of hitting a dead end eventually. I know there are multiple GC engines which maybe could sidestep this issue but did not investigate further.
- [CoreCLR](https://github.com/dotnet/runtime/tree/main/src/coreclr) is the runtime you get when running modern dotnet on supported platforms, most of the considerations for Native AOT apply here too. I also assumed that since this is the main binary for first-class platforms it would have a lot more OS-dependent code.
- [bflat](https://github.com/bflattened/bflat) is an interesting project, a build tool that uses the roslyn API to build native dotnet code with a custom runtime. It also comes with a `zerolib` demo that can build binaries that don't depend on the framework at all such as UEFI binaries. I was confident I could get this last part to work on switch, but my goal was something at least resembling the real runtime.
- [dotnet anywhere](https://github.com/chrisdunelm/DotNetAnywhere) was the most promising of the bunch, however I remember playing around with it a few years ago already (yes, this thing has been in my mind for that long) and hitting a wall with 64-bit support, I did not give it a second chance.
- [Mono AOT](https://www.mono-project.com/docs/advanced/aot/) is a very promising concept, like NativeAOT it compiles C# into native code on top of the mono runtime. We can't use its most common configuration that produces dynamic libraries because we don't have support for that on switch. However it seems it also supports producing static libs with a few gotchas. The main issue for this is that we need to build the rest of the mono runtime for our target first.
- [WineHQ/Mono](https://gitlab.winehq.org/mono/mono): By now, I pretty much decided I'd try to port the mono interpreter, the question was which one? Apparently there is a non-microsoft fork from WineHQ which has the original build system. This was an interesting avenue since there's [prior work](https://github.com/Stary2001/mono/commit/e269d7ace09a28624bf1d0e519364cee72bc30f7) on getting it to run on switch but that didn't seem to go far enough to justify using it.

In the end, since the general consensus on the internet seems to be that "mono is easier to port" I decided to just bite the bullet of the crazy build system and try to port the one from the dotnet/runtime repo with the eventual goal of maybe being able to run AOT builds.

## On the shoulders of giants

Well then. Clone the repo, read the relevant docs, now what?

I started by trying to build just the mono runtime, that is, the native side with no base class library (BCL) or the native platform libraries. I also made sure my build environment was correctly configured by building the native linux version of it by running `./build.sh --subset mono.runtime`.

At this point I started looking into adding my custom target to the build system, I quickly found out that aside from the common first class targets dotnet also supports quite the variety of platforms, [haiku](https://www.haiku-os.org/) in particular is very interesting because most of its implementation happened in public over github issues and pull requests providing precious documentation and examples.

In fact, I started by following the changes made in this [pull request](https://github.com/dotnet/runtime/pull/86303) which adding the necessary haiku scaffolding to the build system. I decided to name my platform `libnx` rather than `switch` because i wanted to make clear the unofficial nature of this port.

The switch OS model is closer to a mobile device than a PC, we don't have a compiler or even an interactive shell on console so I had to resort to implement it as a cross compilation target. For this the tizen target was also a good reference.

So I spent the initial hours of my porting work to search for all the haiku references in the build scripts and project files so i could add my libnx target there too. I also had to manually write the cmake toolchain configuration file to be as similar as possible to the makefile we commonly use to build homebrew.

The only quirk is the need to define the `ROOTFS_DIR` environment variable because the build system assumes that cross building requires a rootfs, which is also how the target platform is identified.

```
export ROOTFS_DIR=$DEVKITPRO
```

I just set this to the devkitpro root folder.

```cmake
elseif(EXISTS ${CROSS_ROOTFS}/libnx/include/switch.h)
  set(CMAKE_SYSTEM_NAME Libnx)
  set(LIBNX 1)
```

Once everything was in place, I could start building mono by running `./build.sh --subset Mono.Runtime --cross -a arm64 --os libnx`, which obviously resulted in an endless amount of build errors.

I approached this by first reducing the amount of source files needed, I disabled most of the feature flags I could find, the current [CMakeLists.txt](https://github.com/exelix11/dotnet_runtime/blob/libnx/src/mono/CMakeLists.txt) file for mono looks like this:

```cmake
elseif(CLR_CMAKE_HOST_OS STREQUAL "libnx")
  set(HOST_LINUX 1) # We want to use most existing posix code
  set(HOST_LIBNX 1)
  
  # I still don't understand where are the HOST_ macros being set, as a hack I'm manually defining it here.
  add_definitions(-DHOST_LIBNX)
  
  set(DISABLE_SHARED_LIBS 1) # We don't have dynamic linking support
  set(DISABLE_EXECUTABLES 1) # We also don't have a command line or a shell, mono will be invoked by a wrapper application
  
  # Initially i disabled components, however during runtime porting i found out that we need the marshal-ilgen component for proper pinvoke
  set(STATIC_COMPONENTS 1)
  
  # Misc options i set by trial and error
  set(ENABLE_PERFTRACING 0)
  set(DISABLE_LOG_PROFILER_GZ 1)  
  set(HAVE_GNU_STRERROR_R 0)
  set(HAVE_SIGACTION 0)
```

After this most of the work was a very repetitive loop of build, fix the top few errors, repeat. 

WASM/WASI support was very useful as well, since these platforms are often under a strict sandbox they are just as different from POSIX as our libnx environment, providing useful pointers to what functions to work on.

Many `#ifdefs` later, I had a static library built for libnx and no way to test it.

![Success, finally](https://github.com/user-attachments/assets/6b3d2234-0f27-465d-945c-32e4bdf95390)

I wrote a quick wrapper using the mono embedding API and tried it on console. As one might expect, it resulted in an immediate crash. A little underwhelming, I know.

At first I tried debugging with the GDB stub implementation we have on switch, however I decided to switch to logging and testing on an emulator, with a few symlinks I could reduce my build-test cycle to a few seconds which greatly helped. It's hard to overstate how helpful a short build-test cycle can be.

After figuring out how to listen to mono's internal logging events, i started to make real progress since I started getting clean error messages and source locations.

![Humble beginnings](https://github.com/user-attachments/assets/9a348e0f-67f9-495b-a41e-bed93f228688)

With a few more fixes I was finally able to get to the point of mono attempting to load the core library `System.Private.CoreLib.dll` which contains most of the base types of C# such as `string`, `List` and so on.

![Slowly getting there](https://github.com/user-attachments/assets/74b2d500-9b76-45b4-b9a6-b8b930864b49)

Here I was somewhat confident that it could already run a managed dll if I found a way to build it without a reference to the core library, something simple like a method that returns an integer. However that was not my goal so I went ahead with porting the core library.

> [!NOTE]
> With the hindsight of having finished the port, this probably wouldn't have worked. Mono requires the corelib dll very early in its execution to load the metadata of some core C# classes.

## System.Private.CoreLib

Many dotnet libraries are split between a managed component and a native component. Building the managed component is usually easy enough since it's just a C# project, while the native part requires some porting work.

As a sanity check to ensure mono was actually loading and executing some IL code I also tried to copy over a `System.Private.CoreLib.dll` i had laying around on my PC. 

![Mono attempting to load System.Native](https://github.com/user-attachments/assets/fdf09177-ef5f-462e-8eda-09f4c0790f63)

This was the first tangible success, the logs show that mono started executing the constructors for some managed classes and eventually failed at the first P/Invoke call, which is what I expected since the native part was still missing.

> [!NOTE]
>Again, after finishing the port I now know there are a thousand of things that won't work with this approach besides P/Invoke. But still, seeing the result definitely gave me more motivation to continue the project.

At this point the astute reader might be wondering how does P/Invoke work if the switch homebrew toolchain doesn't have dynamic linking.
The answer is that it isn't doing any dynamic linking. Using mono's `mono_dl_fallback_register` function the wrapper application simulates `dlopen`/`dlsym` by just returning function pointers to functions that have been linked statically.

There are a few very long files containing only the dynamic function definitions:

![](https://github.com/user-attachments/assets/7c96b134-d281-4e00-b0d1-a8f5ca6f42b6)

I generated these definitions by extracting the symbols from the .a libraries and transforming them with some regex and text editing macros.

Once the dynamic loading issue was "solved", I decided it was about time to start porting the corelib. I started by building only the managed part, this required a small amount of changes mostly in the form of `ifdefs` and updating the project file.

The command I use to build the managed corelib is `./build.sh --subset mono.corelib --cross -a arm64 --os libnx`.

I then moved to building the native libraries, this is the command i used: `./build.sh --subset libs.native --cross -a arm64 --os libnx`. Note that `libs.native` is not limited to the corelib but includes all the native code needed by the other BCL libraries.

However, the [`CMakeLists.txt`](https://github.com/exelix11/dotnet_runtime/blob/libnx/src/native/libs/CMakeLists.txt#L144) file reveals that only two are needed for basic functionality:

```cmake
add_subdirectory(System.Native)
add_subdirectory(System.Globalization.Native)

if (CLR_CMAKE_TARGET_BROWSER OR CLR_CMAKE_TARGET_WASI OR CLR_CMAKE_TARGET_LIBNX)
        # skip for now
        ...
```

With the usual loop of build-fix-try again, I managed to build `System.Native`, however I hit a roadblock with `System.Globalization.Native` since it requires the [unicode icu](https://github.com/unicode-org/icu) library.

The switch toolchain has a variety of [portlibs](https://devkitpro.org/wiki/portlibs), which are these common third party libraries already ported for use in switch homebrew but icu was not among these. How hard could it be to port it to switch?

Turns out the Haiku OS people are one step ahead again, with a very handy reference on cross building ICU in the [docs](https://unicode-org.github.io/icu/userguide/icu4c/build.html#how-to-cross-compile-icu). It required some `./config` fiddling and a single `ifdef` change to build without errors.

While I initially missed it, this is also a good time to add your platform to the various files listing the supported [RIDs](https://learn.microsoft.com/en-us/dotnet/core/rid-catalog#rid-graph) and platforms. To my knowledge these are [`PortableRuntimeIdentifierGraph.json`](https://github.com/dotnet/runtime/blob/main/src/libraries/Microsoft.NETCore.Platforms/src/PortableRuntimeIdentifierGraph.json), [`Microsoft.NETCore.Platforms\runtime.json`](https://github.com/dotnet/runtime/blob/main/src/libraries/Microsoft.NETCore.Platforms/src/runtime.json) and [`eng/versioning.targets`](https://github.com/dotnet/runtime/blob/main/eng/versioning.targets). But it is possible that I missed something else.

With some more hacks and `ifdef`s in `System.Native` and minor changes in `System.Globalization.Native` I finally had the two static libraries compiled and could try running a real C# program on mono for switch.

![The first C# program running on switch](https://github.com/user-attachments/assets/52150436-d368-4c06-b9c0-23bae6a6507d)

Note that at this time I was still manually driving mono by doing something like the following (error handling omitted, and trust me, there were lots of errors).

```c
MonoDomain *domain = mono_jit_init("embedded_mono");

MonoAssembly *assembly = mono_domain_assembly_open(domain, "/mono/app/example.dll");

MonoImage *image = mono_assembly_get_image(assembly);
MonoClass *klass = mono_class_from_name(image, "Test", "TestClass");
MonoMethod *method = mono_class_get_method_from_name(klass, "TestEntrypoint", 0);

void *callargs[2];
int32_t arg1 = 10;
int32_t arg2 = 20;
callargs[0] = &arg1;
callargs[1] = &arg2;

MonoObject *exception = NULL;
MonoObject *result = mono_runtime_invoke(method, NULL, callargs, &exception);
int32_t result_value = *(int32_t*)mono_object_unbox(result);
printf("Result: %d\n", result_value);
```

Eventually I switched to invoking the `Main` method by calling `mono_jit_exec` with the `assembly` pointer. This is because I noticed that mono was doing more initialization behind the scenes and wanted to make sure the managed code would run in a normal environment.

## Porting the rest of the runtime

With an hello world working and a better understanding of mono I had a clearer goal in mind. I wanted to get more basic functionality working, in particular I focused on file access, threading, async and finally network.

Also, since this was all an experiment I did not go through the systematic testing present in the runtime source but wrote a simple C# file to test only the features I cared about. This is not an exhaustive approach but it's a start, and after all, a proper port was not my goal.

I went back to the loop of build, fix errors and repeat. This time most of the work was fixing up the project files rather than the code itself. Most of the time I only needed to add support for the `libnx` platform and include the existing implementation files, sometimes the same as `unix`, other times the same as `wasi`.

It was all downhill from here, with the corelib in place I started getting nice exception messages and stack traces with every unimplemented feature, it was just a matter of looking it up in the source and fixing it.

Two changes that are worth noting in this post are:
- Implementing `SystemNative_SysLog`, this is used by the default `DebugProvider` class and allowed me to receive BCL internal logs as well as adding my own logging using `Debug.WriteLine` to better debug issues.
- Implementing `SystemNative_GetCpuUtilization`, even as a stub, allowed async code to work. Even though I tried to guard it on the managed side with `UnsupportedOSPlatform` and other asserts, `PortableThreadPool` depends on it and won't catch exceptions.<br>It should be noted that changing the public API of the BCL, even with just attributes, is its own can of worms since any changes must be reflected in the `ref` set of sources. In the end I decided against doing that. 

I should also mention the `System.Resources.UseSystemResourceKeys` AppContext switch. This flag controls whether the corelib uses proper error messages from its resources or the hardcoded resource keys. Initially the `StackTrace` class constructor was causing a fatal exception when accessing these strings via the `SR` class. I couldn't understand why this was happening but reading the code led me to use `AppContext.SetSwitch("System.Resources.UseSystemResourceKeys", true);` as a workaround which fixed my test program.
Eventually, towards the end of the project I tried again without setting the switch and it was working fine, some change in the build configuration must have fixed the issue without me noticing. When I started working on AOT the issue surfaced again, this time a flag in `ILLink` fixed it.

With all of this out of the way I put together a simple test app for the set of features I cared about, and it worked fine in the emulator.

My enthusiasm was short lived though, as soon as I tried to run it on my console the result was not as good.

![Stock image for an application error](https://github.com/user-attachments/assets/c4494cd7-5821-4390-9db4-71642147dce9)

I went to check the crash report and found something weird.

```
Exception Info:
    Type:                        Instruction Abort
    Address:                     00000029ffa37460
Crashed Thread Info:
    Thread ID:                   0000000000000a5e
    Stack Region:                000000438ca3c000-000000438cb3c000
    Registers:
        ...
        LR:                      000000121aff217c (testapp + 0x2ba17c)
        SP:                      000000438cb3ae60
        PC:                      00000029ffa37460
    Stack Trace:
        ReturnAddress[00]:       000000121affccf4 (testapp + 0x2c4cf4)
        ReturnAddress[01]:       000000121b0030cc (testapp + 0x2cb0cc)
        ReturnAddress[02]:       000000121afa96fc (testapp + 0x2716fc)
        ReturnAddress[03]:       000000121af5ec90 (testapp + 0x226c90)
```

The error indicates that the code jumped somewhere in memory that was not valid executable memory. The `PC` register contains the address of the code that mono tried to execute, I immediately noticed that's very far from the addresses of the main binary that are in the stack trace.

This must be a bug, I thought.

Assuming this crash was the result of a function call with the `bl` instruction, the `LR` register contains the address of the caller, which looks correct as it's inside of the memory range of the application, so i started by checking it out.

This is easy since switch builds also produce a normal `.elf` file with symbols, there are multiple ways to figure out the address to source line mapping, my preferred one is loading the elf in binary ninja since this also flattens C macros removing possibly confusing syntax.

![Disassembly of the caller](https://github.com/user-attachments/assets/b1bd42f5-ba5e-4733-8ee5-e96ffea859e0)

## JIT? In my interpreter?

As it turns out it was not a bug.

Even when mono is initialized with the `MONO_AOT_MODE_INTERP_ONLY` flag it will emit P/Invoke trampolines dynamically.

A P/Invoke trampoline is a function pointer that sets the register state to jump from the interpreter into the target native function, these need to be specialized for each function signature since the code will change depending on the number and type of arguments that need to be passed.

Essentially, the emulator was helpfully ignoring memory access permissions and everything worked as intended. But when ran on a console, where the real OS enforces the W^X memory policy it failed.

What are our options? How do other platforms handle it?
- Windows and unix platforms just let mono allocate memory with RWX privileges. This is done in the `src/mono/mono/utils/mono-codeman.c` file. This is not going to work on switch since, while we do have access to the [OS JIT API](https://github.com/switchbrew/libnx/blob/master/nx/include/switch/kernel/jit.h), it's W^X only.
- Web assembly builds pre-generate the list of P/Invoke stubs they need and compile them statically with the runtime, this is done with the `GEN_PINVOKE` build flag. This seems exactly what we'd need, but it depends on a lot of complexity in the build system which is likely tedious to figure out. 
- Iphone and other consoles seem to use mono AOT builds, these produce dynamic libraries ahead of time with all the P/Invoke stubs already populated. While this was one of the possible goals it felt like a chicken-and-egg problem. I needed the runtime working on-console for AOT but at this point had no way of knowing if the runtime really worked.

Here I took a few weeks of break from this project because I seemed to be stuck at a major roadblock and kept putting it off, to the point of almost giving up.

>[!Note]
> Looking back now, if I had started working on AOT here I might have given up since that turned out to require a lot more debugging work. I might have decided that was not worth the effort without the guarantee of a working runtime.

Eventually I decided to try implementing a switch code manager that uses the native JIT API. This was somewhat of an hard task because the switch JIT model is very different than what you find on other platforms. Usually, even if you want to maintain a W^X policy, you can allocate RW memory, write to it and change its permissions to RX. In Win32 terms it looks like this.

```c
void* memory = VirtualAlloc(NULL, SIZE, ... , PAGE_READWRITE);

generate_code(memory);

VirtualProtect(exec_mem, SIZE, PAGE_EXECUTE_READ, ...);

MyFunctionType function = (MyFunctionType)memory;
function();
```

This works under the assumption that changing the memory protection does not change its address. And it seems to be a widely accepted convention in most JIT engines. But it is not true on switch where this code would look like the following:

```c
Jit codeArea;
jitCreate(&codeArea, SIZE);
jitTransitionToWritable(&codeArea);

generate_code(codeArea->rw_addr);

jitTransitionToExecutable(&codeArea);

MyFunctionType function = (MyFunctionType)codeArea->rx_addr;
function();
```

Here `rw_addr` and `rx_addr` are not guaranteed to be the same, and in fact in all of my testing they were always views of the memory mapped at different addresses. Unfortunately this means this wasn't a trivial port and it's likely not upstreamable, but that wasn't the plan anyway.

> [!NOTE]
> It is worth noting that as far I know, officially released software on the switch is not allowed to use the JIT API for security reasons. Not even the browser does.

One additional issue came from mono's source itself. While it does have the concept of "can I write executable memory" it is not bound to specific memory regions but to a thread as a whole. This is implemented in the `mono_codeman_enable_write` and `mono_codeman_disable_write` functions.

One one hand this was useful, it let me figure out all the places that emit dynamic code by looking for references to these functions. On the other hand it means that mono JIT is designed to work with RWX memory and won't have it any other way.

If I wanted to port JIT support I had to find a way to introduce the concept of different rw and rx memory views to the `code_manager` interface. The optimal approach would have been to introduce a struct representing both, however this required too many changes in the logic since the memory pointers are passed around quite a bit during code generation.

The way I implemented this was to add two new functions with additional semantic: 
- `mono_codeman_enable_write_ex`: Takes an RX pointer to a previously allocated JIT area, switches its permissions to RW and returns the proper RW address. This also accounts for offsets, if it's passed `rx_base + 0x10` it will return `rw_base + 0x10`. This is done with a simple O(n) lookup logic in a global list of allocated blocks, but it's good enough for our purposes.
- `mono_codeman_disable_write_ex`: Does the opposite as the `enable_write` function, it takes an RW pointer and returns the RX one.

I know, I know. This comes with an absurd amount of gotchas but I had to start somewhere.

On non-switch platforms these just fall-back to the original functions, returning the same pointer that they were passed in. Hopefully nothing broke.

With the help of some hacky macros, I updated all arm64 code I could find to this new JIT API and hoped for the best.

On paper this should work, the only real issue could be absolute jumps. In practice it didn't. My initial implementation for the `new_codechunk` function was creating a new JIT area of the requested size every time, this worked fine for the first 10 or so allocations then it would fail with error code `0xce01` indicating resource exhaustion. It seems that the switch kernel will only allocate a set number of JIT code regions.

Not all hope was lost though, mono's code manager already accounts for the possibility of "reserving" memory by doing one big memory allocation and filling it up with JITed code. Initially I did not implement this for reasons I'll explain in a bit but seeing this was the only way forward I changed my mind.

And finally, after much pain and anguish we have real mono running on a real switch 🎉

https://github.com/user-attachments/assets/b0d8e9e0-8925-47f9-9039-4b2c9f04b2aa

>[!Note]
> Launching a dll from the hbmenu is something I added much later, it's here only for dramatic effect.

So why did I try to avoid reserving memory? It was because I was worried it could come back to bite me once multi-threading is set up. The mono docs at `docs/design/mono/web/thread-safety.md` claim that code generation is protected by a mutex, my understanding is that `mono_jit_lock` is used only when code is being emitted but not when such code is being run.

This means that if we have JITed code running that somehow invokes more code generation there's the possibility that the code manager will pick the same memory area that is currently executing and switch its permissions to RW, causing an instruction abort. This normally is not an issue on other platforms because mono assumes RWX memory.

I don't believe this can happen in single thread scenarios because as long as the code in the JIT area is not actively running (eg, it's just in a stack frame) it won't be an issue. But if it's currently running on a different thread that will most definitely crash.

Just for the sake of experimenting, since I already had a libnx code-manager implementation I wanted to try running full JIT  to see how far it could go. As expected it fails pretty early due to the implementation confusing the rw and rx pointers in jump tables. P/Invoke stubs work fine only because they're extremely simple by comparison.

I also decided to dig in both [libnx's JIT implementation](https://github.com/switchbrew/libnx/blob/master/nx/source/kernel/jit.c) and the [open source kernel implementation](https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/source/svc/kern_svc_code_memory.cpp) to ensure I was not missing an easy way out. In retrospect, I should have done this earlier as it gave me two key findings:

1. The default backend on modern firmware is `JitType_CodeMemory`, which unlike what the API might suggest is always mapped both in the RX and RW views, we see this in the implementation for the [function that switches permissions](https://github.com/switchbrew/libnx/blob/60bf943ec14b1fb2ae169e627e64ab93a24c042b/nx/source/kernel/jit.c#L110). While this is not real RWX it solves the multi threading issue I mentioned earlier. Knowing this I rewrote the codeman implementation to clean it up a bit.
2. By experimenting with the raw `svcControlCodeMemory` syscall manually I figured out that it is indeed possible to have a single memory view and manually switch it protections between RW and RX by means of unmapping and then mapping it again at the same address. This however brings back the threading issue.

TLDR; To get full JIT support either I do substantial changes to mono to make it support the rw/rx view concept or use the unmap-remap trick which requires figuring out how to suspend all the managed threads during codegen, possibly by abusing the GC lock. None of these sound especially interesting to me so this is where I stopped with JIT support. After all, the interpreter is more than enough for a demo and I still wanted to look into AOT builds.

Note that there are likely more blockers for real JIT support, I can think of the following:

- Just like CoreCLR there's hardware acceleration for exceptions. A C# `NullReferenceException` will trigger a real null pointer dereference in JITed code which will be handled via signals. It seems this behavior can be disabled with the build option `MONO_ARCH_DISABLE_HW_TRAPS` but I did not investigate.
- Given the 10 `CodeRegion` resource limit, there is still a limit to the amount of JIT regions that can be allocated. Certain .NET features like dynamic methods will cause mono to create a new code manager instance and will not share the memory regions allocated by other instances. This might not be an issue for `svcControlCodeMemory`.

## Reaching out to the internet

The last goal for the demo was getting network access to work. My initial test was trying to use an `HttpClient` but that had too many failure points so I started simple: first got an UDP socket to work, then a TCP socket and finally the `HttpClient`.

The first exception looked like this

```
---- DEBUG ASSERTION FAILED ----
---- Assert Short Message ----
InternalException thrown for unexpected value: ENOSYS
---- Assert Long Message ----
   at System.Diagnostics.DebugProvider.Fail(String message, String detailMessage)
   at System.Diagnostics.Debug.Fail(String message, String detailMessage)
   at System.Diagnostics.Debug.Fail(String message)
   at System.Net.InternalException..ctor(Object unexpectedValue)
   at System.Net.Sockets.SocketAsyncEngine..ctor()
   at System.Net.Sockets.SocketAsyncEngine.CreateEngines()
   at System.Net.Sockets.SocketAsyncEngine..cctor()
   at System.Net.Sockets.SafeSocketHandle..ctor()
   at System.Net.Sockets.SocketPal.CreateSocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType, SafeSocketHandle& socket)
   at System.Net.Sockets.Socket..ctor(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
   at System.Net.Sockets.Socket..ctor(SocketType socketType, ProtocolType protocolType)
   ...
```

You might be wondering what is a `SocketAsyncEngine`. After some digging I found out it's the low level class that implements async networking on posix platforms. This class is spawned for each physical core, with some scaling factor, and it implements a busy loop into an `epoll` syscall.

When a socket instance is created it registers itself with this class so the caller can be notified asynchronously when there's an event to be handled.

The error is coming from `pal_networking.c`. Dotnet currently has two backends for async networking: `epoll` and `kqueue`, when none of these are available the default implementation for `SystemNative_CreateSocketEventPort` and related functions is to always return `ENOSYS`.

Since we only have `poll` on switch I started thinking how to implement the `SocketEventPort` API in terms of it, this is not easy in C given the thread-safety and multiple instances requirements. But it turns out there was no need to, a much simpler approach was to write my own switch-specific `SocketAsyncEngine`. It lives in the `SocketAsyncEngine.Libnx.cs` file and implements the same public API as the posix one but calling `poll` instead. C# has solid concurrency primitives which made it rather easy to write.

The native PAL already contains a well-defined `poll` wrapper as `Interop.Sys.Poll`, I only had to implement the handle management logic. With just a few attempts we had sockets working on switch.

Something I should mention is that none of this worked in the emulator, in fact it completely aborts when trying to send data. The reason is that the network PAL uses `sendmsg`/`recvmsg` which appear to not be implemented at all. This means that networking will only ever work on a real console.

Without further ado, here's mono on switch sending an HTTP request to google.com

![](https://github.com/user-attachments/assets/d89d6143-0f54-4897-81f8-3a8802674fa9)

Getting the `HttpClient` class to work required some additional fiddling with `System.Security`, it works but TLS is not supported because that depends on native platform APIs, for linux that is openssl.

Libnx does have support for the [HOS's ssl service](https://github.com/switchbrew/libnx/blob/master/nx/include/switch/services/ssl.h) but I did not try to implement it.

## The smoke test

I wanted to see how far would this hacked-up mono go with some real third-party libraries so I put together a simple demo using [SDL2_net](https://github.com/libsdl-org/SDL_net) and [Imgui.NET](https://github.com/ImGuiNET/ImGui.NET) that shows a proper UI, to my surprise everything just worked without a hitch.

![](https://github.com/user-attachments/assets/5d270e09-bc5c-4423-869f-dccfc0ff45b5)

While P/Invoke still needs all the native dependencies statically linked, this is a C# program calling into SDL2 and Imgui, definitely surpassed all my expectations.

Running a more complex piece of code revealed a few issues I missed:
- File paths on libnx are rooted with the name of the newlib device they refer to, the most common are `sdmc:/` and `romfs:/`. Most C functions like `readlink` and `getcwd` will return paths in this format. Unfortunately, this breaks all posix code around paths. For C# I implemented support into the `Path` class and file access now works as intended. Native mono code such as the dll loader will fail when one such path is passed, my workaround for this is using `/` rooted paths which refer to the device of the current working directory but you won't be able to mix loading files from the sd card and the romfs.
- While playing around with async sockets I had mono throw a fatal error after it tried to call `pthread_kill`, which is not supported on switch. I tracked this down to `mono_thread_info_resume` and related functions. My workaround here was switching thread mode to cooperative, this comes with a [set of implications](https://github.com/dotnet/runtime/blob/main/docs/design/mono/mono-thread-state-machine.md#cooperative-suspend) but I don't think this affects us much.

As for issues I couldn't figure out, there is something that corrupts the state of the homebrew process and subsequent homebrew ran in the same process will crash. This is an issue specific to switch homebrew due to the legacy of how nro files are loaded by [nx-hbloader](https://github.com/switchbrew/nx-hbloader): typically they all share the same process as far as the OS can see. So if we have some kernel resource leak our homebrew might exit fine but the next one will fail in a way that is hard to debug. 

In this specific case I've noticed that the next homebrew in line will likely crash when writing to memory returned by malloc. Specific applications like the mono interpreter itself and [ftpd](https://github.com/mtheall/ftpd) consistently reproduce this issue while not every other homebrew I tried does.

I debugged both:
- The interpreter crashes inside mono initialization in `_malloc_r` itself.
- ftpd crashes inside of `memset` called by libnx while initializing the [`tmem`]([https://github.com/switchbrew/libnx/blob/master/nx/source/kernel/tmem.c#L25](https://github.com/switchbrew/libnx/blob/de7cfeb3d95990abfd7595d49ab8d89a11099178/nx/source/kernel/tmem.c#L25)) needed for [`socketInitializeDefault`](https://github.com/switchbrew/libnx/blob/de7cfeb3d95990abfd7595d49ab8d89a11099178/nx/source/services/bsd.c#L239).

In both cases the pointer that causes the data abort seems to be correct, as in it points close enough to the heap region. It is worth mentioning that every homebrew is statically linked with its own allocator so as far as I know heap corruption shouldn't pass between apps. 

While mono has had reports of problems with the deinit code [here](https://github.com/mono/mono/issues/20191) and [here](https://stackoverflow.com/questions/10651230/multiple-mono-jit-init-mono-jit-cleanup-issue), I can't say if this is the exact cause. My guess would be something related to memory permissions but from the logs of my JIT code nothing ever touches that the affected region. I tried to reproduce this by replicating the API calls I suspected without mono in the loop but it was to no avail.

In the end I decided to add yet another workaround and always terminate the process when the interpreter finishes running, this should avoid hitting seemingly unexplainable crashes.

## Going ahead (of time)

The last thing I wanted to try was getting AOT working, By now most of my goals had been achieved so I only did limited testing since I didn't see the point of spending too much time on this.

If you thought that all we did until now was barely documented, wait to see the non-existent docs for statically linking mono AOT binaries. All The main references are [aot design document](https://github.com/dotnet/runtime/blob/main/docs/design/mono/aot.md), the [mono man page](https://github.com/dotnet/runtime/blob/main/docs/design/mono/mono-manpage-1.md#runtime-options) and some [CI workflows](https://github.com/dotnet/runtime/blob/c97d3a415dfcf6cbf099a31747dad98ce7c13279/eng/pipelines/runtime.yml#L1203). Don't fall for the outdated [mono project website](https://www.mono-project.com/docs/advanced/aot/) or [mailing list](https://mono-list.ximian.narkive.com/yhMLfquS/aot-compile-a-static-library).

Since the runtime and BCL are already ported getting this to work required minimal code changes, most of it was project files, the `CMakeLists.txt` and adding the libnx build configuration to `offsets-tool.py`.

In fact, the build system here gave me lots of headaches. We need to build linux-x64 mono with the libnx-arm64 cross compiler configuration. There are two steps involved: first build a limited libnx-arm64 version which produces a header file containing the target ABI information, then build the native linux compiler with that header file.

It's this way because the x64 compiler doesn't know the arm64 ABI so the build system runs `offsets-tool.py` that uses libclang to "simulate" a mono build for the target and extract all the binary offsets for the various structures.

Here's kicker though, if I add my libnx target among the other AOT targets cmake fails trying to build a linux program with the libnx toolchain.

```
<BuildMonoAOTCrossCompiler Condition="'$(TargetsBrowser)' == 'true'">true</BuildMonoAOTCrossCompiler>
<BuildMonoAOTCrossCompiler Condition="'$(TargetsAndroid)' == 'true'">true</BuildMonoAOTCrossCompiler>
<BuildMonoAOTCrossCompiler Condition="'$(TargetsWasi)' == 'true'">true</BuildMonoAOTCrossCompiler>
<!-- This is broken -->
<!-- <BuildMonoAOTCrossCompiler Condition="'$(TargetsLibnx)' == 'true'">true</BuildMonoAOTCrossCompiler> -->
```

I still don't know why this happens, there is probably something I'm missing in the toolchain file or the project file.

Instead, by reading through the CI build scripts I found that it's possible to run these two steps individually and this works fine. I don't know if this is the correct way but it works for me.

First I build the offset file using a terminal where with the `ROOTFS_DIR` environment variable set. This will use the switch toolchain.

```
./build -s mono.aotcross /p:MonoGenerateOffsetsOSGroups=libnx
```

Then, I undefine `ROOTFS_DIR` and build the linux version of mono. This will use the host toolchain.

```
./build.sh -s mono /p:AotHostArchitecture=x64 /p:AotHostOS=linux /p:MonoCrossAOTTargetOS=libnx /p:SkipMonoCrossJitConfigure=true /p:BuildMonoAOTCrossCompilerOnly=true
```

But then this wouldn't build the cross-compiler. So as a final workaround I conditionally enabled the cross compiler when the `BuildMonoAOTCrossCompilerOnly` flag is present. This is not pretty but it works.

```
<BuildMonoAOTCrossCompiler Condition="'$(TargetsLibnx)' == 'true' and '$(BuildMonoAOTCrossCompilerOnly)' == 'true'">true</BuildMonoAOTCrossCompiler>
```

We finally have the cross compiler compiler binary in `./artifacts/bin/mono/libnx.arm64.Debug/cross/libnx-arm64/mono-aot-cross`

Assuming all the framework dll files are in a folder called `framework`, using the cross compiler is self-explanatory: `./mono-aot-cross --path=framework/ --aot=full,static,tool-prefix=aarch64-none-elf- my_assembly.dll` produces a file called `my_assembly.o`. This object file has several exports where the most important one is called something like `mono_aot_module_MyAssemblyNamespace_info` which needs to be registered with `mono_aot_register_module` in the wrapper application.

But this is not enough, mono AOT libraries only contain the code of the assembly it was compiled from, all dependencies must be compiled separately.

At first I tried to build the whole framework folder but that was taking too long and decided to find a better way.

To discover the dependencies of my test program I used the very convenient `copyused` option of [illink](https://github.com/dotnet/runtime/blob/main/docs/tools/illink/illink-options.md) which copies all the framework dependencies of an assembly and the assembly itself to the output folder, at this point it's just a matter of running `mono-aot-cross` on all of them.

Or is it? Turns out some libraries will fail to link.
![](https://github.com/user-attachments/assets/ea4ad7dd-ec28-4983-9f26-3231750611ad)

If you had never seen this error message before, you're not alone. It means the linker failed to relocate some symbols because all the libraries combined use so much program address space that the distance between a jump and it's target is bigger than the maximum distance addressable with a jump instruction.

I'm not sure if the reason for this is that there really is that much code or some data tables are taking away precious space, either way this is not gonna fly, we need trimming.

The main purpose of Illink is not copying dependencies around but actually trimming assemblies to remove unused classes or individual class members. I didn't do this right away because I didn't know if it had any side effects nor how to debug them but apparently it's the only way.

The configuration that ended up working for me is
```
dotnet artifacts/bin/Mono.Linker/Debug/net9.0/illink.dll \
    -x src/mono/System.Private.CoreLib/src/ILLink/ILLink.Descriptors.xml \
    -x src/mono/System.Private.CoreLib/src/ILLink/ILLink.LinkAttributes.xml \
    --feature System.Resources.UseSystemResourceKeys true \
    -d framework/ --trim-mode link \
    -a my_assembly.dll
```

The arguments passed as `-x` are definitions files that will tell illink what methods or classes must not be removed, these are critical otherwise illink will trim away core classes that mono assumes always exist such as `NullReferenceException`.

Figuring out the `--feature` option took me ages and fixes the issue I mentioned earlier about exception messages stored in the resources, but with a twist this time. More in a bit.

One `mono-aot-cross` step later and we finally have object files that link properly and actually work.

These AOT libraries only contain the methods compiled as native code, to run them mono also needs the original dll files to load the actual .NET metadata. Mono will also check the assembly GUIDs to ensure they are the same that were used to build the AOT code, meaning that only the illink outputs will work for this and not the original framework assemblies. 

If you're following along at home you might get hit with this error when running an AOT binary:

![](https://github.com/user-attachments/assets/158bffcd-46ab-4f46-8ce7-1ba8b492efc5)

If you read the message it sounds as if `NullReferenceException` was trimmed, but checking with ILSpy it was clearly there. By tracing the code flow form the assert error I added some logging to `mono_runtime_try_invoke` which produced a sort of stack-trace composed of the `Invoking method` logs. From there I saw exceptions being thrown recursively and the `SR` class and _guessed_ this had to be the same `UseSystemResourceKeys` issue as before. This time it's definitely Illink's fault, with some searching around the xml configuration files I found the `--feature System.Resources.UseSystemResourceKeys` option.

>[!Note]
> If I hadn't got the interpreter working first, I'd have never figured this one out.

One last gotcha is binary size. The AOT demo in my repo bundles all the needed files in a single nro file, that is great, except for the fact that the final hello world is 60MB. Half of that is used up by the icu data file.

## Closing thoughts

_Whew_, this was an insanely long write-up for a project that was just as insane. 

As I already mentioned in the repo readme this was just an experiment and I'm not going to maintain it as an actual mono port. You probably shouldn't use to make real homebrew applications.

If you're wondering how long all of this took it's a little hard to say, I worked on it for about three months mostly during weekends but took a few longer breaks when faced with blocking issues. If I had to give an estimate in work-days I'd say the initial bring up took around one week, it felt like most of it was spent fighting the build system. Getting basic BCL support was relatively fast and figuring out AOT took a couple more days. Including cleaning up the code for release and writing this post it's probably around three weeks in total.

At this point what's next? Could we make this a real supported platform? _Probably not_, my implementation is full of hacks and things I still don't fully understand. Going this route the first goal would be to find a way to run the official test suite and use that as a guide to figure out what needs fixing.

While bringing support and passing tests for BCL classes like `Console` shouldn't be too hard there are few bigger issues like JIT support needed for native performances, a openssl port needed for `System.Net.Security`, P/Invoke which will never truly work and there's probably much more. 

In my opinion this is too much work for a hobby project, I guess this is the reason we haven't seen a mono port until now.

Thank you for reading until the end. I hope you enjoyed it and that I could inspire you to try porting mono to your favorite weird platform.
