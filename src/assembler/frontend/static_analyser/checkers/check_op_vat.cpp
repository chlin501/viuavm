/*
 *  Copyright (C) 2018 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <viua/assembler/frontend/static_analyser.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_vat(Register_usage_profile& register_usage_profile,
                  Instruction const& instruction) -> void {
    auto result = get_operand<RegisterIndex>(instruction, 0);
    if (not result) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index or void");
    }

    auto source = get_operand<RegisterIndex>(instruction, 1);
    if (not source) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_use_of_register(register_usage_profile, *source);
    assert_type_of_register<viua::internals::ValueTypes::VECTOR>(
        register_usage_profile, *source);

    auto key = get_operand<RegisterIndex>(instruction, 2);
    if (not key) {
        throw invalid_syntax(instruction.operands.at(2)->tokens,
                             "invalid operand")
            .note("expected register index or void");
    }

    check_use_of_register(register_usage_profile, *key);
    assert_type_of_register<viua::internals::ValueTypes::INTEGER>(
        register_usage_profile, *key);

    auto val       = Register(*result);
    val.value_type = ValueTypes::POINTER;
    register_usage_profile.define(val, result->tokens.at(0));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
