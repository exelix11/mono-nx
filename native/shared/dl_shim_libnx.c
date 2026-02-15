#include "dl_shim_base.h"
#include <switch.h>

u32 extensionPadStateSize() 
{
    return sizeof(PadState);
}

void *getsym_Libnx(const char *name)
{
    SYM_RESOLVE_EXISTING(padConfigureInput);
    SYM_RESOLVE_EXISTING(padUpdate);
    SYM_RESOLVE_EXISTING(padInitializeWithMask);
    SYM_RESOLVE_EXISTING(extensionPadStateSize);
    
    SYM_RESOLVE_EXISTING(appletMainLoop);

    // Used by opentk
    SYM_RESOLVE_EXISTING(nwindowGetDefault);
    return NULL;
}