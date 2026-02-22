#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <switch.h>

#include "io_util.h"
#include "dl_shim.h"
#include "third_party/ini/ini.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/exception.h>
#include <mono/utils/mono-logger.h>
#include <mono/utils/mono-dl-fallback.h>

#define CONFIG_INI_PATH "/mono/config.ini"

struct AppConfiguration
{
    bool mono_runtime_logging;
    bool mononx_logging;

    char *icudata_path;
    char *assembly_dir;
    char *config_dir;
    char *default_assembly;

    bool svc_io_redirect;
    char *udp_io_redirect;
    char *file_io_redirect;

    bool force_console_init;
    bool exit_process_on_end;
};

extern struct AppConfiguration g_config;

// Loads config
bool application_initialize(const char* configFile);

// Sets up dlshim and exception hooks
void application_configure_mono();

void application_terminate();

void application_chdir_to_assembly(const char* path);

void input_ensure_init();

void console_ensure_init();
void console_dispose();
void console_update();

void socket_esnure_init_thread_safe();
void socket_terminate();

void fatal_error(const char *message);