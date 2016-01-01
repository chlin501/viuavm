#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <viua/types/type.h>
#include <viua/types/pointer.h>
#include <viua/types/exception.h>
using namespace std;


void Pointer::attach() {
    points_to->pointers.push_back(this);
    valid = true;
}
void Pointer::detach() {
    if (valid) {
        points_to->pointers.erase(std::find(points_to->pointers.begin(), points_to->pointers.end(), this));
    }
    valid = false;
}

void Pointer::invalidate(Type* t) {
    if (t == points_to) {
        valid = false;
    }
}
bool Pointer::expired() {
    return !valid;
}
void Pointer::reset(Type* t) {
    detach();
    points_to = t;
    attach();
}
Type* Pointer::to() {
    return points_to;
}

string Pointer::type() const {
    if (not valid) {
        throw new Exception("expired pointer exception");
    }
    return (points_to->type() + "Pointer");
}

bool Pointer::boolean() const {
    return valid;
}

Type* Pointer::copy() const {
    if (not valid) {
        throw new Exception("expired pointer exception");
    }
    return new Pointer(points_to);
}

Pointer::Pointer(Type* t): points_to(t), valid(true) {
    attach();
}
Pointer::~Pointer() {
    detach();
}
