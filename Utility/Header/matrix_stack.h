#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>
#include <iostream>


struct matrix_stack {
private:
    std::vector<glm::mat4> _stack;

public:
    matrix_stack(glm::mat4 m) {
        _stack.push_back(m);
    }

    matrix_stack() {
        _stack.push_back(glm::mat4(1.0));
    }


    void push(glm::mat4 m) {
        _stack.push_back(m);
    }

    void pushLastElement() {
        push(_stack.back());
    }

    void pushMultiply(glm::mat4 m) {
        push(_stack.back() * m);
    }

    template<bool exit_on_error = true>
    glm::mat4 pop() {
        if (_stack.empty()) {
            std::cout << "Error: pop on a empty stack. Push/pop are unbalanced" << std::endl;
            if (exit_on_error)
                exit(0);
        }
        glm::mat4 m = _stack.back();
        _stack.pop_back();
        return m;
    }

    void multiply(glm::mat4 m) {
        _stack.back() = _stack.back() * m;
    }

    void load(glm::mat4 m) {
        _stack.back() = m;
    }

    void load_identity() {
        load(glm::mat4(1.0));
    }

    const glm::mat4 &peak() { return _stack.back(); }

};