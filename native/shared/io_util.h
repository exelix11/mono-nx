#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

bool io_load_file(const char* path, uint8_t** out_data, size_t* out_size);

int io_init_libicu(const char* icudata_path, bool log);

bool io_has_stdio_redirection();

int io_stdio_to_svc();

int io_stdio_to_udp(const char* host, int port);

int io_stdio_to_file(const char* filename);

void io_debugf(const char *fmt, ...);

char* io_strdup(const char* str);