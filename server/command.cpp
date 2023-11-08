#include "command.h"

command::command()
{
    this->text = "command";
}

command::command(const std::string &text)
{
    this->text = text;
}

void command::execute()
{
    std::cout << "Incorrect command\n";
}

const std::string &command::getText() const
{
    return this->text;
}

void command::setText(const std::string &text)
{
    this->text = text;
}