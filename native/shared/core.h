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
    bool mono_logging;
    char *icudata_path;
    char *assembly_dir;
    char *config_dir;
    char *default_assembly;

    bool svc_io_redirect;
    char *udp_io_redirect;
    char *file_io_redirect;

    bool force_console_init;
};

extern struct AppConfiguration g_config;

bool application_initialize();
void application_terminate();

void input_ensure_init();
void console_ensure_init();
void console_dispose();
void console_update();

void fatal_error(const char *message);

void on_mono_log(const char *log_domain, const char *log_level, const char *message, mono_bool fatal, void *user_data);
void *MonoDlFallback_Load(const char *name, int flags, char **err, void *user_data);
void *MonoDlFallback_Symbol(void *handle, const char *name, char **err, void *user_data);
void *MonoDlFallback_Close(void *handle, void *user_data);
void Mono_unhandledExceptionHook(MonoObject *exc, void *user_data);