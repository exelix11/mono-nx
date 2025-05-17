#include "core.h"

int main(int argc, char *argv[])
{
    if (!application_initialize())
        return 1;

    MonoDomain *domain = NULL;

    mono_jit_set_aot_mode(MONO_AOT_MODE_INTERP_ONLY);

    if (g_config.mono_logging)
    {
        mono_trace_set_log_handler(on_mono_log, NULL);
        mono_trace_set_mask_string("all");
        mono_trace_set_level_string("debug");
    }

    mono_dl_fallback_register(MonoDlFallback_Load, MonoDlFallback_Symbol, MonoDlFallback_Close, NULL);
    mono_install_unhandled_exception_hook(Mono_unhandledExceptionHook, NULL);

    domain = mono_jit_init("embedded_mono");
    if (!domain)
    {
        fatal_error("Failed to initialize mono domain\n");
        return 1;
    }

    char *launch_dll = io_strdup(argc > 1 ? argv[1] : g_config.default_assembly);
    if (!launch_dll)
    {
        fatal_error("No .dll was specified");
        return 1;
    }

    io_debugf("Loading assembly %s", launch_dll);

    MonoAssembly *assembly = mono_domain_assembly_open(domain, launch_dll);
    if (!assembly)
    {
        fatal_error("Failed to load assembly");
        return 1;
    }

    char *monoargs[] = {launch_dll};

    mono_jit_exec(domain, assembly, 1, monoargs);

    mono_jit_cleanup(domain);

    application_terminate();

    return 0;
}
