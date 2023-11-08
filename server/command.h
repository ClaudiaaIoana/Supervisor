#pragma once

#include <iostream>
#include <string.h>

class command {
    std::string text;
public:
    command();
    command(const std::string& text);
    ~command() = default;
    void execute();
    const std::string& getText() const;
    void setText(const std::string& text);
};
