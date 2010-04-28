//   Copyright (C) 2009, Vaclav Haisman. All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without modifica-
//   tion, are permitted provided that the following conditions are met:
//
//   1. Redistributions of  source code must  retain the above copyright  notice,
//      this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//   FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//   APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//   INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//   DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//   OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//   ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//   (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LOG4CPLUS_HELPERS_STRINGPARAM_HXX
#define LOG4CPLUS_HELPERS_STRINGPARAM_HXX

#include <string>
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cwchar>
#include <cassert>
#include <log4cplus/config.hxx>
#include <log4cplus/tstring.h>


namespace log4cplus { namespace helpers {

namespace stringparam_impl
{


template <bool Val>
struct boolean
{
    enum Value { value = Val };
};


template <bool B, class T = void>
struct enable_if_c
    : public boolean<true>
{
    typedef T type;
};


template <class T>
struct enable_if_c<false, T>
    : public boolean<false>
{ };


template <class Cond, class T = void>
struct enable_if
    : public enable_if_c<Cond::value, T>
{ };


template <typename T, typename U>
struct is_same
    : public boolean<false>
{ };


template <typename T>
struct is_same<T, T>
    : public boolean<true>
{ };


enum Flags
{
    Undefined = 0x0000,
    Defined   = 0x0001,

    CharTypeBit  = 1,
    StringBit    = 3,
    OwnershipBit = 4,

    CharTypeMask = 3 << CharTypeBit,
    StringTypeMask = 1 << StringBit,

    CharArray   = 0 << CharTypeBit,
    WCharArray  = 1 << CharTypeBit,
#if defined (LOG4CPLUS_HAVE_CPP0X)
    Char16Array = 2 << CharTypeBit,
    Char32Array = 3 << CharTypeBit,
#endif
#if defined (UNICODE)
    TCharArray = WCharArray,
#else
    TCharArray = CharArray,
#endif

    StdString    = CharArray | 1 << StringBit,
    StdWString   = WCharArray | 1 << StringBit,
#if defined (LOG4CPLUS_HAVE_CPP0X)
    StdU16String = Char16Array | 1 << StringBit,
    StdU32String = Char32Array | 1 << StringBit,
#endif
#if defined (UNICODE)
    TString    = StdWString,
#else
    TString        = StdString,
#endif

    Ownership = 1 << OwnershipBit
};


template <typename T>
struct char_array_value_type
{
    typedef T char_type;

    std::size_t size;
    char_type const * ptr;
};


template <typename T>
struct string_value_type
{
    typedef T char_type;

    std::size_t size;
    std::basic_string<char_type> const * ptr;
};


union param_value_type
{
    char_array_value_type<void> unspec;
    char_array_value_type<char> array;
    char_array_value_type<wchar_t> warray;
    char_array_value_type<log4cplus::tchar> tarray;
    string_value_type<char> str;
    string_value_type<wchar_t> wstr;
    string_value_type<log4cplus::tchar> tstr;
#if defined (LOG4CPLUS_HAVE_CPP0X)
    char_array_value_type<char16_t> u16array;
    char_array_value_type<char32_t> u32array;
    string_value_type<char16_t> u16str;
    string_value_type<char32_t> u32str;
#endif
};


template <typename T>
struct param_traits;

#define LOG4CPLUS_DEF_PARAM_TRAITS(T, VAL)       \
    template <>                                  \
    struct param_traits<T>                       \
    {                                            \
        enum TypeValue { type_value = (VAL) };   \
    }

LOG4CPLUS_DEF_PARAM_TRAITS (char *, CharArray);
LOG4CPLUS_DEF_PARAM_TRAITS (char const *, CharArray);
LOG4CPLUS_DEF_PARAM_TRAITS (wchar_t *, WCharArray);
LOG4CPLUS_DEF_PARAM_TRAITS (wchar_t const *, WCharArray);
LOG4CPLUS_DEF_PARAM_TRAITS (std::string, StdString);
LOG4CPLUS_DEF_PARAM_TRAITS (std::wstring, StdWString);
#if defined (LOG4CPLUS_HAVE_CPP0X)
LOG4CPLUS_DEF_PARAM_TRAITS (char16_t *, Char16Array);
LOG4CPLUS_DEF_PARAM_TRAITS (char16_t const *, Char16Array);
LOG4CPLUS_DEF_PARAM_TRAITS (char32_t *, Char32Array);
LOG4CPLUS_DEF_PARAM_TRAITS (char32_t const *, Char32Array);
LOG4CPLUS_DEF_PARAM_TRAITS (std::u16string, StdU16String);
LOG4CPLUS_DEF_PARAM_TRAITS (std::u32string, StdU32String);
#endif

#undef LOG4CPLUS_DEF_PARAM_TRAITS


struct make_copy_tag { };


#if defined (LOG4CPLUS_HAVE_CPP0X)
size_t const STRING_PARAM_TABLE_SIZE = 8;
#else
size_t const STRING_PARAM_TABLE_SIZE = 4;
#endif


class string_param
{
    struct sfinae_filler
    { };


public:
    typedef std::numeric_limits<std::size_t> size_t_limits;


