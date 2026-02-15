#pragma once

#include <stdint.h>

void* dlshim_loadLibrary(const char *name, int flags, char **err, void *user_data);
void* dlshim_closeLibrary(void *handle, void *user_data);
void* dlshim_getSymbol(void *handle, const char *name, char **err, void *user_data);