# Building the mono AOT compiler

To build AOT homebrew you will first need to build all the mono static libraries and the cross compiler on your machine.

Once you have all the requirements ready AOT builds boil down to the following steps:

1) Build the regular C# project
2) Use Illink to trim the assemblies
3) Compile the trimmed assemblies with mono-aot
4) Statically link the compiled code with the example program. Here you will need to manually register the assemblies with the `STATIC_MONO_SYM` in `main.c`.

Note that while this does compile the IL code to native code the object files will only contain the code, mono will still need the original dll files for the rest of the metadata. Currently they're stored in the romfs. 

With this process even a simple hello world produces a huge nro of around 60MB. Half of that is caused by the icu data file in the romfs, the framework dlls take around 4MB and the rest is code.

It is possible to save a lot of memory by stripping the icu data file and using the invariant culture, in early tests this seems to work by calling setenv(DOTNET_SYSTEM_GLOBALIZATION_INVARIANT, 1) before initializing mono. The icu build script compiles a 2MB trimmed data file but it is currently not copied automatically by the build scripts since it needs more testing.

We should also be using mono compiler's `direct-pinvoke` option to produce direct references to the System.Native functions, this would allow us to scrap the fake dynamic loader and should let the linker trim away all the unneeded code. This is currently not done because some framework libraries will pull in symbols that are not built in the switch port and fail to link, they can probably be stubbed without consequences but I did not look into it.