    string_param ()
        : type (Undefined)
    { }


    static LOG4CPLUS_EXPORT make_copy_tag const makecopy;


    string_param (string_param const & other, make_copy_tag const &)
        : type (other.type | Ownership)
    {
        make_copy (other);
    }


    template <typename T>
    string_param (T const & param,
        typename enable_if_c<is_same<char const *, T>::value
            || is_same<char *, T>::value
            || is_same<wchar_t const *, T>::value
            || is_same<wchar_t *, T>::value
#if defined (LOG4CPLUS_HAVE_CPP0X)
            || is_same<char16_t *, T>::value
            || is_same<char16_t const *, T>::value
            || is_same<char32_t *, T>::value
            || is_same<char32_t const *, T>::value
#endif
            , std::size_t>::type strsize
            = (size_t_limits::max) ())
        : type (param_traits<T>::type_value | Defined)
    {
        value.unspec.size = strsize;
        value.unspec.ptr = param;
    }


    template <std::size_t N, typename T>
    string_param (T const (& param)[N],
        typename enable_if_c<is_same<char, T>::value
            || is_same<wchar_t, T>::value
#if defined (LOG4CPLUS_HAVE_CPP0X)
            || is_same<char16_t, T>::value
            || is_same<char32_t, T>::value
#endif
            , sfinae_filler>::type * = 0)
        : type (param_traits<T *>::type_value | Defined)
    {
        value.unspec.size = N - 1;
        value.unspec.ptr = &param[0];
    }


    template <typename T>
    string_param (std::basic_string<T> const & str,
        typename enable_if_c<is_same<char, T>::value
            || is_same<wchar_t, T>::value
#if defined (LOG4CPLUS_HAVE_CPP0X)
            || is_same<char16_t, T>::value
            || is_same<char32_t, T>::value
#endif
            , sfinae_filler>::type * = 0)
        : type (param_traits<std::basic_string<T> >::type_value | Defined)
    {
        value.unspec.size = str.size ();
        value.unspec.ptr = &str;
    }


    ~string_param ()
    {
        if (type & Ownership)
            delete_worker ();
    }


    std::size_t
    get_size () const
    {
        assert (type & Defined);

        if (value.unspec.size == (size_t_limits::max) ())
            get_size_worker ();

        return value.unspec.size;
    }


    char const *
    c_str () const
    {
        assert (! is_wide ());
        if (is_wide ())
            throw std::runtime_error ("Calling c_str() on wchar_t parameter.");

        if (is_basic_string ())
            return value.str.ptr->c_str ();
        else
            return value.array.ptr;
    }


    wchar_t const *
    c_wstr () const
    {
        assert (is_wide ());
        if (! is_wide ())
            throw std::runtime_error ("Calling c_wstr() on char parameter.");

        if (is_basic_string ())
            return value.wstr.ptr->c_str ();
        else
            return value.warray.ptr;
    }


