
#ifdef __linux__

#include <dlfcn.h>

__asm__(".symver stat, __xstat@GLIBC_2.0");

int __wrap_stat(const char* _0, struct stat* _1)
{
#ifndef __x86_64__
	return __xstat(3, _0, _1);
#else
	return __xstat(1, _0, _1);
#endif
}

__asm__(".symver dlopen, dlopen@GLIBC_2.1");

void* __wrap_dlopen(const char* _0, int _1)
{
	return dlopen(_0, _1);
}

__asm__(".symver dlsym, dlsym@GLIBC_2.0");

void* __wrap_dlsym(void* _0, const char* _1)
{
	return dlsym(_0, _1);
}

__asm__(".symver dlclose, dlclose@GLIBC_2.0");

int __wrap_dlclose(void* _0)
{
	return dlclose(_0);
}

__asm__(".symver dladdr, dladdr@GLIBC_2.0");

int __wrap_dladdr(const void* _0, Dl_info* _1)
{
	return dladdr(_0, _1);
}

__asm__(".symver __isoc23_strtoull, strtoull@GLIBC_2.0");

unsigned long long int __wrap___isoc23_strtoull(const char* _0, char** _1, int _2)
{
	return strtoull(_0, _1, _2);
}

#if 0

__asm__(".symver fstat, __fxstat@GLIBC_2.0");

int __wrap_fstat(int _0, struct stat* _1)
{
#ifndef __x86_64__
    return __fxstat(3, _0, _1);
#else
    return __fxstat(1, _0, _1);
#endif
}

#endif

#endif
