#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string_view>
#include <vector>
enum class TokenType
{
    ADDTEAM,
    START,
    SUBMIT,
    FLUSH,
    FREEZE,
    SCROLL,
    QUERY_RANKING,
    QUERY_SUBMISSION,
    END,

    ACCEPTED,
    WRONG_ANSWER,
    TIME_LIMIT_EXCEED,
    RUNTIME_ERROR,

    UNKNOWN
};
class token
{
public:
    TokenType type = TokenType::UNKNOWN;
    std::string_view value;
};

class tokenstream
{
private:
    std::vector<token> tokens;
    size_t currentIndex = 0;

public:
    tokenstream(const std::vector<token> &tokens) : tokens(tokens) {}
    const token *peek()
    {
        if (currentIndex < tokens.size())
        {
            return &tokens[currentIndex];
        }
        return nullptr;
    }
    token *get()
    {
        if (currentIndex < tokens.size())
        {
            return &tokens[currentIndex++];
        }
        return nullptr;
    }
};

#endif
