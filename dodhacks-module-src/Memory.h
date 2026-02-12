
#pragma once

#include <stdlib.h>

#include <string_compat.h>
#include <vector_compat.h>

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
	const unsigned char* pSig, ::size_t SigSize,
	::size_t* pAddr = NULL,
	bool QuestionMarkAllowsZero = true) noexcept;

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
	const ::SourceHook::CVector < unsigned char > vSig,
	::size_t* pAddr = NULL,
	bool QuestionMarkAllowsZero = true) noexcept;

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
	const unsigned char* pSig, ::size_t SigSize,
	::size_t* pAddr = NULL,
	bool QuestionMarkAllowsZero = true) noexcept;

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
	const ::SourceHook::CVector < unsigned char > vSig,
	::size_t* pAddr = NULL,
	bool QuestionMarkAllowsZero = true) noexcept;

bool vectorizeSignature(const char* pSig,
	::SourceHook::CVector < unsigned char >& vSig,
	const char* pCharactersToSkip = ", \t\r\n\f\v\a\b;.:/-\\sigwnlSIGWNL\"'&$#@!^%[](){}<>`~_=+|hHjJkKmMoOpPqQrRtTuUvVyYzZ") noexcept;

bool vectorizeSignature(const ::SourceHook::String& sSig,
	::SourceHook::CVector < unsigned char >& vSig,
	const char* pCharactersToSkip = ", \t\r\n\f\v\a\b;.:/-\\sigwnlSIGWNL\"'&$#@!^%[](){}<>`~_=+|hHjJkKmMoOpPqQrRtTuUvVyYzZ") noexcept;

void displayVectorizedSignature(const ::SourceHook::CVector < unsigned char > vSig,
	bool LowerCase, int (*displayFunc) (const char*, ...)) noexcept;

void displayVectorizedSignature(const unsigned char* pSig, ::size_t SigSize,
	bool LowerCase, int (*displayFunc) (const char*, ...)) noexcept;
