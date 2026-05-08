#include "core.h"
#include <unistd.h>

#define STATIC_MONO_SYM(x) do {\
    extern void* x; \
    mono_aot_register_module(x);\
} while(0)

int main(int argc, char *argv[])
{
    // Needed when using minimal ICU data to reduce the size of the binary.
    //setenv("DOTNET_SYSTEM_GLOBALIZATION_INVARIANT", "1", 1);
    
    romfsInit();

    // Mono doesn't like : in paths, we must cd to the romfs to trick it into loading the dll files from it
    chdir("romfs:/");

    if (!application_initialize("romfs:/aot_config.ini"))
        return 1;

    MonoDomain *domain = NULL;
    
    // Output from build_aot.sh
    STATIC_MONO_SYM(mono_aot_module_System_Collections_Concurrent_info);
    STATIC_MONO_SYM(mono_aot_module_System_Console_info);
    STATIC_MONO_SYM(mono_aot_module_System_Diagnostics_DiagnosticSource_info);
    STATIC_MONO_SYM(mono_aot_module_System_Private_CoreLib_info);
    STATIC_MONO_SYM(mono_aot_module_System_Runtime_Serialization_Formatters_info);
    STATIC_MONO_SYM(mono_aot_module_System_Runtime_info);
    STATIC_MONO_SYM(mono_aot_module_System_Threading_Thread_info);
    STATIC_MONO_SYM(mono_aot_module_program_info);

    mono_jit_set_aot_mode(MONO_AOT_MODE_FULL);

    application_configure_mono();

    domain = mono_jit_init("embedded_mono");
    if (!domain)
    {
        fatal_error("Failed to initialize mono domain\n");
        return 1;
    }

    io_debugf("Loading assembly %s", g_config.default_assembly);

    MonoAssembly *assembly = mono_domain_assembly_open(domain, g_config.default_assembly);
    if (!assembly)
    {
        fatal_error("Failed to load assembly");
        return 1;
    }

    char *monoargs[] = {g_config.default_assembly};

    mono_jit_exec(domain, assembly, 1, monoargs);

    mono_jit_cleanup(domain);

    romfsExit();
    
    application_terminate();

    return 0;
}
