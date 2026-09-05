#pragma once
#include "../result/result.h"
#include "str.h"
#include <unicode/ucnv.h>
#include <unicode/unistr.h>

namespace enishi ::foundation {
    Result<UTF8, StringError> sjis_to_utf8(const std::string& sjis);
} // namespace enishi::foundation