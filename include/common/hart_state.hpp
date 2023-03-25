// SPDX-FileCopyrightText: 2022 - 2023 Serdar Sayın <https://serdarsayin.com>
//
// SPDX-License-Identifier: MIT

#ifndef COMMON_HART_STATE_HPP
#define COMMON_HART_STATE_HPP

#include "op.hpp"
#include "program_counter.hpp"

struct hart_state {
  op              dec;
  program_counter pc;
  uint32_t        instr;
  hart_state(uint32_t pc) : pc{pc} {}
};

#endif /* end of include guard: COMMON_HART_STATE_HPP */