    log4cplus::tchar const *
    c_tstr () const
    {
#if defined (UNICODE)
        return c_wstr ();
#else
        return c_str ();
#endif
    }


    log4cplus::tstring
    to_tstring () const
    {
        log4cplus::tstring str;
        this->to_tstring (str);
        return str;
    }


    void
    to_tstring (log4cplus::tstring & str) const
    {
        visit (totstring_visitor (str));
    }


    log4cplus::tstring const &
    get_tstring () const
    {
        assert (is_tstring ());
        return *value.tstr.ptr;
    }


    std::string const &
    get_string () const
    {
        assert (is_string ());
        return *value.str.ptr;
    }


    std::wstring const &
    get_wstring () const
    {
        assert (is_wstring ());
        return *value.wstr.ptr;
    }


    bool
    is_wide () const
    {
        assert (type & Defined);
        return (type & CharTypeMask) == WCharArray;
    }


    bool
    is_tchar () const
    {
        assert (type & Defined);
        return (type & CharTypeMask) == TCharArray;
    }


#if defined (LOG4CPLUS_HAVE_CPP0X)
    bool
    is_char16_t () const
    {
        assert (type & Defined);
        return (type & CharTypeMask) == Char16Array;
    }


    bool
    is_char32_t () const
    {
        assert (type & Defined);
        return (type & CharTypeMask) == Char32Array;
    }
#endif


    bool
    is_basic_string () const
    {
        assert (type & Defined);
        return (type & StringTypeMask) == StringTypeMask;
    }


    bool
    is_tstring () const
    {
        assert (type & Defined);
        return (type & (CharTypeMask | StringTypeMask))
            == (TCharArray | StringTypeMask);
    }


    bool
    is_string () const
    {
        assert (type & Defined);
        return (type & (CharTypeMask | StringTypeMask))
            == (CharArray | StringTypeMask);
    }


    bool
    is_wstring () const
    {
        assert (type & Defined);
        return (type & (CharTypeMask | StringTypeMask))
            == (WCharArray | StringTypeMask);
    }


#if defined (LOG4CPLUS_HAVE_CPP0X)
    bool
    is_u16string () const
    {
        assert (type & Defined);
        return (type & (CharTypeMask | StringTypeMask))
            == (Char16Array | StringTypeMask);
    }


    bool
    is_u32string () const
    {
        assert (type & Defined);
        return (type & (CharTypeMask | StringTypeMask))
            == (Char32Array | StringTypeMask);
    }
#endif


    template <typename Visitor>
    void
    visit (Visitor const & visitor) const
    {
        std::size_t const size = get_size ();
        switch (get_table_index ())
        {
        case CharArray >> CharTypeBit:
            visitor (value.array.ptr, size);
            break;

        case WCharArray >> CharTypeBit:
            visitor (value.warray.ptr, size);
            break;

        case Char16Array >> CharTypeBit:
            visitor (value.u16array.ptr, size);
            break;

        case Char32Array >> CharTypeBit:
            visitor (value.u32array.ptr, size);
            break;

        case StdString >> CharTypeBit:
            visitor (*value.str.ptr);
            break;

        case StdWString >> CharTypeBit:
            visitor (*value.wstr.ptr);
            break;

        case StdU16String >> CharTypeBit:
            visitor (*value.u16str.ptr);
            break;

        case StdU32String >> CharTypeBit:
            visitor (*value.u32str.ptr);
            break;

        default:
            assert (0 && "Unknown string_param type");
            throw std::runtime_error ("Unknown string_param type");
        }
    }


    void
    swap (string_param & other) throw ()
    {
        std::swap (type, other.type);
        std::swap (value, other.value);
    }


private:
    typedef std::char_traits<char> traits;
    typedef std::char_traits<wchar_t> wtraits;


    string_param (string_param const &);
    string_param & operator = (string_param const &);

    struct totstring_visitor;
    friend struct totstring_visitor;

    struct totstring_visitor
    {
        totstring_visitor (tstring & s)
            : str (s)
        { }


