
/**
 * string_compat.h & vector_compat.h require _CRT_SECURE_NO_WARNINGS
 *
 * they are needed for the software to run on older linux systems
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS
#	define _SHOULD_UNDEFINE_CRTSNW
#endif

#include "Memory.h"

#ifdef _SHOULD_UNDEFINE_CRTSNW
#	undef _CRT_SECURE_NO_WARNINGS
#	undef _SHOULD_UNDEFINE_CRTSNW
#endif

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
	const unsigned char* pSig, ::size_t SigSize,
	::size_t* pAddr,
	bool QuestionMarkAllowsZero) noexcept
{
	static ::size_t MemIter = false;
	static ::size_t SigIter = false;
	static ::size_t SigMax = false;
	static ::size_t MemPos = false;

	SigMax = SigSize - true;
	MemSize -= SigMax;

	for (MemIter = false; MemIter < MemSize; MemIter++)
	{
		for (SigIter = false; SigIter < SigSize; SigIter++)
		{
			MemPos = MemIter + SigIter;

			if (pMem[MemPos] == pSig[SigIter] || pSig[SigIter] == (unsigned char)('?') || pSig[SigIter] == (unsigned char)('*'))
			{
				if (false == QuestionMarkAllowsZero && false == pMem[MemPos] && pSig[SigIter] == (unsigned char)('?'))
				{
					break;
				}

				if (SigIter != SigMax)
				{
					continue;
				}

				if (pAddr)
				{
					*pAddr = ::size_t(pMem + MemIter);
				}

				return true;
			}

			break;
		}
	}

	if (pAddr)
	{
		*pAddr = false;
	}

	return false;
}

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
	const ::SourceHook::CVector < unsigned char > vSig,
	::size_t* pAddr,
	bool QuestionMarkAllowsZero) noexcept
{
	static ::size_t SigSize = false;
	static ::size_t MemIter = false;
	static ::size_t SigIter = false;
	static ::size_t SigMax = false;
	static ::size_t MemPos = false;
	static unsigned char SigByte = false;

	SigSize = vSig.size();
	SigMax = SigSize - true;
	MemSize -= SigMax;

	for (MemIter = false; MemIter < MemSize; MemIter++)
	{
		for (SigIter = false; SigIter < SigSize; SigIter++)
		{
			MemPos = MemIter + SigIter;
			SigByte = vSig[SigIter];

			if (pMem[MemPos] == SigByte || SigByte == (unsigned char)('?') || SigByte == (unsigned char)('*'))
			{
				if (false == QuestionMarkAllowsZero && false == pMem[MemPos] && SigByte == (unsigned char)('?'))
				{
					break;
				}

				if (SigIter != SigMax)
				{
					continue;
				}

				if (pAddr)
				{
					*pAddr = ::size_t(pMem + MemIter);
				}

				return true;
			}

			break;
		}
	}

	if (pAddr)
	{
		*pAddr = false;
	}

	return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
	const unsigned char* pSig, ::size_t SigSize,
	::size_t* pAddr,
	bool QuestionMarkAllowsZero) noexcept
{
	static unsigned char MemByte = false;
	static ::size_t MemSize = false;
	static ::size_t MemIter = false;
	static ::size_t SigIter = false;
	static ::size_t SigMax = false;
	static ::size_t MemPos = false;

	SigMax = SigSize - true;
	MemSize = vMem.size() - SigMax;

	for (MemIter = false; MemIter < MemSize; MemIter++)
	{
		for (SigIter = false; SigIter < SigSize; SigIter++)
		{
			MemPos = MemIter + SigIter;
			MemByte = vMem[MemPos];

			if (MemByte == pSig[SigIter] || pSig[SigIter] == (unsigned char)('?') || pSig[SigIter] == (unsigned char)('*'))
			{
				if (false == QuestionMarkAllowsZero && false == MemByte && pSig[SigIter] == (unsigned char)('?'))
				{
					break;
				}

				if (SigIter != SigMax)
				{
					continue;
				}

				if (pAddr)
				{
					*pAddr = ::size_t(vMem.m_Data + MemIter);
				}

				return true;
			}

			break;
		}
	}

	if (pAddr)
	{
		*pAddr = false;
	}

	return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
	const ::SourceHook::CVector < unsigned char > vSig,
	::size_t* pAddr,
	bool QuestionMarkAllowsZero) noexcept
{
	static unsigned char MemByte = false;
	static unsigned char SigByte = false;
	static ::size_t MemSize = false;
	static ::size_t SigSize = false;
	static ::size_t MemIter = false;
	static ::size_t SigIter = false;
	static ::size_t SigMax = false;
	static ::size_t MemPos = false;

	SigSize = vSig.size();
	SigMax = SigSize - true;
	MemSize = vMem.size() - SigMax;

	for (MemIter = false; MemIter < MemSize; MemIter++)
	{
		for (SigIter = false; SigIter < SigSize; SigIter++)
		{
			MemPos = MemIter + SigIter;
			SigByte = vSig[SigIter];
			MemByte = vMem[MemPos];

			if (MemByte == SigByte || SigByte == (unsigned char)('?') || SigByte == (unsigned char)('*'))
			{
				if (false == QuestionMarkAllowsZero && false == MemByte && SigByte == (unsigned char)('?'))
				{
					break;
				}

				if (SigIter != SigMax)
				{
					continue;
				}

				if (pAddr)
				{
					*pAddr = ::size_t(vMem.m_Data + MemIter);
				}

				return true;
			}

			break;
		}
	}

	if (pAddr)
	{
		*pAddr = false;
	}

	return false;
}

bool vectorizeSignature(const char* pSig,
	::SourceHook::CVector < unsigned char >& vSig,
	const char* pCharactersToSkip) noexcept
{
	static char* pWord = NULL;
	static char* pPos = NULL;
	static ::size_t Byte = false;
	static ::SourceHook::String sCopy;

	vSig.clear();
	if (NULL == pSig || false == *pSig)
	{
		return false;
	}

	sCopy = pSig;
	pWord = ::strtok_s(sCopy.v, pCharactersToSkip, &pPos);
	if (NULL == pWord || false == *pWord)
	{
		pPos = NULL;
		sCopy.clear();
		return false;
	}

	while (NULL != pWord && false != *pWord)
	{
		if (false == ::_strnicmp("0X", pWord, sizeof(short)))
		{
			pWord += sizeof(short);
		}

		switch (*pWord)
		{
		case '*':
		{
			vSig.push_back((unsigned char)('*'));
			break;
		}

		case '?':
		{
			vSig.push_back((unsigned char)('?'));
			break;
		}

		default:
		{
			Byte = (::std::size_t) ::strtoull(pWord, NULL, int(16));
			vSig.push_back((unsigned char)(Byte));
			break;
		}
		}

		pWord = ::strtok_s(NULL, pCharactersToSkip, &pPos);
	}

	pPos = NULL;
	sCopy.clear();
	return true;
}

bool vectorizeSignature(const ::SourceHook::String& sSig,
	::SourceHook::CVector < unsigned char >& vSig,
	const char* pCharactersToSkip) noexcept
{
	static char* pWord = NULL;
	static char* pPos = NULL;
	static ::size_t Byte = false;
	static ::SourceHook::String sCopy;

	vSig.clear();
	if (sSig.empty())
	{
		return false;
	}

	sCopy = sSig;
	pWord = ::strtok_s(sCopy.v, pCharactersToSkip, &pPos);
	if (NULL == pWord || false == *pWord)
	{
		pPos = NULL;
		sCopy.clear();
		return false;
	}

	while (NULL != pWord && false != *pWord)
	{
		if (false == ::_strnicmp("0X", pWord, sizeof(short)))
		{
			pWord += sizeof(short);
		}

		switch (*pWord)
		{
		case '*':
		{
			vSig.push_back((unsigned char)('*'));
			break;
		}

		case '?':
		{
			vSig.push_back((unsigned char)('?'));
			break;
		}

		default:
		{
			Byte = (::std::size_t) ::strtoull(pWord, NULL, int(16));
			vSig.push_back((unsigned char)(Byte));
			break;
		}
		}

		pWord = ::strtok_s(NULL, pCharactersToSkip, &pPos);
	}

	pPos = NULL;
	sCopy.clear();
	return true;
}

void displayVectorizedSignature(const ::SourceHook::CVector < unsigned char > vSig,
	bool LowerCase, int (*displayFunc) (const char*, ...)) noexcept
{
	static ::size_t Iter = false;
	static ::size_t Size = false;
	static unsigned char Byte = false;

	if (vSig.empty())
	{
		displayFunc("Signature: N/ A\n");
		return;
	}

	displayFunc("Signature:");
	Size = vSig.size();

	for (Iter = false; Iter < Size; Iter++)
	{
		Byte = vSig[Iter];

		switch (Byte)
		{
			case (unsigned char)(0x2A) :
			{
				displayFunc(" *");
				break;
			}

			case (unsigned char)(0x3F) :
			{
				displayFunc(" ?");
				break;
			}

			default:
			{
				displayFunc(LowerCase ? " %02x" : " %02X", Byte);
				break;
			}
		}
	}

	displayFunc("\n");
}

void displayVectorizedSignature(const unsigned char* pSig, ::size_t SigSize,
	bool LowerCase, int (*displayFunc) (const char*, ...)) noexcept
{
	static ::size_t Iter = false;
	static unsigned char Byte = false;

	if (!pSig)
	{
		displayFunc("Signature: N/ A\n");
		return;
	}

	displayFunc("Signature:");

	for (Iter = false; Iter < SigSize; Iter++)
	{
		Byte = pSig[Iter];

		switch (Byte)
		{
			case (unsigned char)(0x2A) :
			{
				displayFunc(" *");
				break;
			}

			case (unsigned char)(0x3F) :
			{
				displayFunc(" ?");
				break;
			}

			default:
			{
				displayFunc(LowerCase ? " %02x" : " %02X", Byte);
				break;
			}
		}
	}

	displayFunc("\n");
}
