/* ======== SourceMM ========
* Copyright (C) 2004-2010 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_CSTRING_H
#define _INCLUDE_CSTRING_H

#include <cstring>

#ifndef __linux__
#include <Shlwapi.h>
#endif

//
// File originally from AMX Mod X
//

namespace SourceHook
{
    class String
    {
    public:
        String()
        {
            v = NULL;
            a_size = 0;
        }

        ~String()
        {
#ifndef __linux__
            if (v) delete[] v;
#else
            if (v) ::free(v);
#endif
        }

        String(const char* o)
        {
            v = NULL;
            a_size = 0;
            assign(o);
        }

        String(const String& o)
        {
            v = NULL;
            a_size = 0;
            assign(o);
        }

        bool operator == (const String& o)
        {
            return cmp(o) == 0;
        }

        bool operator == (const char* o)
        {
            return cmp(o) == 0;
        }

        const char* c_str() { return v ? v : ""; }
        const char* c_str() const { return v ? v : ""; }

        void append(const char* s)
        {
            Grow(size() + ::strlen(s) + 1);
#ifdef __linux__
            ::strcat(v, s);
#else
            ::strcat_s(v, a_size, s);
#endif
        }

        void append(const char c)
        {
            ::size_t len = size();
            Grow(len + 2);
            v[len] = c;
            v[len + 1] = '\0';
        }

        void append(const String& d)
        {
            append(d.c_str());
        }

        void assign(const String& s)
        {
            assign(s.c_str());
        }

        void assign(const char* s)
        {
            if (!s)
                clear();
            else {
                Grow(::strlen(s) + 1, false);
#ifdef __linux__
                ::strcpy(v, s);
#else
                ::strcpy_s(v, a_size, s);
#endif
            }
        }

        void clear()
        {
            if (v) v[0] = '\0';
        }

        int cmp(const char* d) const
        {
            return !v ? ::strcmp("", d) : ::strcmp(v, d);
        }

        int cmp(const String& o) const
        {
            return !v ? ::strcmp("", o.c_str()) : ::strcmp(v, o.c_str());
        }

        int cmpn(const char* d, ::size_t n) const
        {
            return !v ? ::strncmp("", d, n) : ::strncmp(v, d, n);
        }

        int cmpn(const String& o, ::size_t n) const
        {
            return !v ? ::strncmp("", o.c_str(), n) : ::strncmp(v, o.c_str(), n);
        }

        int icmp(const char* d) const
        {
            return !v ? ::_stricmp("", d) : ::_stricmp(v, d);
        }

        int icmp(const String& o) const
        {
            return !v ? ::_stricmp("", o.c_str()) : ::_stricmp(v, o.c_str());
        }

        int icmpn(const char* d, ::size_t n) const
        {
            return !v ? ::_strnicmp("", d, n) : ::_strnicmp(v, d, n);
        }

        int icmpn(const String& o, ::size_t n) const
        {
            return !v ? ::_strnicmp("", o.c_str(), n) : ::_strnicmp(v, o.c_str(), n);
        }

        bool has(const char* d) const
        {
            return !v ? bool(::strstr("", d)) : bool(::strstr(v, d));
        }

        bool has(const String& o) const
        {
            return !v ? bool(::strstr("", o.c_str())) : bool(::strstr(v, o.c_str()));
        }

        bool ihas(const char* d) const
        {
            return !v ? bool(::StrStrIA("", d)) : bool(::StrStrIA(v, d));
        }

        bool ihas(const String& o) const
        {
            return !v ? bool(::StrStrIA("", o.c_str())) : bool(::StrStrIA(v, o.c_str()));
        }

        bool empty() const
        {
            if (!v || v[0] == '\0')
                return true;
            return false;
        }

        ::size_t size() const
        {
            return v ? ::strlen(v) : 0;
        }

        ::size_t find(const char c, ::size_t idx = 0) const
        {
            ::size_t len = size();
            if (len < 1 || idx >= len)
                return npos;
            for (::size_t i = idx; i < len; i++)
            {
                if (v[i] == c)
                    return i;
            }
            return npos;
        }

        ::size_t find_last_of(const char c, ::size_t idx = npos) const
        {
            ::size_t len = size();
            if (len < 1 || idx >= len)
                return npos;
            ::size_t i;
            if (idx == npos)
                i = len - 1;
            else
                i = idx;

            for (; i + 1 > 0; i--)
            {
                if (v[i] == c)
                    return i;
            }
            return npos;
        }

        bool is_space(int c) const
        {
            if (c == '\f' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == ' ')
                return true;
            return false;
        }

        void trim()
        {
            if (!v) return;
            ::size_t i = 0, len = ::strlen(v);
            if (len == 1)
            {
                if (is_space(v[i]))
                {
                    clear();
                    return;
                }
            }

            unsigned char c0 = v[0];
            if (is_space(c0))
            {
                for (i = 0; i < len; i++)
                {
                    if (!is_space(v[i]) || (is_space(v[i]) && (i == len - 1)))
                    {
                        erase(0, i);
                        break;
                    }
                }
            }

            len = ::strlen(v);
            if (len < 1)
                return;

            if (is_space(v[len - 1]))
            {
                ::size_t j = 0;
                for (i = len - 1; i < len; i--)
                {
                    if (!is_space(v[i]) || (is_space(v[i]) && i == 0))
                    {
                        erase(i + 1, j);
                        break;
                    }
                    j++;
                }
            }

            if (len == 1)
            {
                if (is_space(v[0]))
                    clear();
            }
        }

        void erase(::size_t start, ::size_t num = npos)
        {
            if (!v) return;
            ::size_t i, len = size();
            if (num == npos || start + num > len - start)
                num = len - start;

            bool cpy = false;
            for (i = 0; i < len; i++)
            {
                if (i >= start && i < start + num)
                {
                    if (i + num < len)
                        v[i] = v[i + num];
                    else
                        v[i] = 0;
                    cpy = true;
                }
                else if (cpy) {
                    if (i + num < len)
                        v[i] = v[i + num];
                    else
                        v[i] = 0;
                }
            }

            len -= num;
            v[len] = 0;
        }

        String substr(::size_t idx, ::size_t num = npos) const
        {
            if (!v)
            {
                String b("");
                return b;
            }

            String ns;
            ::size_t len = size();
            if (idx >= len || !v)
                return ns;

            if (num == npos)
                num = len - idx;
            else if (idx + num >= len)
                num = len - idx;

            ::size_t i, nslen = num + 2;
            ns.Grow(nslen);
            for (i = idx; i < idx + num; i++)
                ns.append(v[i]);

            return ns;
        }

        void toLower()
        {
            if (!v) return;
            ::size_t i, len = ::strlen(v);
            for (i = 0; i < len; i++)
            {
                if (v[i] > 64 && v[i] < 91)
                    v[i] &= ~(1 << 5);
            }
        }

        String& operator = (const String& s)
        {
            assign(s);
            return *this;
        }

        String& operator = (const char* s)
        {
            assign(s);
            return *this;
        }

        char operator [] (::size_t i) const
        {
            if (!v || i >= size())
                return -1;

            return v[i];
        }

        int at(::size_t i) const
        {
            if (!v || i >= size())
                return -1;

            return v[i];
        }

        bool at(::size_t i, char c)
        {
            if (!v || i >= size())
                return false;

            v[i] = c;
            return true;
        }

#ifdef __linux__
        void Grow(::size_t s, bool cpy = true)
        {
            if (s <= a_size)
                return;
            a_size = s + 1;
            char* n = (char*) ::calloc(1, a_size);
            if (cpy && v)
                ::strcpy(n, v);
            if (v)
                ::free(v);
            else
                ::strcpy(n, "");
            v = n;
        }
#else
        void Grow(::size_t s, bool cpy = true)
        {
            if (s <= a_size)
                return;
            a_size = s + 1;
            char* n = new char[a_size];
            if (cpy && v)
                ::strcpy_s(n, a_size, v);
            if (v)
                delete[] v;
            else
                ::strcpy_s(n, a_size, "");
            v = n;
        }
#endif

        char* v;
        ::size_t a_size;

        static const ::size_t npos = static_cast <::size_t> (-1);
    };
};

#endif
