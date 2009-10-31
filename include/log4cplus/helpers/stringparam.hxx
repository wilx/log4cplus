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

#include <string>
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cwchar>
#include <cassert>
#include <log4cplus/config.hxx>
#include <log4cplus/tstring.h>


namespace log4cplus { namespace helpers {


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


class string_param
{
    struct sfinae_filler
    { };


public:
    typedef std::numeric_limits<std::size_t> size_t_limits;


    string_param ()
        : type (Undefined)
    { }


    template <typename T>
    string_param (T const & param,
        typename enable_if_c<is_same<char const *, T>::value
            || is_same<char *, T>::value
            || is_same<wchar_t const *, T>::value
            || is_same<wchar_t *, T>::value,
            std::size_t>::type strsize
            = (size_t_limits::max) ())
        : type (param_traits<T>::type_value | Defined)
    {
        value.unspec.size = strsize;
        value.unspec.ptr = param;
    }


    template <std::size_t N, typename T>
    string_param (T const (& param)[N],
        typename enable_if_c<is_same<char, T>::value
            || is_same<wchar_t, T>::value, sfinae_filler>::type * = 0)
        : type (param_traits<T *>::type_value | Defined)
    {
        value.unspec.size = N - 1;
        value.unspec.ptr = &param[0];
    }


    template <typename T>
    string_param (std::basic_string<T> const & str,
        typename enable_if_c<is_same<char, T>::value
            || is_same<wchar_t, T>::value, sfinae_filler>::type * = 0)
            : type (param_traits<T *>::type_value | Defined)
    {
        value.unspec.size = str.size ();
        value.unspec.ptr = str.c_str ();
    }


    ~string_param ()
    {
        if (type & Owndership)
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

        return value.array.ptr;
    }


    wchar_t const *
    c_wstr () const
    {
        assert (is_wide ());
        if (! is_wide ())
            throw std::runtime_error ("Calling c_wstr() on char parameter.");

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
    totstring () const
    {
        log4cplus::tstring str;
        this->totstring (str);
        return str;
    }


    void
    totstring (log4cplus::tstring & str) const
    {
        visit (totstring_visitor (str));
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


    template <typename Visitor>
    void
    visit (Visitor const & visitor) const
    {
        visit_char_array (visitor);
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


    enum Flags
    {
        Undefined = 0x0000,
        Defined   = 0x0001,
        
        CharTypeBit  = 1,
        OwnershipBit = 3,

        CharTypeMask = 1 << CharTypeBit,

        CharArray  = 0 << CharTypeBit,
        WCharArray = 1 << CharTypeBit,
#if defined (UNICODE)
        TCharArray = WCharArray,
#else
        TCharArray = CharArray,
#endif

        Owndership = 1 << OwnershipBit
    };


    template <typename T>
    struct param_traits;

#define LOG4CPLUS_DEF_PARAM_TRAITS(T, VAL)       \
    friend struct param_traits<T>;               \
    template <>                                  \
    struct param_traits<T>                       \
    {                                            \
        enum TypeValue { type_value = (VAL) };   \
    }

    LOG4CPLUS_DEF_PARAM_TRAITS (char *, CharArray);
    LOG4CPLUS_DEF_PARAM_TRAITS (char const *, CharArray);
    LOG4CPLUS_DEF_PARAM_TRAITS (wchar_t *, WCharArray);
    LOG4CPLUS_DEF_PARAM_TRAITS (wchar_t const *, WCharArray);

#undef LOG4CPLUS_DEF_PARAM_TRAITS


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
            str = helpers::totstring (cstr);
#else
            str.assign (cstr, size);
#endif
        }


        void operator () (wchar_t const * cstr, std::size_t size) const
        {
#if defined (UNICODE)
                str.assign (cstr, size);
#else
                str = helpers::totstring (cstr);
#endif
        }


        tstring & str;
    };


    void 
    delete_wchar_array () const
    {
        delete[] value.warray.ptr;
    }


    void
    delete_char_array () const
    {
        delete[] value.array.ptr;
    }


    static LOG4CPLUS_EXPORT void (string_param:: * const delete_func[2]) () const;


    void
    delete_worker () const
    {
        (this->*delete_func[type >> CharTypeBit & 1]) ();
    }


    std::size_t
    get_size_wchar_array () const 
    {
        return wtraits::length (value.warray.ptr);
    }


    std::size_t
    get_size_char_array () const 
    {
        return traits::length (value.array.ptr);
    }


    static LOG4CPLUS_EXPORT std::size_t (string_param:: * const size_func[2]) () const;


    void
    get_size_worker () const
    {
        value.unspec.size = (this->*size_func[type >> CharTypeBit & 1]) ();
    }


    template <typename Visitor>
    void
    visit_char_array (Visitor const & visitor) const
    {
        if (is_wide ())
            visitor (value.warray.ptr, get_size ());
        else
            visitor (value.array.ptr, get_size ());           
    }


    template <typename T>
    struct char_array_value_type
    {
        typedef T char_type;

        std::size_t size;
        char_type const * ptr;
    };


    union param_value_type
    {
        char_array_value_type<void> unspec;
        char_array_value_type<char> array;
        char_array_value_type<wchar_t> warray;
    };


    mutable unsigned type;
    mutable param_value_type value;
};


} } // namespace log4cplus { namespace helpers {
