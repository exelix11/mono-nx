#pragma once

#include <stdio.h>
#include <string.h>

#define SYM_RESOLVE_NAMED(SYMNAME, FUNCTION)    \
	do                                          \
	{                                           \
		if (name && strcmp(name, SYMNAME) == 0) \
		{                                       \
			extern void *FUNCTION();            \
			return (void *)FUNCTION;            \
		}                                       \
	} while (0)

#define SYM_RESOLVE(SYMNAME) SYM_RESOLVE_NAMED(#SYMNAME, SYMNAME)

#define SYM_RESOLVE_EXISTING(SYMNAME)            \
	do                                           \
	{                                            \
		if (name && strcmp(name, #SYMNAME) == 0) \
		{                                        \
			return (void *)SYMNAME;              \
		}                                        \
	} while (0)
