/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_LOG_ENTRY_DESERIALIZATION_VISITOR_H_
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_LOG_ENTRY_DESERIALIZATION_VISITOR_H_

#include "score/mw/log/detail/common/log_entry_deserialize.h"
#include "static_reflection_with_serialization/serialization/for_logging.h"

namespace score
{
namespace common
{
namespace visitor
{

namespace dlt_server_logging = ::score::mw::log::detail::log_entry_deserialization;

//  Design rationale: It was decided to provide overload of the custom type i.e. SerializedVectorData instead of generic
//  solution due to simplicity of implementation. The goal of the change is to optimize passing data as pointer to
//  shared memory data instead of allocating and passing it through std::vector which happens when using default
//  deserialization. If there will be a need to use similar solution in other places in the project, it is advised to
//  implement generic solution handling any type pointed by user. In such case log entry deserialization should be
//  updated to use it.
template <typename A, typename S>
inline void deserialize(const vector_serialized<A, S>& serial,
                        deserializer_helper<A>& a,
                        dlt_server_logging::SerializedVectorData& t)
{
    using subsize_s_t = const subsize_serialized<A>;
    typename A::offset_t offset;
    deserialize(serial.offset, a, offset);
    if (offset == 0)
    {
        a.setZeroOffset();
        detail::clear(t);
        return;
    }
    const auto vector_size_location = a.template address<subsize_s_t>(offset);
    if (vector_size_location == nullptr)
    {
        // error condition already set by a.address()
        detail::clear(t);
        return;
    }
    typename A::subsize_t subsize;
    deserialize(*vector_size_location, a, subsize);
    const std::size_t n = subsize / sizeof(S);

    const auto vector_contents_offset = static_cast<typename A::offset_t>(offset + sizeof(subsize_s_t));
    const S* const vector_contents_address = a.template address<const S>(vector_contents_offset, n);
    if (vector_contents_address == nullptr)
    {
        detail::clear(t);
        return;
    }
    //  The cast is needed because it allows to realize main purpose of the deserialization i.e. change the form of the
    //  data from series of bytes to C++ structures. This is the crucial place where it is done.
    //  It is safe because the serialization method encodes the length of the payload.
    //  Serialized payload in this case is just a series of elements which have the size of one byte.
    const auto vector_data_address =
        // coverity[autosar_cpp14_m5_2_8_violation]
        static_cast<const std::uint8_t*>(static_cast<const void*>(vector_contents_address));
    t.data = score::cpp::span{vector_data_address, static_cast<std::uint32_t>(n)};
}

//  Instructs the serializer how to treat data inside of SerializedVectorData type
//  SerializedVectorData replaces std::vector in original LogEntry used for serialization and thus
//  vector_serialized_descriptor can be used to access byte stream
template <typename A,
          typename T,
          ::score::common::visitor::if_same_struct<dlt_server_logging::SerializedVectorData, T> = 0>
inline auto visit_as(serialized_visitor<A>& /*unused*/, const T& /*unused*/)
{
    return vector_serialized_descriptor<A, std::uint8_t>();
}

}  //  namespace visitor
}  //  namespace common
}  //  namespace score

#endif  //  SCORE_PAS_LOGGING_INCLUDE_DAEMON_LOG_ENTRY_DESERIALIZATION_VISITOR_H_
