// Module:  Log4CPLUS
// File:    tstring.h
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

/** @file */

#ifndef LOG4CPLUS_TSTRING_HEADER_
#define LOG4CPLUS_TSTRING_HEADER_

#include <log4cplus/config.hxx>
#include <string>

#ifdef UNICODE
#  ifdef LOG4CPLUS_WORKING_LOCALE
#    include <locale>
#  endif // LOG4CPLUS_WORKING_LOCALE
#  define LOG4CPLUS_TEXT2(STRING) L##STRING
#else
#  define LOG4CPLUS_TEXT2(STRING) STRING
#endif // UNICODE
#define LOG4CPLUS_TEXT(STRING) LOG4CPLUS_TEXT2(STRING)


namespace log4cplus
{


#ifdef UNICODE
    typedef wchar_t tchar;
#else
    typedef char tchar;
#endif


    typedef std::basic_string<tchar> tstring;


    namespace helpers {
#ifdef LOG4CPLUS_WORKING_LOCALE
        LOG4CPLUS_EXPORT std::string tostring(const std::wstring&,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::string tostring (wchar_t const *,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::wstring towstring(const std::string&,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::wstring towstring(char const *,
            std::locale const & = std::locale ());

#if defined (LOG4CPLUS_HAVE_CPP0X)
        LOG4CPLUS_EXPORT std::string tostring(const std::u16string&,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::string tostring (char16_t const *,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::string tostring(const std::u32string&,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::string tostring (char32_t const *,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::wstring towstring(const std::u16string&,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::wstring towstring (char16_t const *,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::wstring towstring(const std::u32string&,
            std::locale const & = std::locale ());

        LOG4CPLUS_EXPORT std::wstring towstring (char32_t const *,
            std::locale const & = std::locale ());
#endif

#else // LOG4CPLUS_WORKING_LOCALE
        LOG4CPLUS_EXPORT std::string tostring(const std::wstring&);
        LOG4CPLUS_EXPORT std::string tostring(wchar_t const *);
        LOG4CPLUS_EXPORT std::wstring towstring(const std::string&);
        LOG4CPLUS_EXPORT std::wstring towstring(char const *);
#endif // LOG4CPLUS_WORKING_LOCALE


#if defined (UNICODE)
        inline tstring totstring (std::wstring const & str)
        {
            return str;
        }

        inline tstring totstring (wchar_t const * str)
        {
            return str;
        }

        inline tstring totstring (std::string const & str)
        {
            return helpers::towstring (str);
        }

        inline tstring totstring (char const * str)
        {
            return helpers::towstring (str);
        }

#if defined (LOG4CPLUS_HAVE_CPP0X)
        inline tstring totstring (std::u16string const & str)
        {
            return helpers::towstring (str);
        }

        inline tstring totstring (char16_t const * str)
        {
            return helpers::towstring (str);
        }

        inline tstring totstring (std::u32string const & str)
        {
            return helpers::towstring (str);
        }

        inline tstring totstring (char32_t const * str)
        {
            return helpers::towstring (str);
        }
#endif

#define LOG4CPLUS_C_STR_TO_TSTRING(STRING) (log4cplus::helpers::towstring(STRING))
#define LOG4CPLUS_STRING_TO_TSTRING(STRING) (log4cplus::helpers::towstring(STRING))
#define LOG4CPLUS_TSTRING_TO_STRING(STRING) (log4cplus::helpers::tostring(STRING))

#else // UNICODE
        inline tstring totstring (std::wstring const & str)
        {
            return helpers::tostring (str);
        }

        inline tstring totstring (wchar_t const * str)
        {
            return helpers::tostring (str);
        }

        inline tstring totstring (std::string const & str)
        {
            return str;
        }

        inline tstring totstring (char const * str)
        {
            return str;
        }

#if defined (LOG4CPLUS_HAVE_CPP0X)
        inline tstring totstring (std::u16string const & str)
        {
            return helpers::tostring (str);
        }

        inline tstring totstring (char16_t const * str)
        {
            return helpers::tostring (str);
        }

        inline tstring totstring (std::u32string const & str)
        {
            return helpers::tostring (str);
        }

        inline tstring totstring (char32_t const * str)
        {
            return helpers::tostring (str);
        }
#endif

#define LOG4CPLUS_C_STR_TO_TSTRING(STRING) (std::string(STRING))
#define LOG4CPLUS_STRING_TO_TSTRING(STRING) (STRING)
#define LOG4CPLUS_TSTRING_TO_STRING(STRING) (STRING)

#endif // UNICODE

    } // namespace helpers
} // namespace log4cplus


#endif // LOG4CPLUS_TSTRING_HEADER_
