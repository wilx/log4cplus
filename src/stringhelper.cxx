// Module:  Log4CPLUS
// File:    stringhelper.cxx
// Created: 4/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2009 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/streams.h>
#include <log4cplus/internal/internal.h>

#include <iterator>
#include <algorithm>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cctype>

#ifdef UNICODE
#  include <cassert>
#  include <vector>
#endif


namespace log4cplus
{

namespace internal
{

log4cplus::tstring const empty_str;

} // namespace internal

} // namespace log4cplus


//////////////////////////////////////////////////////////////////////////////
// Global Methods
//////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE

log4cplus::tostream& 
operator <<(log4cplus::tostream& stream, const char* str)
{
    return (stream << log4cplus::helpers::towstring(str));
}

#endif


namespace log4cplus
{

namespace helpers
{

#ifdef LOG4CPLUS_WORKING_LOCALE

static
void
clear_mbstate (std::mbstate_t & mbs)
{
    // Initialize/clear mbstate_t type.
    // XXX: This is just a hack that works. The shape of mbstate_t varies
    // from single unsigned to char[128]. Without some sort of initialization
    // the codecvt::in/out methods randomly fail because the initial state is
    // random/invalid.
    std::memset (&mbs, 0, sizeof (std::mbstate_t));
}


template <typename IntChar, typename ExtChar>
struct codecvt_use
{
    typedef ExtChar ext_char_type;
    typedef IntChar int_char_type;

    typedef std::codecvt<int_char_type, ext_char_type, std::mbstate_t>
            codecvt_type;

