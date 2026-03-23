// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

/// @file evaluation_main.cpp
/// @brief Entry point that exercises all planted issues so that every
///        cc_binary target compiles and the review tool can analyse them.

#include "score/evaluation/concurrency/concurrency_issues.h"
#include "score/evaluation/design/design_faults.h"
#include "score/evaluation/exception_handling/exception_error_handling.h"
#include "score/evaluation/idiomatic/non_idiomatic_cpp.h"
#include "score/evaluation/memory/memory_issues.h"
#include "score/evaluation/modern_cpp/modern_cpp_syntax.h"
#include "score/evaluation/optimization/code_optimization.h"
#include "score/evaluation/patterns/design_patterns.h"
#include "score/evaluation/security/security_issues.h"
#include "score/evaluation/safety/safe_cpp.h"
#include "score/evaluation/solid/solid_violations.h"
#include "score/evaluation/template_metaprogramming/template_metaprogramming_issues.h"

int main()
{
    // --- SOLID violations ---
    score::evaluation::LogManager lm;
    lm.LoadConfig("cfg.json");
    lm.RouteMessage("hello");

    score::evaluation::AreaCalculator calc;
    score::evaluation::Shape s{"circle", 3.0, 0.0};
    calc.Calculate(s);

    score::evaluation::HighLevelProcessor hlp;
    hlp.Process("data");

    // --- Memory issues ---
    score::evaluation::RawOwner ro(10);
    ro.At(0) = 42;

    score::evaluation::VectorPointerInvalidation vpi;
    vpi.DemonstrateUAF();

    score::evaluation::FillBuffer(2, 'X');

    score::evaluation::SelfAssignBroken sab(4);
    sab = sab;  // triggers self-assignment bug

    // --- Non-idiomatic C++ ---
    score::evaluation::ToDouble(5);
    score::evaluation::SumVector({1, 2, 3});
    score::evaluation::HasPrefix("hello world", "hello");
    score::evaluation::PrintLines({"line1", "line2"});

    // --- Modern C++ syntax ---
    score::evaluation::StringList sl{"alpha", "beta", "gamma_long_string"};
    score::evaluation::CountNonEmpty(sl);
    score::evaluation::FilterLong(sl);
    score::evaluation::MakeAdder(10)(5);
    score::evaluation::InitialiseSubsystem(0);  // return value ignored

    // --- Design patterns ---
    score::evaluation::BrokenSingleton::GetInstance()->DoWork();

    score::evaluation::WidgetFactory wf;
    score::evaluation::Widget* w = wf.Create("button");
    delete w;  // caller must remember to delete

    score::evaluation::BubbleSort bs;
    score::evaluation::Sorter sorter(&bs);
    std::vector<int> nums{3, 1, 4, 1, 5};
    sorter.Sort(nums);

    // --- Code optimisation ---
    score::evaluation::SumLargeVector({1, 2, 3, 4, 5});
    score::evaluation::IsError("ERROR");
    score::evaluation::JoinLines({"a", "b", "c"});
    score::evaluation::SortAndSlice({5, 3, 1, 4, 2});

    // --- Safe C++ ---
    std::vector<int> v{1, 2, 3};
    score::evaluation::IndexInRange(-1, v);    // signed/unsigned comparison
    score::evaluation::MultiplyUnchecked(100000, 100000);
    score::evaluation::TruncateToByte(300);
    score::evaluation::CountFlags(true, false, true);

    // --- Security issues ---
    score::evaluation::EvaluateSecuritySamples();

    // --- Design faults ---
    score::evaluation::Order order{1, 150.0, false, "Alice"};
    score::evaluation::OrderProcessor op;
    op.Apply10PercentDiscount(order);

    score::evaluation::CalculateScore(200);
    score::evaluation::ExportData("out.bin", true, false, true);

    // --- Concurrency issues ---
    score::evaluation::UnsafeCounter uc;
    uc.Increment();

    score::evaluation::SpuriousWakeupBug swb;
    swb.Signal();

    // --- Exception / error handling ---
    score::evaluation::CatchByValue();
    score::evaluation::LoadFile("nonexistent.txt");
    score::evaluation::InitDevice(0);  // return value intentionally ignored

    // --- Template metaprogramming issues ---
    score::evaluation::EvaluateTemplateMetaprogrammingSamples();

    return 0;
}
