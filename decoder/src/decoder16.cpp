// SPDX-FileCopyrightText: 2022 - 2023 Serdar Sayın <https://serdarsayin.com>
//
// SPDX-License-Identifier: MIT

#include "decoder/decoder.hpp"

namespace {
op decode16_quad0(uint16_t word);
op decode16_quad1(uint16_t word);
op decode16_quad2(uint16_t word);

op decode16_quad2_extract_other(uint16_t word);

constexpr uint16_t QUAD0 = 0b00;
constexpr uint16_t QUAD1 = 0b01;
constexpr uint16_t QUAD2 = 0b10;
constexpr uint16_t QUAD3 = 0b11;

} // namespace

// Quad0
enum class quad0 : uint16_t {
  C_ADDI4SPN  = 0b000,
  C_FLD       = 0b001,
  C_LW        = 0b010,
  C_FLW       = 0b011,
  C_RESERVED0 = 0b100,
  C_FSD       = 0b101,
  C_SW        = 0b110,
  C_FSW       = 0b111,
};

enum class quad1 : uint16_t {
  C_ADDI         = 0b000,
  C_JAL          = 0b001,
  C_LI           = 0b010,
  C_ADDI16SP_LUI = 0b011,
  C_ARITH        = 0b100,
  C_J            = 0b101,
  C_BEQZ         = 0b110,
  C_BNEZ         = 0b111
};

// instr[11:10]
enum class arith {
  C_SRLI = 0b00,
  C_SRAI = 0b01,
  C_ANDI = 0b10,
  // C_SUB, C_XOR, C_OR, C_AND
  ARITH_NO_IMM = 0b11
};

// instr[6:5]
// [12:12] should be 0 for 32-bit OPS, 1 for 64-bit OPS
enum class arith_no_imm {
  C_SUB = 0b00,
  C_XOR = 0b01,
  C_OR  = 0b10,
  C_AND = 0b11
};

enum class quad2 : uint16_t {
  C_SLLI  = 0b000,
  C_FLDSP = 0b001,
  C_LWSP  = 0b010,
  C_FLWSP = 0b011,
  // C_JR, C_MV, C_EBREAK, C_JALR, C_ADD
  OTHER   = 0b100,
  C_FSDSP = 0b101,
  C_SWSP  = 0b110,
  C_FSWSP = 0b111
};

op decode16(uint16_t word) {
  if (word == 0U) return make_illegal(true);
  switch (word & 0b11) {
  case QUAD0:
    return decode16_quad0(word);
  case QUAD1:
    return decode16_quad1(word);
  case QUAD2:
    return decode16_quad2(word);
  case QUAD3:
  default:
    return make_illegal(true);
  }
}

