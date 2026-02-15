#include "core.h"

int main(int argc, char *argv[])
{
    if (!application_initialize(CONFIG_INI_PATH))
        return 1;

    MonoDomain *domain = NULL;

    mono_jit_set_aot_mode(MONO_AOT_MODE_INTERP_ONLY);

    application_configure_mono();

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
    application_chdir_to_assembly(launch_dll);

    MonoAssembly *assembly = mono_domain_assembly_open(domain, launch_dll);
    if (!assembly)
    {
        fatal_error("Failed to load assembly");
        return 1;
    }

    char *monoargs[] = {launch_dll};

    mono_jit_exec(domain, assembly, 1, monoargs);

    mono_jit_cleanup(domain);

    free(launch_dll);

    application_terminate();

    return 0;
}
