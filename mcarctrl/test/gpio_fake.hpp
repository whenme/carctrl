// SPDX-License-Identifier: GPL-2.0
#pragma once

/* Fail the next matching GPIO sysfs operation. Loaded via LD_PRELOAD. */
enum TestGpioFail {
    FailNone = 0,
    FailOpenExport,
    FailWriteExport,
    FailOpenUnexport,
    FailWriteUnexport,
    FailOpenDirection,
    FailWriteDirection,
    FailOpenEdge,
    FailWriteEdge,
    FailOpenValue,
    FailWriteValue,
    FailReadValue,
    FailLseek
};
