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

#include "cocaine/essentials/logging_service.hpp"

#include "cocaine/logging.hpp"

#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>

using namespace cocaine;
using namespace cocaine::logging;
using namespace cocaine::service;

logging_t::logging_t(context_t& context,
                     const std::string& name,
                     const Json::Value& args):
    reactor_t(context, name, args),
    m_context(context)
{
    on<io::logging::emit>(boost::bind(&logging_t::on_emit, this, _1, _2, _3));
}

bool
logging_t::on_emit(int priority,
                   const std::string& source,
                   const std::string& message)
{
    log_map_t::iterator it = m_logs.find(source);

    if(it == m_logs.end()) {
        boost::tie(it, boost::tuples::ignore) = m_logs.emplace(
            source,
            new log_t(m_context, source)
        );
    }

    COCAINE_LOG(
        it->second,
        static_cast<logging::priorities>(priority),
        "%s",
        message
    );

    return true;
}
