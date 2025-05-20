#include "core.h"
#include <unistd.h>
#include <pthread.h>

static bool logging = false;

static PadState pad;
static bool using_console = false;

static pthread_mutex_t sockets_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool using_sockets = false;

void socket_esnure_init_thread_safe()
{    
    if (using_sockets) 
        return;

    pthread_mutex_lock(&sockets_mutex);
    if (using_sockets) {
        pthread_mutex_unlock(&sockets_mutex);
        return;
    }

    using_sockets = true;

    io_debugf("Initializing sockets");
    Result rc = socketInitializeDefault();
    if (R_FAILED(rc))
    {
        using_sockets = false;
        pthread_mutex_unlock(&sockets_mutex);
        io_debugf("Failed to init socketing: %08X", rc);
        fatal_error("failed to init socketing");
        return;
    }

    pthread_mutex_unlock(&sockets_mutex);
}

void socket_terminate()
{
    if (using_sockets)
    {
        io_debugf("Closing sockets");
        socketExit();
        using_sockets = false;
    }
}

struct AppConfiguration g_config;

void input_ensure_init()
{
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
}

void console_ensure_init()
{
    if (io_has_stdio_redirection())
        return;

    if (using_console)
        return;

    using_console = true;
    consoleInit(NULL);
}

void console_dispose()
{
    if (using_console)
        consoleExit(NULL);

    using_console = false;
}

void console_update()
{
    if (using_console)
        consoleUpdate(NULL);
}

void fatal_error(const char *message)
{
    // If the guest app was using SDL console init will fail and vice versa.
    // In case of a fatal error we might get stuck on a black screen.
    console_ensure_init();

    if (message)
        io_debugf("%s", message);

    io_debugf("Press + to exit");

    input_ensure_init();

    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & (HidNpadButton_Plus | HidNpadButton_Minus))
            break;

        if (using_console)
            consoleUpdate(NULL);
        else
            svcSleepThread(1000000);
    }

    svcExitProcess();
}

void on_mono_log(const char *log_domain, const char *log_level, const char *message, mono_bool fatal, void *user_data)
{
    if (logging)
        io_debugf("%s %s %s", log_domain, log_level, message);

    if (fatal)
    {
        io_debugf("Fatal error in Mono");
        fatal_error(message);
    }
}

void *MonoDlFallback_Load(const char *name, int flags, char **err, void *user_data)
{
    if (!name)
        return (void *)LibHandle_Internal;

    if (strcmp(name, "__Internal") == 0)
        return (void *)LibHandle_Internal;

    if (strcmp(name, "libSystem.Native") == 0)
        return (void *)LibHandle_SystemNative;

    if (strcmp(name, "libSystem.Globalization.Native") == 0)
        return (void *)LibHandle_GlobalizationNative;

    if (strcmp(name, "cimgui") == 0)
        return (void *)LibHandle_cimgui;

    if (strcmp(name, "SDL2") == 0)
        return (void *)LibHandle_sdl2;

    if (strcmp(name, "SDL2_image") == 0)
        return (void *)LibHandle_SDL2_image;

    if (strcmp(name, "libnx") == 0)
        return (void *)LibHandle_libnx;

    if (logging)
        io_debugf("MonoDlFallback_Load %s library=%s", "unknown library", name);

    return NULL;
}

void *MonoDlFallback_Symbol(void *handle, const char *name, char **err, void *user_data)
{
    void *symbol = NULL;
    if ((intptr_t)handle == LibHandle_SystemNative)
        symbol = getsym_SystemNative(name);
    else if ((intptr_t)handle == LibHandle_GlobalizationNative)
        symbol = getsym_GlobalizationNative(name);
    else if ((intptr_t)handle == LibHandle_cimgui)
        symbol = getsym_cimgui(name);
    else if ((intptr_t)handle == LibHandle_sdl2)
        symbol = getsym_sdl2(name);
    else if ((intptr_t)handle == LibHandle_SDL2_image)
        symbol = getsym_SDL2_image(name);
    else if ((intptr_t)handle == LibHandle_libnx)
        symbol = getsym_libnx(name);
    else if ((intptr_t)handle == LibHandle_Internal)
    {
        // Custom extensions
        if (strcmp(name, "console_ensure_init") == 0)
            symbol = (void *)console_ensure_init;
        else if (strcmp(name, "console_dispose") == 0)
            symbol = (void *)console_dispose;
        else if (strcmp(name, "console_update") == 0)
            symbol = (void *)console_update;
    }
    else
    {
        if (logging)
            io_debugf("MonoDlFallback_Symbol %s handle=%p symbol=%s", "invalid handle", handle, name);

        return NULL;
    }

    if (symbol)
        return symbol;

    if (logging)
        io_debugf("MonoDlFallback_Symbol %s handle=%p symbol=%s", "invalid symbol", handle, name);

    return NULL;
}

void *MonoDlFallback_Close(void *handle, void *user_data)
{
    return NULL;
}

void Mono_unhandledExceptionHook(MonoObject *exc, void *user_data)
{
    io_debugf("--- Unhandled exception ---");
    MonoString *exc_str = mono_object_to_string(exc, NULL);
    char *exc_cstr = mono_string_to_utf8(exc_str);    
    
    fatal_error(exc_cstr);
    
    mono_free(exc_cstr);
}

