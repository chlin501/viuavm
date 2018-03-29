/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <memory>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
using namespace std;

using viua::bytecode::decoder::operands::fetch_and_advance_addr;
using viua::bytecode::decoder::operands::fetch_optional_and_advance_addr;
using viua::bytecode::decoder::operands::fetch_register;
using viua::bytecode::decoder::operands::fetch_register_index;
using Register_index = viua::internals::types::register_index;

viua::internals::types::byte* viua::process::Process::opframe(
    viua::internals::types::byte* addr) {
    /** Create new frame for function calls.
     */
    auto const arguments = fetch_and_advance_addr<Register_index>(
        fetch_register_index, addr, this);
    auto const local_registers = fetch_and_advance_addr<Register_index>(
        fetch_register_index, addr, this);

    request_new_frame(arguments, local_registers);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opparam(
    viua::internals::types::byte* addr) {
    /** Run param instruction.
     */
    auto const parameter_no_operand_index =
        fetch_and_advance_addr<Register_index>(
            fetch_register_index, addr, this);

    auto const source = fetch_and_advance_addr<viua::types::Value*>(
        viua::bytecode::decoder::operands::fetch_object, addr, this);

    if (parameter_no_operand_index >= stack->frame_new->arguments->size()) {
        throw make_unique<viua::types::Exception>(
            "parameter register index out of bounds (greater than arguments "
            "set "
            "size) while adding parameter");
    }
    stack->frame_new->arguments->set(parameter_no_operand_index,
                                     source->copy());
    stack->frame_new->arguments->clear(parameter_no_operand_index);

    return addr;
}

viua::internals::types::byte* viua::process::Process::oppamv(
    viua::internals::types::byte* addr) {
    /** Run pamv instruction.
     */
    auto const parameter_no_operand_index =
        fetch_and_advance_addr<Register_index>(
            fetch_register_index, addr, this);
    auto const source = fetch_and_advance_addr<viua::kernel::Register*>(
        fetch_register, addr, this);

    if (parameter_no_operand_index >= stack->frame_new->arguments->size()) {
        throw make_unique<viua::types::Exception>(
            "parameter register index out of bounds (greater than arguments "
            "set "
            "size) while adding parameter");
    }
    stack->frame_new->arguments->set(parameter_no_operand_index,
                                     source->give());
    stack->frame_new->arguments->clear(parameter_no_operand_index);
    stack->frame_new->arguments->flag(parameter_no_operand_index, MOVED);

    return addr;
}

viua::internals::types::byte* viua::process::Process::oparg(
    viua::internals::types::byte* addr) {
    /** Run arg instruction.
     */
    auto const target =
        fetch_optional_and_advance_addr<viua::kernel::Register*>(
            fetch_register, addr, this);
    auto const parameter_no_operand_index =
        fetch_and_advance_addr<Register_index>(
            fetch_register_index, addr, this);

    if (parameter_no_operand_index >= stack->back()->arguments->size()) {
        auto oss = ostringstream{};
        oss << "invalid read: read from argument register out of bounds: "
            << parameter_no_operand_index;
        throw make_unique<viua::types::Exception>(oss.str());
    }

    unique_ptr<viua::types::Value> argument;

    if (stack->back()->arguments->isflagged(parameter_no_operand_index,
                                            MOVED)) {
        argument = stack->back()->arguments->pop(parameter_no_operand_index);
    } else {
        argument =
            stack->back()->arguments->get(parameter_no_operand_index)->copy();
    }

    if (target) {
        *target.value() = std::move(argument);
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opargc(
    viua::internals::types::byte* addr) {
    auto const target = fetch_and_advance_addr<viua::kernel::Register*>(
        fetch_register, addr, this);

    *target = make_unique<viua::types::Integer>(
        static_cast<int>(stack->back()->arguments->size()));

    return addr;
}

viua::internals::types::byte* viua::process::Process::opcall(
    viua::internals::types::byte* addr) {
    auto const return_register =
        fetch_optional_and_advance_addr<viua::kernel::Register*>(
            fetch_register, addr, this);

    auto call_name = string{};
    auto ot        = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn = fetch_and_advance_addr<viua::types::Function*>(
            viua::bytecode::decoder::operands::fetch_object_of<
                viua::types::Function>,
            addr,
            this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->frame_new->set_local_register_set(
                static_cast<viua::types::Closure*>(fn)->rs(), false);
        }
    } else {
        call_name = fetch_and_advance_addr<std::string>(
            viua::bytecode::decoder::operands::fetch_atom, addr, this);
    }

    auto const is_native         = scheduler->is_native_function(call_name);
    auto const is_foreign        = scheduler->is_foreign_function(call_name);
    auto const is_foreign_method = scheduler->is_foreign_method(call_name);

    if (not(is_native or is_foreign or is_foreign_method)) {
        throw make_unique<viua::types::Exception>("call to undefined function: "
                                                  + call_name);
    }

    if (is_foreign_method) {
        if (stack->frame_new == nullptr) {
            throw make_unique<viua::types::Exception>(
                "cannot call foreign method without a frame");
        }
        if (stack->frame_new->arguments->size() == 0) {
            throw make_unique<viua::types::Exception>(
                "cannot call foreign method using empty frame");
        }
        if (stack->frame_new->arguments->at(0) == nullptr) {
            throw make_unique<viua::types::Exception>(
                "frame must have at least one argument when used to call a "
                "foreign method");
        }
        auto obj = stack->frame_new->arguments->at(0);
        return call_foreign_method(
            addr, obj, call_name, return_register.value_or(nullptr), call_name);
    }

    auto const caller = (is_native ? &viua::process::Process::call_native
                                   : &viua::process::Process::call_foreign);
    return (this->*caller)(
        addr, call_name, return_register.value_or(nullptr), "");
}

viua::internals::types::byte* viua::process::Process::optailcall(
    viua::internals::types::byte* addr) {
    if (stack->state_of() == viua::process::Stack::STATE::RUNNING) {
        stack->register_deferred_calls();
        stack->state_of(
            viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP);

        if (not stacks_order.empty()) {
            stack = stacks_order.top();
            stacks_order.pop();
            return (addr - 1);
        }
    }

    if (stack->state_of()
        != viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
        throw make_unique<viua::types::Exception>(
            "stack left in an invalid state");
    }

    stack->state_of(viua::process::Stack::STATE::RUNNING);

    auto call_name = string{};
    auto ot        = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn = fetch_and_advance_addr<viua::types::Function*>(
            viua::bytecode::decoder::operands::fetch_object_of<
                viua::types::Function>,
            addr,
            this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->back()->local_register_set.reset(
                static_cast<viua::types::Closure*>(fn)->give());
        }
    } else {
        call_name = fetch_and_advance_addr<decltype(call_name)>(
            viua::bytecode::decoder::operands::fetch_atom, addr, this);
    }

    auto const is_native         = scheduler->is_native_function(call_name);
    auto const is_foreign        = scheduler->is_foreign_function(call_name);
    auto const is_foreign_method = scheduler->is_foreign_method(call_name);

    if (not(is_native or is_foreign or is_foreign_method)) {
        throw make_unique<viua::types::Exception>(
            "tail call to undefined function: " + call_name);
    }
    // FIXME: make to possible to tail call foreign functions and methods
    if (not is_native) {
        throw make_unique<viua::types::Exception>(
            "tail call to non-native function: " + call_name);
    }

    // FIXME tailcalled functions should not inherit local register set of the
    // frame they replace
    stack->back()->arguments = std::move(stack->frame_new->arguments);

    // new frame must be deleted to prevent future errors
    // it's a simulated "push-and-pop" from the stack
    stack->frame_new.reset(nullptr);

    return adjust_jump_base_for(call_name);
}

viua::internals::types::byte* viua::process::Process::opdefer(
    viua::internals::types::byte* addr) {
    auto call_name = string{};
    auto ot        = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn = fetch_and_advance_addr<viua::types::Function*>(
            viua::bytecode::decoder::operands::fetch_object_of<
                viua::types::Function>,
            addr,
            this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->back()->local_register_set.reset(
                static_cast<viua::types::Closure*>(fn)->give());
        }
    } else {
        call_name = fetch_and_advance_addr<decltype(call_name)>(
            viua::bytecode::decoder::operands::fetch_atom, addr, this);
    }

    auto const is_native         = scheduler->is_native_function(call_name);
    auto const is_foreign        = scheduler->is_foreign_function(call_name);
    auto const is_foreign_method = scheduler->is_foreign_method(call_name);

    if (not(is_native or is_foreign or is_foreign_method)) {
        throw make_unique<viua::types::Exception>(
            "defer of undefined function: " + call_name);
    }

    push_deferred(call_name);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opreturn(
    viua::internals::types::byte* addr) {
    if (stack->size() == 0) {
        throw make_unique<viua::types::Exception>(
            "no frame on stack: no call to return from");
    }

    if (stack->state_of() == viua::process::Stack::STATE::RUNNING) {
        stack->set_return_value();
        stack->register_deferred_calls();
        stack->state_of(
            viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP);

        if (not stacks_order.empty()) {
            stack = stacks_order.top();
            stacks_order.pop();
            return addr;
        }
    }

    if (stack->state_of()
        != viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
        throw make_unique<viua::types::Exception>(
            "stack left in an invalid state");
    }

    addr = stack->back()->ret_address();

    unique_ptr<viua::types::Value> returned;
    auto const return_register = stack->back()->return_register;
    if (return_register != nullptr) {
        returned = std::move(stack->return_value);
    }

    stack->state_of(viua::process::Stack::STATE::RUNNING);

    stack->pop();

    // place return value
    if (returned and stack->size() > 0) {
        *return_register = std::move(returned);
    }

    if (stack->size() > 0) {
        adjust_jump_base_for(stack->back()->function_name);
    }

    return addr;
}
