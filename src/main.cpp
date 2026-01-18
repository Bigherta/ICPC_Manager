#include <cstdio>
#include <cstring>
#include <string>
#include "../include/parser.hpp"

// Fast input buffer
namespace fastio
{
    constexpr int MAXSIZE = 1 << 20; // 1MB buffer
    char ibuf[MAXSIZE], *p1 = ibuf, *p2 = ibuf;

    inline char get()
    {
        if (p1 == p2)
        {
            p2 = ibuf + fread(ibuf, 1, MAXSIZE, stdin);
            p1 = ibuf;
            if (p1 == p2)
                return EOF;
        }
        return *p1++;
    }

    void readline(std::string &s)
    {
        s.clear();
        char c;
        while ((c = get()) != '\n' && c != EOF)
        {
            s.push_back(c);
        }
    }
} // namespace fastio

int main()
{
    parser p;
    std::string input;
    input.reserve(256);
    while (true)
    {
        fastio::readline(input);
        if (input.empty() && fastio::p1 == fastio::p2)
            break; // EOF
        if (!input.empty())
        {
            p.execute(input);
        }
    }
    return 0;
}
