/*
    Copyright (c) 2011-2012 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2012 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/

#ifndef COCAINE_STREAM_API_HPP
#define COCAINE_STREAM_API_HPP

#include "cocaine/common.hpp"

namespace cocaine { namespace api {

struct stream_t {
    virtual
    ~stream_t() {
        // Empty.
    }
    
    virtual
    void
    push(const char * chunk,
         size_t size) = 0;

    virtual
    void
    error(error_code code,
          const std::string& message) = 0;

    virtual
    void
    close() = 0;
};

struct null_stream_t:
    public stream_t
{
    virtual
    void
    push(const char * chunk, size_t size) { }

    virtual
    void
    error(error_code code, const std::string& message) { }

    virtual
    void
    close() { }
};

}}

#endif
