#ifndef VIUA_EXCEPTIONS_H
#define VIUA_EXCEPTIONS_H

#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <viua/types/exception.h>


class OutOfRangeException: public Exception {
    protected:
        std::string cause;

    public:
        std::string type() const { return "OutOfRangeException"; }
        OutOfRangeException(const std::string& s): cause(s) {}
};

class ArityException: public Exception {
        unsigned long got_arity;
        std::vector<decltype(got_arity)> valid_arities;
    public:
        std::string type() const override {
            return "ArityException";
        }

        std::string str() const override {
            std::ostringstream oss;
            oss << "got arity " << got_arity << ", expected one of {";
            for (decltype(valid_arities)::size_type i = 0; i < valid_arities.size(); ++i) {
                oss << valid_arities[i];
                if (i < (valid_arities.size()-1)) {
                    oss << ", ";
                }
            }
            oss << '}';
            return oss.str();
        }

        Type* copy() const override {
            return new ArityException(got_arity, valid_arities);
        }

        std::string what() const override {
            return str();
        }

        ArityException(decltype(got_arity) a, decltype(valid_arities) v): got_arity(a), valid_arities(v) {}
        ~ArityException() {}
};


#endif