        void operator () (char const * cstr, std::size_t size) const
        {
#if defined (UNICODE)
            helpers::totstring (cstr).swap (str);
#else
            str.assign (cstr, size);
#endif
        }


        void operator () (wchar_t const * cstr, std::size_t size) const
        {
#if defined (UNICODE)
            str.assign (cstr, size);
#else
            (void)size;
            helpers::totstring (cstr).swap (str);
#endif
        }


        void operator () (std::string const & s) const
        {
#if defined (UNICODE)
            helpers::totstring (s).swap (str);
#else
            str = s;
#endif
        }


        void operator () (std::wstring const & s) const
        {
#if defined (UNICODE)
            str = s;
#else
            helpers::totstring (s).swap (str);
#endif
        }


#if defined (LOG4CPLUS_HAVE_CPP0X)
        void operator () (char16_t const * cstr, std::size_t) const
        {
            helpers::totstring (cstr).swap (str);
        }


        void operator () (std::u16string const & s) const
        {
            helpers::totstring (s).swap (str);
        }


        void operator () (char32_t const * cstr, std::size_t) const
        {
            helpers::totstring (cstr).swap (str);
        }


        void operator () (std::u32string const & s) const
        {
            helpers::totstring (s).swap (str);
        }
#endif


        tstring & str;
    };


    unsigned
    get_table_index () const
    {
        unsigned const index
            = (type & (CharTypeMask | StringTypeMask)) >> CharTypeBit;
        return index;
    }


    typedef void (string_param:: * const delete_func_type) () const;
    static LOG4CPLUS_EXPORT delete_func_type const
        delete_func[STRING_PARAM_TABLE_SIZE];


    template <typename CharType>
    void
    delete_char_array () const
    {
        delete[] static_cast<CharType const *>(value.unspec.ptr);
    }


    template <typename CharType>
    void
    delete_string () const
    {
        delete static_cast<std::basic_string<CharType> const *>(
            value.unspec.ptr);
    }


    LOG4CPLUS_EXPORT void delete_worker () const;


    template <typename CharType>
    std::size_t
    get_size_char_array () const
    {
        return std::char_traits<CharType>::length (
            static_cast<CharType const *>(value.unspec.ptr));
    }


    template <typename CharType>
    std::size_t
    get_size_string () const
    {
        return static_cast<std::basic_string<CharType> const *>(
            value.unspec.ptr)->size ();
    }


    typedef std::size_t (string_param:: * const size_func_type) () const;
    static LOG4CPLUS_EXPORT size_func_type const
        size_func[STRING_PARAM_TABLE_SIZE];


    void
    get_size_worker () const
    {
        value.unspec.size = (this->*size_func[get_table_index ()]) ();
    }


    template <typename CharType>
    void
    make_copy_char_array (string_param const & other)
    {
        size_t const other_len = other.get_size ();
        CharType * copy_buf = new CharType[other_len + 1];
        copy_buf[other_len] = 0;
        std::char_traits<CharType>::copy (copy_buf,
            static_cast<CharType const *>(other.value.unspec.ptr),
            other_len);
        value.unspec.ptr = copy_buf;
        value.unspec.size = other_len;
    }


    template <typename CharType>
    void
    make_copy_string (string_param const & other)
    {
        typedef std::basic_string<CharType> string_type;
        string_type * copy_str = new string_type (
            *static_cast<string_type const *>(other.value.unspec.ptr));
        value.unspec.ptr = copy_str;
        value.unspec.size = other.value.unspec.size;
    }


    typedef void (string_param:: * const make_copy_func_type) (string_param const &);
    static LOG4CPLUS_EXPORT make_copy_func_type const
        make_copy_func[STRING_PARAM_TABLE_SIZE];


    void
    make_copy (string_param const & other)
    {
         (this->*make_copy_func[get_table_index ()]) (other);
    }


    unsigned type;
    mutable param_value_type value;
};


} // namespace stringparam_impl


typedef stringparam_impl::string_param string_param;


} } // namespace log4cplus { namespace helpers {

#endif // LOG4CPLUS_HELPERS_STRINGPARAM_HXX
