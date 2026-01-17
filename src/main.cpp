#include <iostream>
#include <string>
#include "../include/parser.hpp"
int main()
{
    parser p;
    std::string input;
    while (std::getline(std::cin, input))
    {
        if (!input.empty())
        {
            p.execute(input);
        }
    }
    return 0;
}