namespace {
op decode16_quad0(uint16_t word) {
  switch (static_cast<quad0>(offset(word, 13U, 15U))) {
  case quad0::C_ADDI4SPN: {
    rvc_addi4spn isn{word};
    return op{isn.imm, alu::_add, target::alu, isn.rd, 2, 0, true, false, true};
  }
  case quad0::C_LW: {
    rvc_lw isn{word};
    return op{isn.imm, masks::load::lw, target::load, isn.rd, isn.rs1, 0,
              true,    false,           true};
  }
  case quad0::C_SW: {
    rvc_sw isn{word};
    return op{isn.imm, masks::store::sw, target::store, 0,
              isn.rs1, isn.rs2,          true,          false,
              true};
  }
  default:
    return make_illegal(true);
  }
}

op decode16_quad1(uint16_t word) {
  switch (static_cast<quad1>(offset(word, 13U, 15U))) {
  case quad1::C_ADDI: {
    rvc_addi isn{word};
    return op{isn.imm, alu::_add, target::alu, isn.rdrs1, isn.rdrs1,
              0,       true,      false,       true};
  }
  case quad1::C_JAL: {
    rvc_jal isn{word};
    return op{isn.tgt, alu::_jal, target::alu, 1, 0, 0, true, true, true};
  }
  case quad1::C_LI: {
    rvc_li isn{word};
    return op{isn.imm, alu::_add, target::alu, isn.rdrs1, 0,
              0,       true,      false,       true};
  }

  case quad1::C_ADDI16SP_LUI: {
    // C_ADDI16SP
    if (offset(word, 7U, 11U) == 2) {
      rvc_addi16sp isn{word};
      return op{isn.imm, alu::_add, target::alu, isn.rdrs1, 2,
                2,       true,      false,       true};
      // C_LUI
    } else {
      rvc_lui isn{word};
      return op{isn.imm, alu::_add, target::alu, isn.rdrs1, 0,
                0,       true,      false,       true};
    }
  }
  case quad1::C_ARITH: {
    switch (static_cast<arith>(offset(word, 10U, 11U))) {
    case arith::C_SRLI: {
      rvc_srli isn{word};
      return op{isn.ofs, alu::_srl, target::alu, isn.rs1, isn.rs1,
                0,       true,      false,       true};
    }
    case arith::C_SRAI: {
      rvc_srai isn{word};
      return op{isn.ofs, alu::_sra, target::alu, isn.rs1, isn.rs1,
                0,       true,      false,       true};
    }
    case arith::C_ANDI: {
      rvc_andi isn{word};
      return op{isn.ofs, alu::_and, target::alu, isn.rs1, isn.rs1,
                0,       true,      false,       true};
    }
    case arith::ARITH_NO_IMM: {
      switch (static_cast<arith_no_imm>(offset(word, 5U, 6U))) {
      case arith_no_imm::C_SUB: {
        rvc_sub isn{word};
        return op{0,       alu::_sub, target::alu, isn.rdrs1, isn.rdrs1,
                  isn.rs2, false,     false,       true};
      }
      case arith_no_imm::C_XOR: {
        rvc_xor isn{word};
        return op{0,       alu::_xor, target::alu, isn.rdrs1, isn.rdrs1,
                  isn.rs2, false,     false,       true};
      }
      case arith_no_imm::C_OR: {
        rvc_or isn{word};
        return op{0,       alu::_or, target::alu, isn.rdrs1, isn.rdrs1,
                  isn.rs2, false,    false,       true};
      }
      case arith_no_imm::C_AND: {
        rvc_and isn{word};
        return op{0,       alu::_and, target::alu, isn.rdrs1, isn.rdrs1,
                  isn.rs2, false,     false,       true};
      }
      default:
        return make_illegal(true);
      }
    }
    default:
      return make_illegal(true);
    }
  }
  case quad1::C_J: {
    rvc_j isn{word};
    return op{isn.tgt, alu::_jal, target::alu, 0, 0, true, true, true};
  }
  case quad1::C_BEQZ: {
    rvc_beqz isn{word};
    return op{
        isn.ofs, masks::branch::beq, target::branch, 0, isn.rs1, 0, true, false,
        true};
  }
  case quad1::C_BNEZ: {
    rvc_bnez isn{word};
    return op{
        isn.ofs, masks::branch::bne, target::branch, 0, isn.rs1, 0, true, false,
        true};
  }
  default:
    return make_illegal(true);
  }
}

op decode16_quad2(uint16_t word) {
  switch (static_cast<quad2>(offset(word, 13U, 15U))) {
  case quad2::C_SLLI: {
    rvc_slli isn{word};
    return op{isn.imm,   alu::_sll, target::alu, isn.rdrs1, isn.rdrs1,
              isn.rdrs1, true,      false,       true};
  }
    // Floating point RVC for RV32
  case quad2::C_FLDSP:
  case quad2::C_FLWSP:
  case quad2::C_FSDSP:
  case quad2::C_FSWSP:
    return make_illegal(true);

  case quad2::C_LWSP: {
    rvc_lwsp isn{word};
    return op{isn.imm, masks::load::lw, target::load, isn.rdrs1, 2, 0,
              true,    false,           true};
  }
  case quad2::OTHER:
    return decode16_quad2_extract_other(word);

  case quad2::C_SWSP: {
    rvc_swsp isn{word};
    return op{isn.imm, masks::store::sw, target::store, 0,
              2,       isn.rs2,          true,          false,
              true};
  }
  default:
    return make_illegal(true);
  }
}

op decode16_quad2_extract_other(uint16_t word) {
  auto bit_12   = offset(word, 12U, 12U);
  auto bit_6_2  = offset(word, 2U, 6U);
  auto bit_11_7 = offset(word, 7U, 11U);
  if (bit_12 == 0b0) {
    if (bit_6_2 == 0U && bit_11_7 != 0U) /*jr*/ {
      rvc_jr isn{word};
      return op{0,       alu::_jalr, target::alu, 0,   isn.rdrs1,
                isn.rs2, false,      false,       true};
    } else if (bit_6_2 != 0U && bit_11_7 != 0U) /*mv*/ {
      rvc_mv isn{word};
      return op{0,       alu::_add, target::alu, isn.rdrs1, 0,
                isn.rs2, false,     false,       true};
    }
  } else if (bit_12 == 0b1) {
    if (bit_6_2 == 0 && bit_11_7 == 0) /* ebreak*/ {
      return op{0, {}, target::ebreak, 0, 0, 0, false, false, true};
    } else if (bit_6_2 == 0 && bit_11_7 != 0) /*jalr*/ {
      rvc_jalr isn{word};
      return op{0,       alu::_jalr, target::alu, 1,   isn.rdrs1,
                isn.rs2, false,      false,       true};
    }

    else if (bit_6_2 != 0 && bit_11_7 != 0) /*add*/ {
      rvc_add isn{word};
      return op{0,       alu::_add, target::alu, isn.rdrs1, isn.rdrs1,
                isn.rs2, false,     false,       true};
    }
  }

  return make_illegal(true);
}

} // namespace