    static
    void
    in (std::basic_string<int_char_type> & outstr, const ext_char_type * src,
        size_t size, std::locale const & loc)
    {
        typedef ExtChar src_char_type;
        typedef IntChar out_char_type;

        if (size == 0)
        {
            outstr.clear ();
            return;
        }

        const codecvt_type & cdcvt = std::use_facet<codecvt_type>(loc);
        std::mbstate_t state;
        clear_mbstate (state);

        src_char_type const * from_first = src;
        size_t const from_size = size;
        src_char_type const * const from_last = from_first + from_size;
        src_char_type const * from_next = from_first;

        std::vector<out_char_type> dest (from_size);

        out_char_type * to_first = &dest.front ();
        size_t to_size = dest.size ();
        out_char_type * to_last = to_first + to_size;
        out_char_type * to_next = to_first;

        codecvt_type::result result;
        size_t converted = 0;
        while (true)
        {
            result = cdcvt.in (
                state, from_first, from_last,
                from_next, to_first, to_last,
                to_next);
            // XXX: Even if only half of the input has been converted the
            // in() method returns CodeCvt::ok. I think it should return
            // CodeCvt::partial.
            if ((result == codecvt_type::partial || result == codecvt_type::ok)
                && from_next != from_last)
            {
                to_size = dest.size () * 2;
                dest.resize (to_size);
                converted = to_next - to_first;
                to_first = &dest.front ();
                to_last = to_first + to_size;
                to_next = to_first + converted;
                continue;
            }
            else if (result == codecvt_type::ok && from_next == from_last)
                break;
            else if (result == codecvt_type::error
                && to_next != to_last && from_next != from_last)
            {
                clear_mbstate (state);
                ++from_next;
                from_first = from_next;
                *to_next = L'?';
                ++to_next;
                to_first = to_next;
            }
            else
                break;
        }
        converted = to_next - &dest[0];

        outstr.assign (dest.begin (), dest.begin () + converted);
    }

    
    static
    void
    out (std::basic_string<ext_char_type> & outstr, const int_char_type * src,
        size_t size, std::locale const & loc)
    {
        typedef IntChar src_char_type;
        typedef ExtChar out_char_type;

        if (size == 0)
        {
            outstr.clear ();
            return;
        }

        const codecvt_type & cdcvt = std::use_facet<codecvt_type>(loc);
        std::mbstate_t state;
        clear_mbstate (state);

        src_char_type const * from_first = src;
        size_t const from_size = size;
        src_char_type const * const from_last = from_first + from_size;
        src_char_type const * from_next = from_first;

        std::vector<out_char_type> dest (from_size);

        out_char_type * to_first = &dest.front ();
        size_t to_size = dest.size ();
        out_char_type * to_last = to_first + to_size;
        out_char_type * to_next = to_first;

        codecvt_type::result result;
        size_t converted = 0;
        while (from_next != from_last)
        {
            result = cdcvt.out (
                state, from_first, from_last,
                from_next, to_first, to_last,
                to_next);
            // XXX: Even if only half of the input has been converted the
            // in() method returns CodeCvt::ok with VC8. I think it should
            // return CodeCvt::partial.
            if ((result == codecvt_type::partial || result == codecvt_type::ok)
                && from_next != from_last)
            {
                to_size = dest.size () * 2;
                dest.resize (to_size);
                converted = to_next - to_first;
                to_first = &dest.front ();
                to_last = to_first + to_size;
                to_next = to_first + converted;
            }
            else if (result == codecvt_type::ok && from_next == from_last)
                break;
            else if (result == codecvt_type::error
                && to_next != to_last && from_next != from_last)
            {
                clear_mbstate (state);
                ++from_next;
                from_first = from_next;
                *to_next = '?';
                ++to_next;
                to_first = to_next;
            }
            else
                break;
        }
        converted = to_next - &dest[0];

        outstr.assign (dest.begin (), dest.begin () + converted);
    }
};


template <typename OutChar, typename InChar>
static
void
towstring_internal (std::basic_string<OutChar> & outstr, const InChar * src,
    size_t size, std::locale const & loc)
{
    typedef OutChar out_char_type;
    typedef InChar in_char_type;

    if (size == 0)
    {
        outstr.clear ();
        return;
    }

    typedef std::codecvt<out_char_type, in_char_type, std::mbstate_t> CodeCvt;
    const CodeCvt & cdcvt = std::use_facet<CodeCvt>(loc);
    std::mbstate_t state;
    clear_mbstate (state);

    in_char_type const * from_first = src;
    size_t const from_size = size;
    in_char_type const * const from_last = from_first + from_size;
    in_char_type const * from_next = from_first;

    std::vector<out_char_type> dest (from_size);

    out_char_type * to_first = &dest.front ();
    size_t to_size = dest.size ();
    out_char_type * to_last = to_first + to_size;
    out_char_type * to_next = to_first;

    CodeCvt::result result;
    size_t converted = 0;
    while (true)
    {
        result = cdcvt.in (
            state, from_first, from_last,
            from_next, to_first, to_last,
            to_next);
        // XXX: Even if only half of the input has been converted the
        // in() method returns CodeCvt::ok. I think it should return
        // CodeCvt::partial.
        if ((result == CodeCvt::partial || result == CodeCvt::ok)
            && from_next != from_last)
        {
            to_size = dest.size () * 2;
            dest.resize (to_size);
            converted = to_next - to_first;
            to_first = &dest.front ();
            to_last = to_first + to_size;
            to_next = to_first + converted;
            continue;
        }
        else if (result == CodeCvt::ok && from_next == from_last)
            break;
        else if (result == CodeCvt::error
            && to_next != to_last && from_next != from_last)
        {
            clear_mbstate (state);
            ++from_next;
            from_first = from_next;
            *to_next = L'?';
            ++to_next;
            to_first = to_next;
        }
        else
            break;
    }
    converted = to_next - &dest[0];

    outstr.assign (dest.begin (), dest.begin () + converted);
}


template <typename SrcChar>
static inline
std::wstring
to_wstring_from_str (std::basic_string<SrcChar> const & src,
    std::locale const & loc)
{
    std::wstring ret;
    towstring_internal (ret, src.c_str (), src.size (), loc);
    return ret;
}


template <typename SrcChar>
static inline
std::wstring
to_wstring_from_cstr (SrcChar const * src, std::locale const & loc)
{
    std::wstring ret;
    towstring_internal (ret, src, std::char_traits<SrcChar>::length (src), loc);
    return ret;
}


std::wstring 
towstring(const std::string& src, std::locale const & loc)
{
    std::wstring ret;
    codecvt_use<wchar_t, char>::in (ret, src.c_str (), src.size (), loc);
    return ret;
}


std::wstring 
towstring(char const * src, std::locale const & loc)
{
    std::wstring ret;
    codecvt_use<wchar_t, char>::in (ret, src,
        std::char_traits<char>::length (src), loc);
    return ret;
}


#if defined (LOG4CPLUS_HAVE_CPP0X)
std::wstring 
towstring(const std::u16string& src, std::locale const & loc)
{
    return to_wstring_from_str (src, loc);
}


std::wstring 
towstring(char16_t const * src, std::locale const & loc)
{
    return to_wstring_from_cstr (src, loc);
}


std::wstring 
towstring(const std::u32string& src, std::locale const & loc)
{
    return to_wstring_from_str (src, loc);
}


std::wstring 
towstring(char32_t const * src, std::locale const & loc)
{
    return to_wstring_from_cstr (src, loc);
}

#endif


template <typename OutChar, typename InChar>
static
void
tostring_internal (std::basic_string<OutChar> & outstr, const InChar * src, size_t size,
    std::locale const & loc)
{
    typedef OutChar out_char_type;
    typedef InChar in_char_type;

    if (size == 0)
    {
        outstr.clear ();
        return;
    }

    typedef std::codecvt<in_char_type, out_char_type, std::mbstate_t> CodeCvt;
    const CodeCvt & cdcvt = std::use_facet<CodeCvt>(loc);
    std::mbstate_t state;
    clear_mbstate (state);

    in_char_type const * from_first = src;
    size_t const from_size = size;
    in_char_type const * const from_last = from_first + from_size;
    in_char_type const * from_next = from_first;

    std::vector<out_char_type> dest (from_size);

    out_char_type * to_first = &dest.front ();
    size_t to_size = dest.size ();
    out_char_type * to_last = to_first + to_size;
    out_char_type * to_next = to_first;

    CodeCvt::result result;
    size_t converted = 0;
    while (from_next != from_last)
    {
        result = cdcvt.out (
            state, from_first, from_last,
            from_next, to_first, to_last,
            to_next);
        // XXX: Even if only half of the input has been converted the
        // in() method returns CodeCvt::ok with VC8. I think it should
        // return CodeCvt::partial.
        if ((result == CodeCvt::partial || result == CodeCvt::ok)
            && from_next != from_last)
        {
            to_size = dest.size () * 2;
            dest.resize (to_size);
            converted = to_next - to_first;
            to_first = &dest.front ();
            to_last = to_first + to_size;
            to_next = to_first + converted;
        }
        else if (result == CodeCvt::ok && from_next == from_last)
            break;
        else if (result == CodeCvt::error
            && to_next != to_last && from_next != from_last)
        {
            clear_mbstate (state);
            ++from_next;
            from_first = from_next;
            *to_next = '?';
            ++to_next;
            to_first = to_next;
        }
        else
            break;
    }
    converted = to_next - &dest[0];

    outstr.assign (dest.begin (), dest.begin () + converted);
}


template <typename SrcChar>
static inline
std::string
to_string_from_str (std::basic_string<SrcChar> const & src,
    std::locale const & loc)
{
    std::string ret;
    tostring_internal (ret, src.c_str (), src.size (), loc);
    return ret;
}


template <typename SrcChar>
static inline
std::string
to_string_from_cstr (SrcChar const * src, std::locale const & loc)
{
    std::string ret;
    tostring_internal (ret, src, std::char_traits<SrcChar>::length (src), loc);
    return ret;
}


std::string 
tostring (const std::wstring& src, std::locale const & loc)
{
    return to_string_from_str (src, loc);
}


std::string 
tostring (wchar_t const * src, std::locale const & loc)
{
    return to_string_from_cstr (src, loc);
}


#if defined (LOG4CPLUS_HAVE_CPP0X)
std::string
tostring (const std::u16string& src, std::locale const & loc)
{
    return to_string_from_str (src, loc);
}


std::string
tostring (char16_t const * src, std::locale const & loc)
{
    return to_string_from_cstr (src, loc);
}


std::string
tostring (const std::u32string& src, std::locale const & loc)
{
    return to_string_from_str (src, loc);
}


std::string
tostring (char32_t const * src, std::locale const & loc)
{
    return to_string_from_cstr (src, loc);
}

#endif


#else // LOG4CPLUS_WORKING_LOCALE


static
void
tostring_internal (std::string & ret, wchar_t const * src, size_t size)
{
    ret.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
        ret[i] = static_cast<unsigned> (static_cast<int> (src[i])) < 256
            ? static_cast<char>(src[i]) : ' ';
    }
}


std::string 
tostring(const std::wstring& src)
{
    std::string ret;
    tostring_internal (ret, src.c_str (), src.size ());
    return ret;
}


std::string 
tostring(wchar_t const * src)
{
	assert (src);
    std::string ret;
    tostring_internal (ret, src, std::wcslen (src));
    return ret;
}


static
void
towstring_internal (std::wstring & ret, char const * src, size_t size)
{
    ret.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
        ret[i] = static_cast<wchar_t>
            (static_cast<unsigned char> (src[i]));
    }
}


std::wstring 
towstring(const std::string& src)
{
    std::wstring ret;
    towstring_internal (ret, src.c_str (), src.size ());
    return ret;
}


std::wstring 
towstring(char const * src)
{
    assert (src);
    std::wstring ret;
    towstring_internal (ret, src, std::strlen (src));
    return ret;
}

#endif // LOG4CPLUS_WORKING_LOCALE


tstring
toUpper(const log4cplus::tstring& s)
{
    tstring ret;
    std::transform(s.begin(), s.end(),
                   string_append_iterator<tstring>(ret),
#ifdef UNICODE
#  if (defined(__MWERKS__) && defined(__MACOS__)) || defined (LOG4CPLUS_WORKING_LOCALE)
                   std::towupper);
#  else
                   ::towupper);
#  endif
#else
                   ::toupper);
#endif

    return ret;
}


tstring
toLower(const tstring& s)
{
    tstring ret;
    std::transform(s.begin(), s.end(),
                   string_append_iterator<tstring>(ret),
#ifdef UNICODE
#  if (defined(__MWERKS__) && defined(__MACOS__))
                   std::towlower);
#  else
                   ::towlower);
#  endif
#else
                   ::tolower);
#endif

    return ret;
}


} // namespace helpers

} // namespace log4cplus
