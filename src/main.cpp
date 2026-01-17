#include <iostream>
#include <string>
#include "../include/parser.hpp"
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    parser p;
    std::string input;
    input.reserve(256);
    while (std::getline(std::cin, input))
    {
        if (!input.empty())
        {
            p.execute(input);
        }
    }
    return 0;
}
