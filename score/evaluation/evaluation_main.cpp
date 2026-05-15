

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
    score::evaluation::LogManager lm;
    lm.LoadConfig("cfg.json");
    lm.RouteMessage("hello");

    score::evaluation::AreaCalculator calc;
    score::evaluation::Shape s{"circle", 3.0, 0.0};
    calc.Calculate(s);

    score::evaluation::HighLevelProcessor hlp;
    hlp.Process("data");

    score::evaluation::RawOwner ro(10);
    ro.At(0) = 42;

    score::evaluation::VectorPointerInvalidation vpi;
    vpi.DemonstrateUAF();

    score::evaluation::FillBuffer(2, 'X');

    score::evaluation::SelfAssignBroken sab(4);
    sab = sab;

    score::evaluation::ToDouble(5);
    score::evaluation::SumVector({1, 2, 3});
    score::evaluation::HasPrefix("hello world", "hello");
    score::evaluation::PrintLines({"line1", "line2"});

    score::evaluation::StringList sl{"alpha", "beta", "gamma_long_string"};
    score::evaluation::CountNonEmpty(sl);
    score::evaluation::FilterLong(sl);
    score::evaluation::MakeAdder(10)(5);
    score::evaluation::InitialiseSubsystem(0);

    score::evaluation::BrokenSingleton::GetInstance()->DoWork();

    score::evaluation::WidgetFactory wf;
    score::evaluation::Widget* w = wf.Create("button");
    delete w;

    score::evaluation::BubbleSort bs;
    score::evaluation::Sorter sorter(&bs);
    std::vector<int> nums{3, 1, 4, 1, 5};
    sorter.Sort(nums);

    score::evaluation::SumLargeVector({1, 2, 3, 4, 5});
    score::evaluation::IsError("ERROR");
    score::evaluation::JoinLines({"a", "b", "c"});
    score::evaluation::SortAndSlice({5, 3, 1, 4, 2});

    std::vector<int> v{1, 2, 3};
    score::evaluation::IndexInRange(-1, v);
    score::evaluation::MultiplyUnchecked(100000, 100000);
    score::evaluation::TruncateToByte(300);
    score::evaluation::CountFlags(true, false, true);

    score::evaluation::EvaluateSecuritySamples();

    score::evaluation::Order order{1, 150.0, false, "Alice"};
    score::evaluation::OrderProcessor op;
    op.Apply10PercentDiscount(order);

    score::evaluation::CalculateScore(200);
    score::evaluation::ExportData("out.bin", true, false, true);

    score::evaluation::UnsafeCounter uc;
    uc.Increment();

    score::evaluation::SpuriousWakeupBug swb;
    swb.Signal();

    score::evaluation::CatchByValue();
    score::evaluation::LoadFile("nonexistent.txt");
    score::evaluation::InitDevice(0);

    score::evaluation::EvaluateTemplateMetaprogrammingSamples();

    return 0;
}
