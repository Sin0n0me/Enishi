#include "to_utf8.h"
namespace enishi ::foundation {
    constexpr const char* SOURCE_ENCODING = "Shift_JIS";

    Result<UTF8, StringError> sjis_to_utf8(const std::string& sjis) {
        UErrorCode status = U_ZERO_ERROR;

        const icu::UnicodeString unicode(
            sjis.c_str(), static_cast<int32_t>(sjis.size()), SOURCE_ENCODING);

        if (U_FAILURE(status)) {
            return Error(StringError::DecodeError, "Failed to decode Shift_JIS");
        }

        std::string utf8;
        unicode.toUTF8String(utf8);

        return utf8;
    }
} // namespace enishi::foundation