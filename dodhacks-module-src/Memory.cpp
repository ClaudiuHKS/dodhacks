
#include "Memory.h"

#if 0

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    static ::size_t SigMax = false;
    static ::size_t MemPos = false;
    static ::size_t MemIter = false;
    static ::size_t SigIter = false;

    SigMax = SigSize - true;
    MemSize -= SigMax;

    for (MemIter = false; MemIter < MemSize; MemIter++)
        for (SigIter = false; SigIter < SigSize; SigIter++)
        {
            MemPos = MemIter + SigIter;

            if (pMem[MemPos] == pSig[SigIter] || pSig[SigIter] == (unsigned char)('?') || pSig[SigIter] == (unsigned char)('*'))
            {
                if (false == QuestionMarkAllowsZero && false == pMem[MemPos] && pSig[SigIter] == (unsigned char)('?'))
                    break;

                if (SigIter != SigMax)
                    continue;

                if (pAddr)
                    *pAddr = ::size_t(pMem + MemIter);

                return true;
            }

            break;
        }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    static ::size_t SigMax = false;
    static ::size_t MemPos = false;
    static ::size_t SigSize = false;
    static ::size_t MemIter = false;
    static ::size_t SigIter = false;
    static unsigned char SigByte = false;

    SigSize = vSig.size();
    SigMax = SigSize - true;
    MemSize -= SigMax;

    for (MemIter = false; MemIter < MemSize; MemIter++)
        for (SigIter = false; SigIter < SigSize; SigIter++)
        {
            MemPos = MemIter + SigIter;
            SigByte = vSig[SigIter];

            if (pMem[MemPos] == SigByte || SigByte == (unsigned char)('?') || SigByte == (unsigned char)('*'))
            {
                if (false == QuestionMarkAllowsZero && false == pMem[MemPos] && SigByte == (unsigned char)('?'))
                    break;

                if (SigIter != SigMax)
                    continue;

                if (pAddr)
                    *pAddr = ::size_t(pMem + MemIter);

                return true;
            }

            break;
        }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    static ::size_t SigMax = false;
    static ::size_t MemPos = false;
    static ::size_t MemSize = false;
    static ::size_t MemIter = false;
    static ::size_t SigIter = false;
    static unsigned char MemByte = false;

    SigMax = SigSize - true;
    MemSize = vMem.size() - SigMax;

    for (MemIter = false; MemIter < MemSize; MemIter++)
        for (SigIter = false; SigIter < SigSize; SigIter++)
        {
            MemPos = MemIter + SigIter;
            MemByte = vMem[MemPos];

            if (MemByte == pSig[SigIter] || pSig[SigIter] == (unsigned char)('?') || pSig[SigIter] == (unsigned char)('*'))
            {
                if (false == QuestionMarkAllowsZero && false == MemByte && pSig[SigIter] == (unsigned char)('?'))
                    break;

                if (SigIter != SigMax)
                    continue;

                if (pAddr)
                    *pAddr = ::size_t(vMem.m_Data + MemIter);

                return true;
            }

            break;
        }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    static ::size_t SigMax = false;
    static ::size_t MemPos = false;
    static ::size_t MemSize = false;
    static ::size_t SigSize = false;
    static ::size_t MemIter = false;
    static ::size_t SigIter = false;
    static unsigned char MemByte = false;
    static unsigned char SigByte = false;

    SigSize = vSig.size();
    SigMax = SigSize - true;
    MemSize = vMem.size() - SigMax;

    for (MemIter = false; MemIter < MemSize; MemIter++)
        for (SigIter = false; SigIter < SigSize; SigIter++)
        {
            MemPos = MemIter + SigIter;
            SigByte = vSig[SigIter];
            MemByte = vMem[MemPos];

            if (MemByte == SigByte || SigByte == (unsigned char)('?') || SigByte == (unsigned char)('*'))
            {
                if (false == QuestionMarkAllowsZero && false == MemByte && SigByte == (unsigned char)('?'))
                    break;

                if (SigIter != SigMax)
                    continue;

                if (pAddr)
                    *pAddr = ::size_t(vMem.m_Data + MemIter);

                return true;
            }

            break;
        }

    if (pAddr)
        *pAddr = false;

    return false;
}

