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

#ifndef SCORE_PAS_LOGGING_DATAROUTER_FEATURE_CONFIG_H
#define SCORE_PAS_LOGGING_DATAROUTER_FEATURE_CONFIG_H

// --- Conditional Compilation Feature Switch ---
#if defined(PERSISTENT_CONFIG_FEATURE_ENABLED)
#include "score/datarouter/src/persistency/ara_per_persistent_dictionary/ara_per_persistent_dictionary.h"
#include "score/datarouter/src/persistency/ara_per_persistent_dictionary/ara_per_persistent_dictionary_factory.h"
#else
#include "score/datarouter/src/persistency/stub_persistent_dictionary/stub_persistent_dictionary.h"
#include "score/datarouter/src/persistency/stub_persistent_dictionary/stub_persistent_dictionary_factory.h"
#endif

#if defined(NON_VERBOSE_DLT)
#include "score/datarouter/src/dlt/nonverbose_dlt_impl/nonverbose_dlt.h"
#else
#include "score/datarouter/src/dlt/nonverbose_dlt_stub/stub_nonverbose_dlt.h"
#endif

#if defined(DYNAMIC_CONFIGURATION_IN_DATAROUTER)
#include "dynamic_config_session_factory.h"
#include "unix_domain/unix_domain_server.h"
#else
#include "stub_config_session_factory.h"
#include "stub_session_handle.h"
#endif

#if defined(DLT_FILE_TRANSFER_FEATURE)
#include "score/datarouter/src/file_transfer/file_transfer_impl/filetransfer_stream.h"
#else
#include "score/datarouter/src/file_transfer/file_transfer_stub/file_transfer_stream_handler_stub.h"
#endif

#if defined(PERSISTENT_LOGGING)
#include "score/datarouter/src/persistent_logging/persistent_logging/sysedr_concrete_factory.h"
#else
#include "score/datarouter/src/persistent_logging/persistent_logging_stub/stub_sysedr_factory.h"
#endif

namespace score
{
namespace platform
{
namespace internal
{

#if defined(PERSISTENT_LOGGING)
using SysedrHandlerType = SysedrHandler;
using SysedrFactoryType = SysedrConcreteFactory;
#else
using SysedrHandlerType = StubSysedrHandler;
using SysedrFactoryType = StubSysedrFactory;
#endif

}  // namespace internal

namespace datarouter
{

// --- Conditional Compilation Feature Switch ---
#if defined(PERSISTENT_CONFIG_FEATURE_ENABLED)
using PersistentDictionaryFactoryType = AraPerPersistentDictionaryFactory;
inline constexpr bool kPersistentConfigFeatureEnabled = true;
#else
using PersistentDictionaryFactoryType = StubPersistentDictionaryFactory;
inline constexpr bool kPersistentConfigFeatureEnabled = false;
#endif

#if defined(NON_VERBOSE_DLT)
using DltNonverboseHandlerType = score::logging::dltserver::DltNonverboseHandler;
inline constexpr bool kNonVerboseDltEnabled = true;
#else
using DltNonverboseHandlerType = score::logging::dltserver::StubDltNonverboseHandler;
inline constexpr bool kNonVerboseDltEnabled = false;
#endif

#if defined(DYNAMIC_CONFIGURATION_IN_DATAROUTER)
using DynamicConfigurationHandlerFactoryType = score::logging::daemon::DynamicConfigSessionFactory;
using ConfigSessionHandleType = score::platform::internal::UnixDomainServer::SessionHandle;
#else
using DynamicConfigurationHandlerFactoryType = score::logging::daemon::StubConfigSessionFactory;
using ConfigSessionHandleType = score::logging::daemon::StubSessionHandle;
#endif

#if defined(DLT_FILE_TRANSFER_FEATURE)
using FileTransferStreamHandlerType = score::logging::dltserver::FileTransferStreamHandler;
#else
using FileTransferStreamHandlerType = score::logging::dltserver::StubFileTransferStreamHandler;
#endif

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_DATAROUTER_FEATURE_CONFIG_H
