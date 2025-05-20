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

#define SYM_RESOLVE_EXISTING_NAMED(SYMNAME, FUNCTION)  \
	do                                           \
	{                                            \
		if (name && strcmp(name, SYMNAME) == 0) \
		{                                        \
			return (void *)FUNCTION;             \
		}                                        \
	} while (0)

#define SYM_RESOLVE_EXISTING(SYMNAME) SYM_RESOLVE_EXISTING_NAMED(#SYMNAME, SYMNAME)