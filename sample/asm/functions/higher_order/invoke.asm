;
;   Copyright (C) 2015, 2016, 2017 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.function: sum/4
    ; this function takes four integers as parameters and
    ; adds them, and returns the sum
    add %0 (arg %4 %3) (add %0 (arg %3 %2) (add %0 (arg %1 %0) (arg %2 %1)))
    return
.end

.function: invoke/2
    ; this function takes two parameters:
    ;    1) a function object
    ;    2) a vector of parameters for function given as first parameter
    ;
    ; it then creates a frame with required number of parameter slots (as
    ; specified by length of the vector), and calls given function with this
    ; frame
    arg (.name: %iota fn_to_call) %0
    arg (.name: %iota parameters_list) %1

    ; take length of the vector
    .name: %iota vector_length
    vlen %vector_length %2
    frame @vector_length

    ; zero loop counter
    .name: %iota loop_counter
    izero %loop_counter
    .mark: while_begin

    ; simple condition:
    ; while (loop_counter < vector_length) {
    if (gte %iota %loop_counter %vector_length) while_end while_body

    .mark: while_body

    .name: %iota slot
    ; store item located inside parameter vector at index denoted by loop_counter in
    ; a register and
    ; pass it as a parameter
    pamv @loop_counter (copy %iota *(vat %slot %parameters_list %loop_counter))

    ; loop_counter++
    iinc %loop_counter

    jump while_begin

    .mark: while_end

    ; finally, after the frame is ready
    ; call the function
    move %0 (call %iota %fn_to_call)
    return
.end

.function: main/1
    ; create the vector
    vpush (vec %1) (istore %2 20)
    vpush %1 (istore %3 16)
    vpush %1 (istore %4 8)
    vpush %1 (istore %5 -2)

    istore %2 20
    istore %3 16
    istore %4 8
    istore %5 -2

    ; call sum/4() function
    frame ^[(param %0 %2) (param %1 %3) (param %2 %4) (param %3 %5)]
    print (call %6 sum/4)

    ; call sum/4 function via invoke/2 function
    frame ^[(param %0 (function %7 sum/4)) (param %1 %1)]
    print (call %8 invoke/2)

    izero %0 local
    return
.end
