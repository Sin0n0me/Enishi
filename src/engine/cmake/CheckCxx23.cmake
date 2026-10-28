include_guard(GLOBAL)

function(enishi_check_cxx23)
    include(CheckCXXSourceCompiles)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    check_cxx_source_compiles([=[
        #include <expected>
        #include <format>
        #include <ranges>
        #include <string>
        int main() {
            std::expected<int, int> value{1};
            auto joined = std::views::single(std::string{"test"}) | std::views::join_with(',');
            auto text = joined | std::ranges::to<std::string>();
            return std::format("{}", *value).empty() || text.empty();
        }
    ]=] ENISHI_HAS_REQUIRED_CXX23)
    if(NOT ENISHI_HAS_REQUIRED_CXX23)
        message(FATAL_ERROR
            "Enishi requires C++23 std::expected, std::format, ranges::to and views::join_with. "
            "Use a compatible compiler and standard library together (for example Clang 19+ "
            "with libstdc++ 14+, or recent LLVM libc++ on macOS).")
    endif()
endfunction()