#elif defined __AVX2__

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (!pMem || !pSig || SigSize < 1 || MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (pSig[Iter] != '?' && pSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = pSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm256_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 32 <= MemSize)
    {
        const auto Data = ::_mm256_loadu_si256((const ::__m256i*) (pMem + Pos));
        const auto Cmp = ::_mm256_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm256_movemask_epi8(Cmp);

        while (Mask)
        {
            const auto Candidate = Pos + ::_tzcnt_u32(Mask);
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = pSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = pMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(pMem + Beg);

                        return true;
                    }
                }
            }

            Mask = ::_blsr_u32(Mask);
        }

        Pos += 32;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (pMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = pSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = pMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(pMem + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (vMem.empty() || !pSig || SigSize < 1)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto MemSize = vMem.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (pSig[Iter] != '?' && pSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = pSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm256_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 32 <= MemSize)
    {
        const auto Data = ::_mm256_loadu_si256((const ::__m256i*) (vMem.m_Data + Pos));
        const auto Cmp = ::_mm256_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm256_movemask_epi8(Cmp);

        while (Mask)
        {
            const auto Candidate = Pos + ::_tzcnt_u32(Mask);
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = pSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = vMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(vMem.m_Data + Beg);

                        return true;
                    }
                }
            }

            Mask = ::_blsr_u32(Mask);
        }

        Pos += 32;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (vMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = pSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = vMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(vMem.m_Data + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (vMem.empty() || vSig.empty())
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto SigSize = vSig.size(), MemSize = vMem.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (vSig[Iter] != '?' && vSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = vSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm256_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 32 <= MemSize)
    {
        const auto Data = ::_mm256_loadu_si256((const ::__m256i*) (vMem.m_Data + Pos));
        const auto Cmp = ::_mm256_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm256_movemask_epi8(Cmp);

        while (Mask)
        {
            const auto Candidate = Pos + ::_tzcnt_u32(Mask);
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = vSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = vMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(vMem.m_Data + Beg);

                        return true;
                    }
                }
            }

            Mask = ::_blsr_u32(Mask);
        }

        Pos += 32;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (vMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = vSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = vMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(vMem.m_Data + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (!pMem || vSig.empty())
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto SigSize = vSig.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (vSig[Iter] != '?' && vSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = vSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm256_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 32 <= MemSize)
    {
        const auto Data = ::_mm256_loadu_si256((const ::__m256i*) (pMem + Pos));
        const auto Cmp = ::_mm256_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm256_movemask_epi8(Cmp);

        while (Mask)
        {
            const auto Candidate = Pos + ::_tzcnt_u32(Mask);
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = vSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = pMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(pMem + Beg);

                        return true;
                    }
                }
            }

            Mask = ::_blsr_u32(Mask);
        }

        Pos += 32;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (pMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = vSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = pMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(pMem + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

#elif defined __AVX__

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (!pMem || !pSig || SigSize < 1 || MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (pSig[Iter] != '?' && pSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = pSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 16 <= MemSize)
    {
        const auto Data = ::_mm_loadu_si128((const ::__m128i*) (pMem + Pos));
        const auto Cmp = ::_mm_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm_movemask_epi8(Cmp);

        while (Mask)
        {
#ifndef __linux__
            unsigned long X;
            ::_BitScanForward(&X, Mask);
            const auto Candidate = Pos + X;
#else
            const auto Candidate = Pos + ::__builtin_ctz(Mask);
#endif
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = pSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = pMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(pMem + Beg);

                        return true;
                    }
                }
            }

            Mask &= (Mask - true);
        }

        Pos += 16;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (pMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = pSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = pMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(pMem + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (vMem.empty() || !pSig || SigSize < 1)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto MemSize = vMem.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (pSig[Iter] != '?' && pSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = pSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 16 <= MemSize)
    {
        const auto Data = ::_mm_loadu_si128((const ::__m128i*) (vMem.m_Data + Pos));
        const auto Cmp = ::_mm_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm_movemask_epi8(Cmp);

        while (Mask)
        {
#ifndef __linux__
            unsigned long X;
            ::_BitScanForward(&X, Mask);
            const auto Candidate = Pos + X;
#else
            const auto Candidate = Pos + ::__builtin_ctz(Mask);
#endif
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = pSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = vMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(vMem.m_Data + Beg);

                        return true;
                    }
                }
            }

            Mask &= (Mask - true);
        }

        Pos += 16;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (vMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = pSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = vMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(vMem.m_Data + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (vMem.empty() || vSig.empty())
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto SigSize = vSig.size(), MemSize = vMem.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (vSig[Iter] != '?' && vSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = vSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 16 <= MemSize)
    {
        const auto Data = ::_mm_loadu_si128((const ::__m128i*) (vMem.m_Data + Pos));
        const auto Cmp = ::_mm_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm_movemask_epi8(Cmp);

        while (Mask)
        {
#ifndef __linux__
            unsigned long X;
            ::_BitScanForward(&X, Mask);
            const auto Candidate = Pos + X;
#else
            const auto Candidate = Pos + ::__builtin_ctz(Mask);
#endif
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = vSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = vMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(vMem.m_Data + Beg);

                        return true;
                    }
                }
            }

            Mask &= (Mask - true);
        }

        Pos += 16;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (vMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = vSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = vMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(vMem.m_Data + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (!pMem || vSig.empty())
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto SigSize = vSig.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (vSig[Iter] != '?' && vSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = vSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;
    const auto Needle = ::_mm_set1_epi8((char)AnchorByte);
    ::size_t Pos = false;

    while (Pos + 16 <= MemSize)
    {
        const auto Data = ::_mm_loadu_si128((const ::__m128i*) (pMem + Pos));
        const auto Cmp = ::_mm_cmpeq_epi8(Data, Needle);
        auto Mask = (unsigned) ::_mm_movemask_epi8(Cmp);

        while (Mask)
        {
#ifndef __linux__
            unsigned long X;
            ::_BitScanForward(&X, Mask);
            const auto Candidate = Pos + X;
#else
            const auto Candidate = Pos + ::__builtin_ctz(Mask);
#endif
            if (Candidate >= AnchorIdx)
            {
                const auto Beg = Candidate - AnchorIdx;
                if (Beg <= Stamp)
                {
                    auto Match = true;
                    for (Iter = false; Iter < SigSize; ++Iter)
                    {
                        const auto Sig = vSig[Iter];
                        if (Sig == '*')
                            continue;

                        const auto Mem = pMem[Beg + Iter];
                        if (Sig == '?')
                        {
                            if (!QuestionMarkAllowsZero && !Mem)
                            {
                                Match = false;
                                break;
                            }

                            continue;
                        }

                        if (Mem != Sig)
                        {
                            Match = false;
                            break;
                        }
                    }

                    if (Match)
                    {
                        if (pAddr)
                            *pAddr = (::size_t)(pMem + Beg);

                        return true;
                    }
                }
            }

            Mask &= (Mask - true);
        }

        Pos += 16;
    }

    auto Beg = (Pos >= AnchorIdx) ? (Pos - AnchorIdx) : false;
    for (; Beg <= Stamp; ++Beg)
    {
        if (pMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = vSig[Iter];
            if (Sig == '*')
                continue;

            const auto Mem = pMem[Beg + Iter];
            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Sig != Mem)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(pMem + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

#else

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (!pMem || vSig.empty())
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto SigSize = vSig.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (vSig[Iter] != '?' && vSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = vSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;

    for (::size_t Beg = false; Beg <= Stamp; ++Beg)
    {
        if (pMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = vSig[Iter];
            const auto Mem = pMem[Beg + Iter];

            if (Sig == '*')
                continue;

            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Mem != Sig)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(pMem + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const unsigned char* pMem, ::size_t MemSize,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (!pMem || !pSig || SigSize < 1 || MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (pSig[Iter] != '?' && pSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = pSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;

    for (::size_t Beg = false; Beg <= Stamp; ++Beg)
    {
        if (pMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = pSig[Iter];
            const auto Mem = pMem[Beg + Iter];

            if (Sig == '*')
                continue;

            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Mem != Sig)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(pMem + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const unsigned char* pSig, ::size_t SigSize,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (vMem.empty() || !pSig || SigSize < 1)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto MemSize = vMem.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (pSig[Iter] != '?' && pSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = pSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;

    for (::size_t Beg = false; Beg <= Stamp; ++Beg)
    {
        if (vMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = pSig[Iter];
            const auto Mem = vMem[Beg + Iter];

            if (Sig == '*')
                continue;

            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Mem != Sig)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(vMem.m_Data + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

bool findInMemory(const ::SourceHook::CVector < unsigned char > vMem,
    const ::SourceHook::CVector < unsigned char > vSig,
    ::size_t* pAddr,
    bool QuestionMarkAllowsZero) noexcept
{
    if (vMem.empty() || vSig.empty())
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    const auto SigSize = vSig.size(), MemSize = vMem.size();
    if (MemSize < SigSize)
    {
        if (pAddr)
            *pAddr = false;

        return false;
    }

    ::size_t AnchorIdx = SIZE_MAX, Iter = false;
    for (; Iter < SigSize; ++Iter)
        if (vSig[Iter] != '?' && vSig[Iter] != '*')
        {
            AnchorIdx = Iter;
            goto keepUp;
        }

    if (pAddr)
        *pAddr = false;

    return false;

keepUp:
    const auto AnchorByte = vSig[AnchorIdx];
    const auto Stamp = MemSize - SigSize;

    for (::size_t Beg = false; Beg <= Stamp; ++Beg)
    {
        if (vMem[Beg + AnchorIdx] != AnchorByte)
            continue;

        auto Match = true;
        for (Iter = false; Iter < SigSize; ++Iter)
        {
            const auto Sig = vSig[Iter];
            const auto Mem = vMem[Beg + Iter];

            if (Sig == '*')
                continue;

            if (Sig == '?')
            {
                if (!QuestionMarkAllowsZero && !Mem)
                {
                    Match = false;
                    break;
                }

                continue;
            }

            if (Mem != Sig)
            {
                Match = false;
                break;
            }
        }

        if (Match)
        {
            if (pAddr)
                *pAddr = (::size_t)(vMem.m_Data + Beg);

            return true;
        }
    }

    if (pAddr)
        *pAddr = false;

    return false;
}

#endif

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
        return false;

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
            pWord += sizeof(short);

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
        return false;

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
            pWord += sizeof(short);

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
