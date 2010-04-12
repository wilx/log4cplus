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

#include <iostream>
#include <iomanip>
#include <log4cplus/helpers/stringparam.hxx>


namespace log4cplus { namespace helpers { namespace stringparam_impl {


LOG4CPLUS_EXPORT make_copy_tag const string_param::makecopy = { };


void
string_param::delete_worker () const
{
    (this->*delete_func[get_table_index ()]) ();
}


string_param::size_func_type const string_param::size_func[4] = {
    &string_param::get_size_char_array<char>,
    &string_param::get_size_char_array<wchar_t>,
    &string_param::get_size_string<char>,
    &string_param::get_size_string<wchar_t>
};


string_param::delete_func_type const string_param::delete_func[4] = {
    &string_param::delete_char_array<char>,
    &string_param::delete_char_array<wchar_t>,
    &string_param::delete_string<char>,
    &string_param::delete_string<wchar_t>,
};


string_param::make_copy_func_type string_param::make_copy_func[4] = {
    &string_param::make_copy_char_array<char>,
    &string_param::make_copy_char_array<wchar_t>,
    &string_param::make_copy_string<char>,
    &string_param::make_copy_string<wchar_t>
};


namespace
{

struct visitor
{
    template <typename T>
    void operator () (T const * str, std::size_t) const
    {
        std::wcout << str << std::endl;
    }

    template <typename T>
    void operator () (std::basic_string<T> const & str) const
    {
        std::wcout << str.c_str () << std::endl;
    }
};



void
foo (string_param const & s)
{
    std::cout << "s.get_size(): " << s.get_size ();

    s.visit (visitor ());

    string_param copy_of_s (s, log4cplus::helpers::string_param::makecopy);
    copy_of_s.visit (visitor ());
}


int
test_stringparam ()
{
    char array[] = "test";
    char const carray[] = "test";
    char * pchar = &array[0];
    char const * pcchar = &array[0];
    char const * const cpcchar = &array[0];
    std::string str ("test");

    wchar_t warray[] = L"test";
    wchar_t const cwarray[] = L"test";
    wchar_t * pwchar = &warray[0];
    wchar_t const * pcwchar = &warray[0];
    wchar_t const * const cpcwchar = &warray[0];
    std::wstring wstr (L"test");

    foo ("test");
    foo (array);
    foo (carray);
    foo (str);
    foo (str.c_str ());
    foo (pchar);
    foo (pcchar);
    foo (cpcchar);

    foo (L"test");
    foo (warray);
    foo (cwarray);
    foo (wstr);
    foo (wstr.c_str ());
    foo (pwchar);
    foo (pcwchar);
    foo (cpcwchar);

    int dummy = 0;

    return 0;
}

} // namespace

} } } // namespace log4cplus { namespace helpers { namespace stringparam {
