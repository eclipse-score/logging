Explicit initialization of logging
==================================

.. dec_rec:: Explicit initialization of logging
   :id: dec_rec__log__explicit_init
   :status: proposed
   :context: See below.
   :decision: TBA

   Provide a library for explicit initialization of logging.

   **Context:**

   Currently there are multiple approaches to logging initialization.
   Rust frontend configuration is provided as code and requires explicit initialization before any logging starts.
   C++ configuration is lazily loaded on first use and based on a JSON file with path provided using environment variable.

   This approach discrepancy makes it difficult to manage logging in multi-language environment.
   Issue can be solved by providing an explicit initialization library.

   Such explicit logging initialization shall:

   - Initialize logging for Rust and C++ environments.
   - Handle configuration of all sub-components of the logging environment.
   - Provide interfaces to Rust and C++.

   Examples of use:

   - Initialize logging in Rust application:
     - Rust logging frontend
     - C++-based backend
     - Rust/C++ log bridge
   - Initialize logging in C++ application utilizing Rust-based libraries:
     - C++ logging
     - Rust logging frontend utilized by libraries
     - C++-based backend
     - Rust/C++ log bridge

   **Consequences**

   Such change will require design changes to middleware logging.

   In Rust it is a common practice to explicitly initialize logging before any logging starts.
   This is true to both `score_log` logging frontend and ubiquitous `log` crate that is a defacto standard for logging in Rust.

   Middleware logging library is currently initialized lazily on first use, using JSON config file.
   It is not possible to perform the initialization operation explicitly.

   **Justification for the Decision**

   Explicit logging initialization will simplify logging initialization in multi-language environments.
