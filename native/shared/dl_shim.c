#include "core.h"
#include "dl_shim.h"

#define REGISTER_LIBRARY(name, string, id) \
	const intptr_t LibHandle_##name = (intptr_t)(0xABC00000 | id); \
	const char* LibName_##name = string; \
	void* getsym_##name(const char *name);

#define CHECK_LIB_NAME(value, name) \
do { \
	if (strcmp(value, LibName_##name) == 0) \
		return (void *)LibHandle_##name; \
} while (0)

// Core libraries
REGISTER_LIBRARY(Libnx, "libnx", 0x01)
REGISTER_LIBRARY(Internal, "__Internal", 0x02)
REGISTER_LIBRARY(SystemNative, "libSystem.Native", 0x03)
REGISTER_LIBRARY(GlobalizationNative, "libSystem.Globalization.Native", 0x04)

// Optional libraries
#if defined(DLSHIM_SDL2)
REGISTER_LIBRARY(SDL2, "SDL2", 0x100)
#endif

#if defined(DLSHIM_SDL2_IMAGE)
REGISTER_LIBRARY(SDL2_image, "SDL2_image", 0x101)
#endif

#if defined(DLSHIM_CIMGUI)
REGISTER_LIBRARY(Cimgui, "cimgui", 0x102)
#endif

void *dlshim_loadLibrary(const char *name, int flags, char **err, void *user_data)
{
    if (!name)
        return (void *)LibHandle_Internal;

    CHECK_LIB_NAME(name, Libnx);
    CHECK_LIB_NAME(name, Internal);
    CHECK_LIB_NAME(name, SystemNative);
    CHECK_LIB_NAME(name, GlobalizationNative);

    #if defined(DLSHIM_SDL2)
	CHECK_LIB_NAME(name, SDL2);
	#endif

	#if defined(DLSHIM_SDL2_IMAGE)
	CHECK_LIB_NAME(name, SDL2_image);
	#endif

	#if defined(DLSHIM_CIMGUI)
	CHECK_LIB_NAME(name, Cimgui);
	#endif	

	if (g_config.mono_logging)
    	io_debugf("dlshim_loadLibrary %s library=%s", "unknown library", name);

    return NULL;
}

void *dlshim_closeLibrary(void *handle, void *user_data)
{
    return NULL;
}

void *dlshim_getSymbol(void *handle, const char *name, char **err, void *user_data)
{
    void *symbol = NULL;
	const char* resolvedLibrary = "<none>";

#define CHECK_LIB_SYMBOL(libName) \
	case LibHandle_##libName: \
		resolvedLibrary = LibName_##libName; \
		symbol = getsym_##libName(name); 

	if (!handle)
		return NULL;

	switch ((intptr_t)handle)
	{		
		CHECK_LIB_SYMBOL(Libnx)
		CHECK_LIB_SYMBOL(Internal)
		CHECK_LIB_SYMBOL(SystemNative)
		CHECK_LIB_SYMBOL(GlobalizationNative)

	#if defined(DLSHIM_SDL2)
		CHECK_LIB_SYMBOL(SDL2)
	#endif

	#if defined(DLSHIM_SDL2_IMAGE)
		CHECK_LIB_SYMBOL(SDL2_image)
	#endif

	#if defined(DLSHIM_CIMGUI)
		CHECK_LIB_SYMBOL(Cimgui)
	#endif
	}

    if (symbol)
        return symbol;

    if (g_config.mono_logging)
        io_debugf("dlshim_getSymbol error: handle=%p lib=%s symbol=%s", handle, resolvedLibrary, name);

    return NULL;
}

void* getsym_Internal(const char *name)
{
	if (strcmp(name, "console_ensure_init") == 0) return (void *)console_ensure_init;
    else if (strcmp(name, "console_dispose") == 0) return (void *)console_dispose;
    else if (strcmp(name, "console_update") == 0) return(void *)console_update;

	return NULL;
}