static char *inf_dup_unquote(const char *input)
{
    char* duplicate = io_strdup(input);    
    if (!duplicate)
    return NULL;
    
    char *s = duplicate;
    int l = strlen(s);
    if (l && s[0] == s[l - 1] && (s[0] == '\'' || s[0] == '"'))
    {
        s[l - 1] = '\0';
        s++;
    }

    // Return a string that is safe to free
    char* res = io_strdup(s);
    free(duplicate);

    return res;
}

static int handle_ini_line(void *user, const char *section, const char *name, const char *value)
{
    struct AppConfiguration *pconfig = (struct AppConfiguration *)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("mono", "logging"))
        pconfig->mono_logging = (strcmp(value, "true") == 0);
    else if (MATCH("mono", "icu"))
        pconfig->icudata_path = inf_dup_unquote(value);
    else if (MATCH("mono", "assembly_dir"))
        pconfig->assembly_dir = inf_dup_unquote(value);
    else if (MATCH("mono", "config_dir"))
        pconfig->config_dir = inf_dup_unquote(value);
    else if (MATCH("mono", "default_assembly"))
        pconfig->default_assembly = inf_dup_unquote(value);
    else if (MATCH("nx", "svc_io_redirect"))
        pconfig->svc_io_redirect = (strcmp(value, "true") == 0);
    else if (MATCH("nx", "udp_io_redirect"))
        pconfig->udp_io_redirect = inf_dup_unquote(value);
    else if (MATCH("nx", "file_io_redirect"))
        pconfig->file_io_redirect = inf_dup_unquote(value);
    else if (MATCH("nx", "force_console_init"))
        pconfig->force_console_init = (strcmp(value, "true") == 0);
    else if (MATCH("nx", "exit_process_on_end"))
        pconfig->exit_process_on_end = (strcmp(value, "true") == 0);
    else
    {
        return 0; /* unknown section/name, error */
    }

#undef MATCH

    return 1;
}

bool application_initialize(const char* configFile)
{
    memset(&g_config, 0, sizeof(struct AppConfiguration));

    if (ini_parse(configFile, handle_ini_line, &g_config) < 0)
    {
        io_debugf("Can't load app config from %s", configFile);
        fatal_error("Can't load app config");
        return false;
    }

    logging = g_config.mono_logging;

    if (g_config.force_console_init)
        console_ensure_init();
    
    // It's fine if this fails, the runtime has fallbacks for it.
    csrngInitialize();

    if (g_config.file_io_redirect)
    {
        if (io_stdio_to_file(g_config.file_io_redirect) < 0)
        {
            fatal_error("Failed to redirect stdio to file");
            return false;
        }
    }
    else if (g_config.udp_io_redirect)
    {
        socket_esnure_init_thread_safe();
        if (io_stdio_to_udp(g_config.udp_io_redirect, 9999) < 0)
        {
            fatal_error("Failed to redirect stdio to udp");
            return false;
        }
    }
    else if (g_config.svc_io_redirect)
    {
        if (io_stdio_to_svc() < 0)
        {
            fatal_error("Failed to redirect stdio to svc");
            return false;
        }
    }

    if (!g_config.config_dir || !g_config.assembly_dir || !g_config.icudata_path)
    {
        fatal_error("Some paths are missing from the config file");
        return false;
    }

    if (!io_init_libicu(g_config.icudata_path, g_config.mono_logging))
    {
        fatal_error("Libicu init failed");
        return false;
    }

    mono_set_dirs(g_config.assembly_dir, g_config.config_dir);   

    return true;
}

// Internal libnx symbol
u32 __nx_applet_exit_mode = 0;

void application_terminate()
{
    io_debugf("Terminating application");

    // This symbol is defined in mono and is used as a workaround to force release all the kernel JIT objects
    extern void nx_jit_force_dispose(void);

    nx_jit_force_dispose();
    
    csrngExit();
    
    io_stdio_finish();

    socket_terminate();

    console_dispose();

    io_dispose_libicu();

    if (g_config.icudata_path) free(g_config.icudata_path);
    if (g_config.assembly_dir) free(g_config.assembly_dir);
    if (g_config.config_dir) free(g_config.config_dir);
    if (g_config.default_assembly) free(g_config.default_assembly);
    if (g_config.udp_io_redirect) free(g_config.udp_io_redirect);
    if (g_config.file_io_redirect) free(g_config.file_io_redirect);

    if (g_config.exit_process_on_end) 
    {
        // This forces libnx to always use applet exit + svcExitProcess
        __nx_applet_exit_mode = 1;
        // Terminate cleanly-enough
        exit(0);
    }
}

void application_chdir_to_assembly(const char* path)
{
    int dirlen = strlen(path);
    if (dirlen < 2)
        return;
    
    char* dir = io_strdup(path);

    bool found = false;
    for (int i = dirlen - 1; i >= 0; i--)
    {
        if (dir[i] == '/')
        {
            // Avoid doing "/test" -> ""
            if (i == 0)
                dir[1] = '\0';
            else 
                dir[i] = '\0';
            
            found = true;
            break;
        }
    }

    if (found)
    {    
        io_debugf("chdir(%s)", dir);
        chdir(dir);
    }

    free(dir);
}
