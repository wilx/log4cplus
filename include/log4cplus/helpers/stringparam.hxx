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


#undef BARK
#if 0
#if defined (__GNUC__)
#define FUNC() __PRETTY_FUNCTION__

#else
#define FUNC() __FUNCTION__

#endif

#define BARK() do { \
    std::cout << FUNC() << ":" \
    << std::dec << __LINE__ << std::endl;\
 } while (0)

#else
#define BARK() do { } while (0)

#endif


namespace log4cplus { namespace helpers {


template <bool B, class T = void>
struct enable_if_c
{
    typedef T type;
};


template <class T>
struct enable_if_c<false, T>
{ };


template <class Cond, class T = void> 
struct enable_if
    : public enable_if_c<Cond::value, T>
{ };


struct true_type
{
    enum Value { value = true };
};


struct false_type
{
    enum Value { value = false };
};


template <typename T, typename U>
struct is_same
    : public false_type
{ };


template <typename T>
struct is_same<T, T>
    : public true_type
{ };


struct string_param
{
    typedef std::numeric_limits<std::size_t> size_t_limits;
    typedef std::char_traits<char> traits;
    typedef std::char_traits<wchar_t> wtraits;


    string_param ()
        : type (Undefined)
    { }


    template <typename T>
    string_param (T const & param,
        typename enable_if_c<is_same<char const *, T>::value
            || is_same<char *, T>::value, std::size_t>::type strsize
            = (size_t_limits::max) ())
        : type (CharArray | Defined)
    {
        BARK();

        value.array.size = strsize;
        value.array.ptr = param;
    }


    template <std::size_t N>
    string_param (char const (& param)[N])
        : type (CharArray | Defined)
    {
        BARK();

        value.array.size = N - 1;
        value.array.ptr = &param[0];
    }


    template <typename T>
    string_param (T const & str,
       typename enable_if<is_same<std::string, T>, T *>::type = 0)
        : type (String | Defined)
    {
        BARK();

        value.string.size = (size_t_limits::max) ();
        value.string.ptr = &str;
    }

    ///////////////////////////////////////////////////////////
    template <typename T>
    string_param (T const & param,
        typename enable_if_c<is_same<wchar_t const *, T>::value
            || is_same<wchar_t *, T>::value, std::size_t>::type strsize
            = (size_t_limits::max) ())
        : type (WCharArray | Defined)
    {
        BARK();

        value.warray.size = strsize;
        value.warray.ptr = param;
    }


    template <std::size_t N>
    string_param (wchar_t const (& param)[N])
        : type (WCharArray | Defined)
    {
        BARK();

        value.warray.size = N - 1;
        value.warray.ptr = &param[0];
    }


    template <typename T>
    string_param (T const & str, 
        typename enable_if<is_same<std::wstring, T>, T *>::type = 0)
        : type (WString | Defined)
    {
        BARK();

        value.wstring.size = (size_t_limits::max) ();
        value.wstring.ptr = &str;
    }


    LOG4CPLUS_EXPORT
    ~string_param ()
    {
        if (type & Owndership)
            delete_worker ();
    }


    void 
    delete_wchar_array () const
    {
        delete[] value.warray.ptr;
    }


    void
    delete_wstring () const
    {
        delete[] value.wstring.ptr;
    }


    void
    delete_char_array () const
    {
        delete[] value.array.ptr;
    }


    void
    delete_string () const
    {
        delete[] value.string.ptr;
    }


    static void (string_param:: * const delete_func[4]) () const;


    void
    delete_worker () const
    {
        (this->*delete_func[type >> CharTypeBit & 3]) ();
    }


    std::size_t
    get_size_wchar_array () const 
    {
        return wtraits::length (value.warray.ptr);
    }

    
    std::size_t
    get_size_wstring () const 
    {
        return value.wstring.ptr->size ();
    }


    std::size_t
    get_size_char_array () const 
    {
        return traits::length (value.array.ptr);
    }

        
    std::size_t
    get_size_string () const 
    {
        return value.string.ptr->size ();
    }


    std::size_t
    get_size () const
    {
        assert (type & Defined);

        if (value.size == (size_t_limits::max) ())
            get_size_worker ();

        return value.size;
    }


    static std::size_t (string_param:: * const size_func[4]) () const;


    void
    get_size_worker () const
    {
        value.size = (this->*size_func[type >> CharTypeBit & 3]) ();
    }


    char const *
    c_str () const
    {
        assert (! is_wide ());
        if (is_wide ())
            throw std::runtime_error ("Calling c_str() on wchar_t parameter.");

        if (is_string ())
            return value.string.ptr->c_str ();
        else
            return value.array.ptr;
    }


    wchar_t const *
    c_wstr () const
    {
        assert (is_wide ());
        if (! is_wide ())
            throw std::runtime_error ("Calling c_wstr() on char parameter.");

        if (is_string ())
            return value.wstring.ptr->c_str ();
        else
            return value.warray.ptr;
    }


    bool
    is_wide () const
    {
        assert (type & Defined);
        return (type & CharTypeMask) == CharTypeMask;
    }


    bool
    is_string () const
    {
        assert (type & Defined);
        return (type & StringMask) == StringMask;
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


    template <typename Visitor>
    void
    visit_string (Visitor const & visitor) const
    {
        if (is_wide ())
            visitor (*value.wstring.ptr, get_size ());
        else
            visitor (*value.string.ptr, get_size ());
    }


    template <typename Visitor>
    void
    visit (Visitor const & visitor) const
    {
        if (is_string ())
            visit_string (visitor);
        else
            visit_char_array (visitor);
    }


    void
    swap (string_param & other) throw ()
    {
        std::swap (type, other.type);
        std::swap (value, other.value);
    }

private:
    string_param (string_param const &);
    string_param & operator = (string_param const &);


    enum Flags
    {
        Undefined = 0x0000,
        Defined   = 0x0001,
        
        CharTypeBit  = 1,
        StringBit    = 2,
        OwnershipBit = 3,

        CharTypeMask = 1 << CharTypeBit,
        StringMask   = 1 << StringBit,

        CharArray  = 0 << StringBit | 0 << CharTypeBit,
        WCharArray = 0 << StringBit | 1 << CharTypeBit,
        String     = 1 << StringBit | 0 << CharTypeBit,
        WString    = 1 << StringBit | 1 << CharTypeBit,
        Owndership = 1 << OwnershipBit
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
        std::size_t size;
        char_array_value_type<char> array;
        char_array_value_type<wchar_t> warray;
        string_value_type<char> string;
        string_value_type<wchar_t> wstring;
    };


    mutable unsigned type;
    mutable param_value_type value;
};


} } // namespace log4cplus { namespace helpers {
