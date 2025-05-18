#pragma once

#include <stdint.h>

#define LibHandle_libnx ((intptr_t)0xAAAAAA11AA)
void* getsym_libnx(const char *name);

#define LibHandle_SDL2_image ((intptr_t)0xAAAAAAAAFF)
void* getsym_SDL2_image(const char *name);

#define LibHandle_sdl2 ((intptr_t)0xAAAAAAAAEE)
void* getsym_sdl2(const char *name);

#define LibHandle_cimgui ((intptr_t)0xAAAAAAAADD)
void* getsym_cimgui(const char *name);

#define LibHandle_SystemNative ((intptr_t)0xAAAAAAAACC)
void* getsym_SystemNative(const char *name);

#define LibHandle_GlobalizationNative ((intptr_t)0xAAAAAAAABB)
void* getsym_GlobalizationNative(const char *name);

#define LibHandle_Internal ((intptr_t)0xAAAAAAAAAA)