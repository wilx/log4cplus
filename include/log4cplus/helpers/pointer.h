// -*- C++ -*-
// Module:  Log4CPLUS
// File:    pointer.h
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright 2001-2017 Tad E. Smith
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

//
// Note: Some of this code uses ideas from "More Effective C++" by Scott
// Myers, Addison Wesley Longmain, Inc., (c) 1996, Chapter 29, pp. 183-213
//

/** @file */

#ifndef LOG4CPLUS_HELPERS_POINTERS_HEADER_
#define LOG4CPLUS_HELPERS_POINTERS_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/thread/syncprims.h>
#include <algorithm>
#include <cassert>
#include <atomic>
#include <memory>


namespace log4cplus {
    namespace helpers {

        /******************************************************************************
         *                       Class SharedObject (from pp. 204-205)                *
         ******************************************************************************/

        class LOG4CPLUS_EXPORT SharedObject
        {
        public:
            void addReference() const LOG4CPLUS_NOEXCEPT;
            void removeReference() const;

        protected:
          // Ctor
            SharedObject()
                : access_mutex()
                , count__(0)
            { }

            SharedObject(const SharedObject&)
                : access_mutex()
                , count__(0)
            { }

            SharedObject(SharedObject &&)
                : access_mutex()
                , count__(0)
            { }

          // Dtor
            virtual ~SharedObject();

          // Operators
            SharedObject& operator=(const SharedObject&) LOG4CPLUS_NOEXCEPT { return *this; }
            SharedObject& operator=(SharedObject &&) LOG4CPLUS_NOEXCEPT { return *this; }

        public:
            thread::Mutex access_mutex;

        private:
#if defined (LOG4CPLUS_SINGLE_THREADED)
            typedef unsigned count_type;
#else
            typedef std::atomic<unsigned> count_type;
#endif
            mutable count_type count__;
        };


        /******************************************************************************
         *           Template Class SharedObjectPtr (from pp. 203, 206)               *
         ******************************************************************************/
        template<class T>
        class SharedObjectPtr
        {
        public:
            // Ctor
            explicit
            SharedObjectPtr(T* realPtr = nullptr) LOG4CPLUS_NOEXCEPT
                : pointee(realPtr)
            {
                addref ();
            }

            SharedObjectPtr(const SharedObjectPtr& rhs) LOG4CPLUS_NOEXCEPT
                : pointee(rhs.get ())
            {
                addref ();
            }

            SharedObjectPtr(SharedObjectPtr && rhs) LOG4CPLUS_NOEXCEPT
                : pointee (rhs.pointee.exchange (nullptr, std::memory_order_consume))
            { }

            SharedObjectPtr & operator = (SharedObjectPtr && rhs) LOG4CPLUS_NOEXCEPT
            {
                if (LOG4CPLUS_UNLIKELY (this == &rhs))
                    return *this;

                T * const local = pointee.exchange (
                    rhs.pointee.exchange (nullptr, std::memory_order_consume),
                    std::memory_order_consume);

                if (LOG4CPLUS_LIKELY (local))
                    local->removeReference ();

                return *this;
            }

            // Dtor
            ~SharedObjectPtr()
            {
                T * const ptr = get ();
                if (LOG4CPLUS_LIKELY (ptr))
                    ptr->removeReference();
            }

            // Operators
            bool operator==(const SharedObjectPtr& rhs) const
            { return (get () == rhs.get ()); }
            bool operator!=(const SharedObjectPtr& rhs) const
            { return (get () != rhs.get ()); }
            bool operator==(const T* rhs) const { return (get () == rhs); }
            bool operator!=(const T* rhs) const { return (get () != rhs); }
            T* operator->() const { T * ptr = get (); assert (ptr); return ptr; }
            T& operator*() const { T * ptr = get (); assert (ptr); return *ptr; }

            SharedObjectPtr& operator=(const SharedObjectPtr& rhs)
            {
                if (this == &rhs)
                    return *this;

                return this->operator = (rhs.get ());
            }

            SharedObjectPtr& operator=(T* rhs)
            {
                if (LOG4CPLUS_LIKELY (rhs))
                    rhs->addReference ();

                T * const local = pointee.exchange (rhs, std::memory_order_consume);

                if (LOG4CPLUS_LIKELY (local))
                    local->removeReference ();

                return *this;
            }

          // Methods
            T* get() const
            {
                return pointee.load (std::memory_order_consume);
            }

            typedef T * (SharedObjectPtr:: * unspec_bool_type) () const;
            operator unspec_bool_type () const
            {
                return get () ? &SharedObjectPtr::get : 0;
            }

            bool operator ! () const
            {
                return ! get ();
            }

        private:
          // Methods
            void addref() const LOG4CPLUS_NOEXCEPT
            {
                T * const ptr = get ();
                if (LOG4CPLUS_LIKELY (ptr))
                    ptr->addReference();
            }

          // Data
            std::atomic<T*> pointee;
        };


        //! Boost `intrusive_ptr` helpers.
        //! @{
        inline
        void
        intrusive_ptr_add_ref (SharedObject const * so)
        {
            so->addReference();
        }

        inline
        void
        intrusive_ptr_release (SharedObject const * so)
        {
            so->removeReference();
        }
        //! @}

    } // end namespace helpers
} // end namespace log4cplus


#endif // LOG4CPLUS_HELPERS_POINTERS_HEADER_
