..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

Assumptions of Use
##################

.. aou_req:: No Private API Usage
   :id: aou_req__log__no_private_api
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid

   Undocumented or private API from mw::log SHALL NOT be used.

   Note_1: The public API of mw::log is contained in the ``mw::log`` namespace and is annotated
   with the documentation tag ``\public``. Everything else is considered part of the private API
   and shall not be used. Entities from any implementation ``details`` namespace SHALL NOT be
   used.

   Note_2: In the following example, the ``TextRecorder`` class is part of a detail namespace
   and SHALL NOT be used directly. It may be used indirectly but only though public API:

   .. code-block:: cpp

      namespace mw
      namespace log
      {
      namespace detail //
      {

      class TextRecorder : public Recorder // Private undocumented API SHALL NOT be used.
      {
          // ...
      };

      } // namespace detail

      /*
       * \public
       */
      LogStream LogInfo(); // Public API is OK to be used by Safety-Related Application Software.

      } // namespace log
      } // namespace mw

.. aou_req:: No Unsupported Context Usage
   :id: aou_req__log__no_unsupported_contexts
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid

   mw::log API SHALL NOT be used from unsupported contexts.

   (1) Threads: mw::log API documented and tagged ``\thread-safe`` shall be thread safe.

   (2) Signal handler: mw::log API SHALL NOT be called from signal handlers.

   (3) Interrupt handler: mw::log API SHALL NOT be called from interrupt handlers.

   Note_1: The majority of the mw::log API is thread-safe. Thread-safe API can be recognized by
   the ``\thread-safe`` tag in the documentation:

   .. code-block:: cpp

      class Logger
      {
          /// \brief Creates a LogStream to log messages of criticality `Fatal` (highest).
          ///
          /// \public
          /// \thread-safe
          ///
          /// \details Fatal shall be used on errors that cannot be recovered and will lead to an
          /// overall failure in the system. The message will be logged under the context that was
          /// provided on construction.
          ///
          /// \return LogStream which can be used stream verbose logging messages (will be flushed
          /// on destruction)
          log::LogStream LogFatal() const noexcept;
      };

   Note_2: LogStream classes are NOT thread-safe. For example, here is an example of what NOT to
   do with LogStream classes:

   .. code-block:: cpp

      auto log_stream = logger.LogInfo();

      std::thread t1([&](){
          // Do not access log_stream from another thread!
          log_stream << "test";
      });

      log_stream << "hello";

   Note_3: Refer to the README (master) for further details of the usage of the mw::log library.

.. aou_req:: Mitigate Unbounded Initialization
   :id: aou_req__log__mitigate_unbounded_init
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid

   Unbounded runtime behavior of mw::log initialization shall be mitigated or tolerated.

   Safety-Related Platform Software and Safety-Related Application Software shall mitigate or
   tolerate unbounded runtime behavior of the mw::log initialization.

   Note_1: Due to dynamic memory pre-allocation during the initialization phase of mw::log the
   runtime behavior of the initialization phase is unbounded and indeterminism.

   Note_2: Safety-Related Platform Software and Safety-Related Application Software shall
   initialize the library by sending any log message with arbitrary content, log level and
   context. This shall be done in an early uncritical phase of the program.

   .. code-block:: cpp

      void InitializeMwLog() {

          mw::log::LogInfo() << "Initializing"; // Do not initialize mw::log in a critical section of the program.

          // After initialization, the mw::log library has deterministic and upper-bounded runtime behavior.
      }

   Note_3: Safety-Related Platform Software and Safety-Related Application Software SHALL NOT
   initialize mw::log in a runtime critical section:

   .. code-block:: cpp

      void CalculateTrajectory(){
           if (SensorValueMissing()) {
              mw::log::LogError() << "Sensor Error"; // If mw::log has not been initialized before, the runtime behavior may be unbounded here.
              StartRecoveryMechanism():
          }
          // ...
      }


      int main() {
          // mw::log::LogInfo() << "Initialize"; // Uncomment this to ensure deterministic runtime behavior of mw::log in the remaining execution.
           while(/* */){
              CalculateTrajectory();
          }
      }

   Note_4: This requirement shall be verified for each assigned application. The whole dependency
   tree of ALL included libraries shall be also verified on this requirement in the application
   use case and by application developers. Application developer shall not make any assumptions
   of mw::log usage and initialization in the dependent libraries, but verify the mw::log
   usage based on available documentation and the source code.

   Note_5: For any library which is used there it needs to be ensured that the library provides
   its safety-related functionality and properties correctly as specified in the safety
   applications design, satisfying its allocated safety requirements.

.. aou_req:: No Static Storage Usage
   :id: aou_req__log__no_static_storage_use
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid

   mw::log SHALL NOT be used during the C++ static storage construction and destruction [1]_ [2]_.

   Safety-Related Platform Software and Safety-Related Application Software SHALL NOT use
   mw::log during C++ static storage initialization and destruction.

   Note_1: Safety-Related Platform Software and Safety-Related Application Software SHALL NOT
   use mw::log in construction or destruction of an object with static storage duration:

   .. code-block:: cpp

      struct MyClass {
          MyClass() {
              mw::log::LogInfo() << "MyClass()"; // mw::log SHALL NOT be used before entering main().
          }
          ~MyClass() {
              mw::log::LogInfo() << "MyClass()"; // mw::log SHALL NOT be used after exiting main().
          }
      };


      static MyClass singleton{};

   Note_2: Safety-Related Platform Software and Safety-Related Application Software SHALL NOT
   use mw::log before entering and after exiting main().

   .. [1] https://en.cppreference.com/w/cpp/language/initialization
   .. [2] https://eel.is/c++draft/basic.start#static

.. aou_req:: No Console Logging in Production
   :id: aou_req__log__no_console_in_production
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid

   Console Logging shall not be used in production.

.. aou_req:: No Safety Reliance on Log Output
   :id: aou_req__log__no_safety_reliance_on_log
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid

   Applications SHALL NOT rely on mw::log output for safety verification/qualification/case.

   Safety-Related Platform Software and Safety-Related Application Software SHALL NOT rely on
   the presence of any form of mw::log output for delivering their business logic. This shall
   hold even for logs sent out by the application itself.

   Note_1: Safety-Related Platform Software and Safety-Related Application Software behavior
   shall not depend on log messages:

   .. code-block:: cpp

      void Step() {
          // Do not rely on log messages!
          if ( ReceiveStopLogMessage() ) {

              ExecuteStopCommand();

          }
          // ...

   Note_2: This requirement shall be verified for each assigned application. The whole dependency
   tree of ALL included libraries shall be also verified on this requirement in the application
   use case and by application developers.
