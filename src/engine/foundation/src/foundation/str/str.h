#pragma once
#include <cstdint>
#include <string>
#include <unicode/unistr.h>

namespace enishi ::foundation {
    using UTF8 = std::string;

    enum class StringError : std::uint32_t {
        EncodeError,
        DecodeError,
    };

    enum class Encoding {
        ASCII,
        UTF8,
        SJIS,
    };

    class _WIP_UTF8 {
      private:
        std::string str;

      public:
    };

    class _WIP_StringWithEncoding {
      private:
        Encoding encoding;
        std::string str;

      public:
        _WIP_StringWithEncoding(_WIP_StringWithEncoding&&) = default;

        _WIP_StringWithEncoding& operator=(_WIP_StringWithEncoding&&) = default;

        _WIP_StringWithEncoding convert_encoding(const Encoding encoding);
    };
} // namespace enishi::foundation