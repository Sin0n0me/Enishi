#include "runtime_tests.h"
#include <test_support/test_runner.h>
#include <foundation/algorithm/resolve_dependencies.h>
#include <foundation/text/config_line_reader.h>
#include <foundation/thread/single_thread_executor.h>
#include <future>
#include <latch>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace enishi::tests {
    void dependency_tests() {
        using Node = foundation::DependencyDescription;
        const auto empty = foundation::resolve_dependencies({});
        check(empty.is_ok() && empty.unwrap().empty(), "empty graph succeeds");
        const std::vector<Node> diamond{{3, {{1, 2, 2}}}, {2, {{0}}}, {1, {{0}}}, {0, {}}};
        const auto sorted = foundation::resolve_dependencies(diamond);
        const std::vector<std::size_t> expected{3, 1, 2, 0};
        check(sorted.is_ok() && sorted.unwrap() == expected,
            "dependencies precede users, duplicate edges count once, ties retain input order");
        const auto optional = foundation::resolve_dependencies({{0, {{99}}}});
        check(optional.is_ok() && optional.unwrap() == std::vector<std::size_t>{0},
            "absent optional dependencies are ignored");
        const auto duplicate = foundation::resolve_dependencies({{1, {}}, {1, {}}});
        check(duplicate.is_err() && duplicate.unwrap_err().get_error() ==
                                        foundation::ResolveDependenciesError::DuplicateNode,
            "duplicate graph node is reported");
        for (const std::vector<Node>& graph :
            std::vector<std::vector<Node>>{{{1, {{1}}}}, {{1, {{2}}}, {2, {{1}}}, {3, {}}}}) {
            const auto cycle = foundation::resolve_dependencies(graph);
            check(cycle.is_err() && cycle.unwrap_err().get_error() ==
                                        foundation::ResolveDependenciesError::CircularDependency,
                "self and disconnected cycles fail instead of returning a partial order");
        }
    }

    void config_stream_tests() {
        for (const std::string newline : {"\n", "\r\n"}) {
            std::istringstream source("\xEF\xBB\xBF# comment" + newline + " \t" + newline +
                                      "alpha 42 # trailing" + newline + "beta 7");
            foundation::ConfigLineReader reader(source);
            std::istringstream fields;
            std::string key;
            int value = 0;
            check(reader.next(fields) && reader.line_number() == 3, "BOM and blank lines skipped");
            fields >> key >> value;
            check(key == "alpha" && value == 42 && !foundation::has_config_trailing_fields(fields),
                "LF and CRLF have the same fields and inline comment behavior");
            fields.setstate(std::ios::failbit);
            check(reader.next(fields), "previous field parse errors do not poison the next line");
            fields >> key >> value;
            check(key == "beta" && value == 7 && !foundation::has_config_trailing_fields(fields),
                "final line without newline is parsed");
            check(!reader.next(fields) && !reader.read_failed(), "EOF is not an I/O error");
        }
        std::istringstream broken;
        broken.setstate(std::ios::badbit);
        foundation::ConfigLineReader reader(broken);
        std::istringstream fields;
        check(!reader.next(fields) && reader.read_failed(),
            "stream failure is distinguished from EOF");
        fields.str("unexpected");
        check(foundation::has_config_trailing_fields(fields), "extra fields are rejected");
    }

    void executor_tests() {
        constexpr std::size_t TASK_COUNT = 128;
        constexpr std::size_t PRODUCER_COUNT = 4;
        std::vector<std::size_t> observed;
        std::latch entered(1);
        std::latch release(1);
        {
            foundation::SingleThreadExecutor executor;
            executor.submit([&] {
                entered.count_down();
                release.wait();
            });
            entered.wait();
            for (std::size_t index = 0; index < TASK_COUNT; ++index) {
                executor.submit([&, index] { observed.push_back(index); });
            }
            check(
                executor.pending_task_count() == TASK_COUNT, "pending count excludes running task");
            release.count_down();
        }
        std::vector<std::size_t> expected(TASK_COUNT);
        std::iota(expected.begin(), expected.end(), 0);
        check(observed == expected, "destruction drains queued work in FIFO order");
        std::vector<std::size_t> counts(PRODUCER_COUNT);
        {
            foundation::SingleThreadExecutor executor;
            executor.submit([] { throw std::runtime_error("test task failure"); });
            std::vector<std::thread> producers;
            for (std::size_t producer = 0; producer < PRODUCER_COUNT; ++producer) {
                producers.emplace_back([&, producer] {
                    for (std::size_t index = 0; index < TASK_COUNT; ++index) {
                        executor.submit([&, producer] { ++counts[producer]; });
                    }
                });
            }
            for (auto& producer : producers) {
                producer.join();
            }
        }
        for (const auto count : counts) {
            check(count == TASK_COUNT,
                "concurrent submissions execute exactly once after a task throws");
        }
        for (std::size_t attempt = 0; attempt < TASK_COUNT; ++attempt) {
            bool ran = false;
            {
                foundation::SingleThreadExecutor executor;
                executor.submit([&] { ran = true; });
            }
            check(ran, "immediate destruction does not drop work before worker starts");
        }
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("dependency_tests", enishi::tests::dependency_tests);
    enishi::tests::run_test("config_stream_tests", enishi::tests::config_stream_tests);
    enishi::tests::run_test("executor_tests", enishi::tests::executor_tests);
    return EXIT_SUCCESS;
}
