# SPDX-FileCopyrightText: 2022 - 2023 Serdar Sayın <https://serdarsayin.com>
#
# SPDX-License-Identifier: MIT

add_library(riscv32-sim-common INTERFACE)
target_include_directories(riscv32-sim-common INTERFACE include)
target_link_libraries(riscv32-sim-common INTERFACE nlohmann_json::nlohmann_json)
