// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// This file is part of the AMD Render Pipeline Shaders SDK which is
// released under the AMD INTERNAL EVALUATION LICENSE.
//
// See file LICENSE.txt for full license details.

#include "rps/core/rps_result.h"
#include "core/rps_core.hpp"

#define RPS_DEFINE_RESULT_INFO( Name )  { #Name, Name }

const char* rpsResultGetName(RpsResult result)
{
    static constexpr struct {
        const char* name;
        RpsResult   value;
    } resultInfo[] = {
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INTERNAL_ERROR),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_RUNTIME_API_ERROR),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_NOT_SUPPORTED),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_TYPE_MISMATCH),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_UNSUPPORTED_MODULE_VERSION),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INVALID_PROGRAM),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_VALIDATION_FAILED),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_RANGE_OVERLAPPING),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INTEGER_OVERFLOW),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_NOT_IMPLEMENTED),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_KEY_DUPLICATED),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_KEY_NOT_FOUND),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INTEROP_DATA_LAYOUT_MISMATCH),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_COMMAND_ALREADY_FINAL),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INDEX_OUT_OF_BOUNDS),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_UNKNOWN_NODE),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_UNSUPPORTED_VERSION_TOO_NEW),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_UNSUPPORTED_VERSION_TOO_OLD),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INVALID_FILE_FORMAT),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_FILE_NOT_FOUND),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_OUT_OF_MEMORY),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INVALID_OPERATION),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INVALID_DATA),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_INVALID_ARGUMENTS),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_UNRECOGNIZED_COMMAND),
        RPS_DEFINE_RESULT_INFO(RPS_ERROR_UNSPECIFIED),
        RPS_DEFINE_RESULT_INFO(RPS_OK)
    };
    static_assert(RPS_COUNTOF(resultInfo) == RPS_RESULT_CODE_COUNT, "RpsResult name table needs update.");
    for (size_t i = 0; i < RPS_COUNTOF(resultInfo); i++) {
        if (resultInfo[i].value == result) {
            return resultInfo[i].name;
        }
    }
    return "<Unknown Error Code>";
}

#undef RPS_DEFINE_RESULT_INFO