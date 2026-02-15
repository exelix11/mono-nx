#include "../dl_shim_base.h"

void *getsym_SDL2_image(const char *name)
{
	SYM_RESOLVE(IMG_Linked_Version);
	SYM_RESOLVE(IMG_Init);
	SYM_RESOLVE(IMG_Quit);
	SYM_RESOLVE(IMG_Load);
	SYM_RESOLVE(IMG_Load_RW);
	SYM_RESOLVE(IMG_LoadTyped_RW);
	SYM_RESOLVE(IMG_LoadTexture);
	SYM_RESOLVE(IMG_LoadTexture_RW);
	SYM_RESOLVE(IMG_LoadTextureTyped_RW);
	SYM_RESOLVE(IMG_ReadXPMFromArray);
	SYM_RESOLVE(IMG_SavePNG);
	SYM_RESOLVE(IMG_SavePNG_RW);
	SYM_RESOLVE(IMG_SaveJPG);
	SYM_RESOLVE(IMG_SaveJPG_RW);
	return NULL;
}