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

#ifndef VIUA_FRONT_ASM_H
#define VIUA_FRONT_ASM_H

#pragma once

#include <map>
#include <string>
#include <vector>
#include <viua/cg/lex.h>
#include <viua/program.h>


struct invocables_t {
    std::vector<std::string> names;
    std::vector<std::string> signatures;
    std::map<std::string, std::vector<viua::cg::lex::Token>> tokens;
};

struct compilationflags_t {
    bool as_lib;

    bool verbose;
    bool debug;
    bool scream;
};


const std::string COLOR_FG_RED = "\x1b[38;5;1m";
const std::string COLOR_FG_YELLOW = "\x1b[38;5;3m";
const std::string COLOR_FG_CYAN = "\x1b[38;5;6m";
const std::string COLOR_FG_LIGHT_GREEN = "\x1b[38;5;10m";
const std::string COLOR_FG_LIGHT_YELLOW = "\x1b[38;5;11m";
const std::string COLOR_FG_WHITE = "\x1b[38;5;15m";
const std::string COLOR_FG_GREEN_1 = "\x1b[38;5;46m";
const std::string COLOR_FG_RED_1 = "\x1b[38;5;196m";
const std::string COLOR_FG_ORANGE_RED_1 = "\x1b[38;5;202m";
const std::string ATTR_RESET = "\x1b[0m";

std::string send_control_seq(const std::string&);


std::vector<std::vector<std::string>> decode_line_tokens(const std::vector<std::string>&);
std::vector<std::vector<std::string>> decode_line(const std::string&);

invocables_t gatherFunctions(const std::vector<viua::cg::lex::Token>&);
invocables_t gatherBlocks(const std::vector<viua::cg::lex::Token>&);
std::map<std::string, std::string> gatherMetaInformation(const std::vector<viua::cg::lex::Token>&);

viua::internals::types::bytecode_size assemble_instruction(
    Program& program, viua::internals::types::bytecode_size& instruction,
    viua::internals::types::bytecode_size i, const std::vector<viua::cg::lex::Token>& tokens,
    std::map<std::string, std::remove_reference<decltype(tokens)>::type::size_type>& marks);
void generate(std::vector<viua::cg::lex::Token>&, invocables_t&, invocables_t&, const std::string&,
              std::string&, const std::vector<std::string>&, const compilationflags_t&);


#endif
