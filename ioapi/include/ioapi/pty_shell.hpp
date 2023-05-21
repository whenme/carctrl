// SPDX-License-Identifier: GPL-2.0

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <string>

struct scc_stats_reborn
{
    uint32_t pipe_continuously_full;
    uint32_t slow_serial_remainig_iterations;
    uint32_t slow_serial_entry_count;
    uint32_t dropped_chars;
    uint32_t dropped_count;
    uint32_t sent_count;
    uint32_t flushed_count;
};

void pty_shell_init(uint32_t channel);

std::string readLink();

#ifdef __cplusplus
}
#endif
