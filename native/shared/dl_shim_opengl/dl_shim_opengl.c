#include "../dl_shim_base.h"

void *getsym_Glad(const char *name)
{
	SYM_RESOLVE(gladLoadGL);
	return NULL;
}

void *getsym_Egl(const char *name)
{
	SYM_RESOLVE(eglGetError);
	SYM_RESOLVE(eglGetDisplay);
	SYM_RESOLVE(eglInitialize);
	SYM_RESOLVE(eglTerminate);
	SYM_RESOLVE(eglQueryString);
	SYM_RESOLVE(eglGetConfigs);
	SYM_RESOLVE(eglChooseConfig);
	SYM_RESOLVE(eglGetConfigAttrib);
	SYM_RESOLVE(eglCreateWindowSurface);
	SYM_RESOLVE(eglCreatePbufferSurface);
	SYM_RESOLVE(eglCreatePixmapSurface);
	SYM_RESOLVE(eglDestroySurface);
	SYM_RESOLVE(eglQuerySurface);
	SYM_RESOLVE(eglBindAPI);
	SYM_RESOLVE(eglQueryAPI);
	SYM_RESOLVE(eglWaitClient);
	SYM_RESOLVE(eglReleaseThread);
	SYM_RESOLVE(eglCreatePbufferFromClientBuffer);
	SYM_RESOLVE(eglSurfaceAttrib);
	SYM_RESOLVE(eglBindTexImage);
	SYM_RESOLVE(eglReleaseTexImage);
	SYM_RESOLVE(eglSwapInterval);
	SYM_RESOLVE(eglCreateContext);
	SYM_RESOLVE(eglDestroyContext);
	SYM_RESOLVE(eglMakeCurrent);
	SYM_RESOLVE(eglGetCurrentContext);
	SYM_RESOLVE(eglGetCurrentSurface);
	SYM_RESOLVE(eglGetCurrentDisplay);
	SYM_RESOLVE(eglQueryContext);
	SYM_RESOLVE(eglWaitGL);
	SYM_RESOLVE(eglWaitNative);
	SYM_RESOLVE(eglSwapBuffers);
	SYM_RESOLVE(eglCopyBuffers);
	SYM_RESOLVE(eglGetProcAddress);
	return NULL;
}