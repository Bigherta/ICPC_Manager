#pragma once
#ifndef PARSER_HPP
#define PARSER_HPP
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include "team.hpp"
#include "token.hpp"

/**
 * 关键字 → TokenType 的映射表
 * 用于词法分析阶段，把输入的字符串关键字转成对应的 TokenType
 */
inline std::unordered_map<std::string, TokenType> keywordMap = {{"ADDTEAM", TokenType::ADDTEAM},
                                                                {"START", TokenType::START},
                                                                {"SUBMIT", TokenType::SUBMIT},
                                                                {"FLUSH", TokenType::FLUSH},
                                                                {"FREEZE", TokenType::FREEZE},
                                                                {"SCROLL", TokenType::SCROLL},
                                                                {"QUERY_RANKING", TokenType::QUERY_RANKING},
                                                                {"QUERY_SUBMISSION", TokenType::QUERY_SUBMISSION},
                                                                {"END", TokenType::END},

                                                                {"Accepted", TokenType::ACCEPTED},
                                                                {"Wrong_Answer", TokenType::WRONG_ANSWER},
                                                                {"Runtime_Error", TokenType::RUNTIME_ERROR},
                                                                {"Time_Limit_Exceed", TokenType::TIME_LIMIT_EXCEED}};

inline std::string tokenTypeToStatusString(TokenType t)
{
    switch (t)
    {
        case TokenType::ACCEPTED:
            return "Accepted";
        case TokenType::WRONG_ANSWER:
            return "Wrong_Answer";
        case TokenType::TIME_LIMIT_EXCEED:
            return "Time_Limit_Exceed";
        case TokenType::RUNTIME_ERROR:
            return "Runtime_Error";
        default:
            return "UNKNOWN";
    }
}

/**
 * parser 类
 * 负责：
 * 1. 对输入命令进行词法分析（tokenize）
 * 2. 根据 token 流执行对应命令（execute）
 */
class parser
{
private:
    /**
     * teamName → team 对象
     */
    std::unordered_map<std::string, team> teamMap;

    /**
     * 排名集合
     * 存储指向 `team` 的指针以避免频繁拷贝和构造/析构开销
     */
    struct TeamPtrLess
    {
        bool operator()(const team *a, const team *b) const { return *a < *b; }
    };

    std::set<team *, TeamPtrLess> rankingSet;

    /// 比赛是否已经开始
    bool is_started = false;

    /// 是否冻结榜单
    bool is_frozen = false;

    /// 题目数量
    int problem_count;

    /// 比赛总时长
    int duration_time;

public:
    parser() { teamMap.reserve(10000); }
    /**
     * tokenize
     * 对输入的一整行命令进行分词
     * 按空白符切分，生成 tokenstream
     */
    tokenstream tokenize(const std::string &input)
    {
        std::vector<token> tokens;
        size_t pos = 0;

        while (pos < input.length())
        {
            // 跳过前导空白字符
            while (pos < input.length() && isspace(input[pos]))
            {
                pos++;
            }
            if (pos >= input.length())
                break;

            // 找到一个完整的单词
            size_t start = pos;
            while (pos < input.length() && !isspace(input[pos]))
            {
                pos++;
            }

            std::string word = input.substr(start, pos - start);

            token tok;

            // 判断是否是关键字
            auto it = keywordMap.find(word);
            if (it != keywordMap.end())
            {
                tok.type = it->second;
            }
            else
            {
                tok.type = TokenType::UNKNOWN;
            }

            tok.value = word;
            tokens.push_back(tok);
        }

        return tokenstream(tokens);
    }

    void flush()
    {
        // 重建 rankingSet 为指针集合，避免复制大量 team 对象
        rankingSet.clear();
        for (auto &pair: teamMap)
        {
            rankingSet.insert(&pair.second);
        }

        int rank = 1;
        for (auto iter = rankingSet.begin(); iter != rankingSet.end(); ++iter)
        {
            std::string teamName = (*iter)->get_name();
            teamMap[teamName].get_rank() = rank++;
        }
    }

    void unfreeze_process(std::set<team *, TeamPtrLess> &freezeOrder)
    {
        if (freezeOrder.empty())
            return;

        // 取排名最靠后且还有冻结题的队伍（freezeOrder 存储 team*）
        auto rev_it = freezeOrder.rbegin();
        team *oldPtr = *rev_it; // 指向被处理队伍的指针
        std::string teamName = oldPtr->get_name();
        team &team_ref = teamMap[teamName];
        auto &statuses = team_ref.get_submit_status();

        // 仅解冻该队编号最小的一道冻结题
        size_t idx = statuses.size();
        for (size_t i = 0; i < statuses.size(); ++i)
        {
            if (statuses[i].state == 2)
            {
                idx = i;
                break;
            }
        }

        // 若没有冻结题则从 freezeOrder 中移除旧键
        if (idx == statuses.size())
        {
            team_ref.get_has_frozen() = false;
            freezeOrder.erase(oldPtr);
            return;
        }

        // 为保持原实现语义：在 rankingSet 仍含旧键时，使用一个 "newKey" 快照来计算 lower_bound
        team oldKey = *oldPtr; // 旧快照
        team newKey = oldKey; // 在拷贝上修改，避免在集合中直接修改元素（UB）
        auto &new_statuses = newKey.get_submit_status();
        auto &status = new_statuses[idx];
        status.state = 0;
        if (status.first_ac_time != -1)
        {
            status.state = 1;
            newKey.get_time_punishment() += status.first_ac_time + status.error_count * 20;
            newKey.add_solved_time(status.first_ac_time);
            newKey.get_solved_count()++;
        }

        bool any_frozen_left = false;
        for (const auto &s: new_statuses)
        {
            if (s.state == 2)
            {
                any_frozen_left = true;
                break;
            }
        }
        newKey.get_has_frozen() = any_frozen_left;

        // 在仍含旧键的 rankingSet 上，用 newKey 计算将被取代的队伍
        auto it = rankingSet.lower_bound(&newKey); // O(log N)
        if (it != rankingSet.end())
        {
            team *displaced = *it;
            if (displaced->get_name() != teamName)
            {
                std::cout << teamName << " " << displaced->get_name() << " " << newKey.get_problem_solved().size()
                          << " " << newKey.get_time_punishment() << '\n';
            }
        }

        // 从集合中移除旧指针，并把 newKey 写回到 teamMap（替换旧对象），再插入新指针
        rankingSet.erase(oldPtr);
        freezeOrder.erase(oldPtr);

        teamMap[teamName] = newKey; // 复制更新后的快照到实际存储
        team *ptr = &teamMap[teamName];
        rankingSet.insert(ptr);

        if (ptr->get_has_frozen())
            freezeOrder.insert(ptr);
    }
    /**
     * execute
     * 执行一条命令
     * 通过第一个 token 决定命令类型
     */
    void execute(const std::string &cmd)
    {
        tokenstream ts = tokenize(cmd);

        // 获取命令关键字
        token *keyToken = ts.get();

        switch (keyToken->type)
        {

            /**
             * ADDTEAM teamName
             * 添加一支队伍
             */
            case TokenType::ADDTEAM: {
                // 比赛开始后禁止添加队伍
                if (is_started)
                {
                    std::cout << "[Error]Add failed: competition has started.\n";
                    return;
                }

                token *nameToken = ts.get();
                if (nameToken)
                {
                    std::string teamName = nameToken->value;

                    // 检查重名
                    if (teamMap.find(teamName) == teamMap.end())
                    {
                        teamMap.emplace(teamName, team(teamName));
                        rankingSet.emplace(&teamMap[teamName]);
                        std::cout << "[Info]Add successfully.\n";
                    }
                    else
                    {
                        std::cout << "[Error]Add failed: duplicated team name.\n";
                    }
                }
                break;
            }

            /**
             * START DURATION x PROBLEM y
             * 开始比赛并初始化数据
             */
            case TokenType::START: {
                if (is_started)
                {
                    std::cout << "[Error]Start failed: competition has started.\n";
                    return;
                }

                is_started = true;
                std::cout << "[Info]Competition starts.\n";

                ts.get(); // 跳过 "DURATION"
                token *duration = ts.get(); // 比赛时长
                duration_time = std::stoi(duration->value);

                ts.get(); // 跳过 "PROBLEM"
                token *count = ts.get(); // 题目数量
                problem_count = std::stoi(count->value);

                // 初始化排名和每道题的提交状态
                int rank = 1;
                for (auto iter = rankingSet.begin(); iter != rankingSet.end(); ++iter)
                {
                    std::string teamName = (*iter)->get_name();
                    teamMap[teamName].get_rank() = rank++;

                    // 初始化每支队伍的每道题提交记录为默认 ProblemStatus
                    teamMap[teamName].get_submit_status().resize(problem_count);
                }
                break;
            }

            /**
             * SUBMIT problem BY team WITH status AT time
             * 处理一次提交
             */
            case TokenType::SUBMIT: {
                token *problemnameToken = ts.get();
                ts.get(); // BY
                token *teamnameToken = ts.get();
                ts.get(); // WITH
                token *statusToken = ts.get();
                ts.get(); // AT
                token *timeToken = ts.get();

                std::string problemName = problemnameToken->value;
                std::string teamName = teamnameToken->value;
                int submitTime = std::stoi(timeToken->value);

                int problemIdx = problemName[0] - 'A';
                auto &submitStatus = teamMap[teamName].get_submit_status()[problemIdx];

                // 统一计数提交次数
                submitStatus.submit_count += 1;

                // 记录 team 级别的最近一次提交（用于时间平局时的判定）
                teamMap[teamName].set_last_submit(problemIdx, statusToken->type, submitTime);

                // 记录该题最近一次提交状态与时间（用于 ALL 状态 + 指定题目的查询）
                submitStatus.last_submit_time = submitTime;
                submitStatus.last_submit_type = statusToken->type;

                bool already_solved = (submitStatus.state == 1);
                bool is_ac = (statusToken->type == TokenType::ACCEPTED);

                if (is_ac)
                {
                    teamMap[teamName].get_submit_status()[problemIdx].last_accept = submitTime;
                    teamMap[teamName].set_last_accept(problemIdx, submitTime);
                    if (!already_solved)
                    {
                        if (submitStatus.first_ac_time == -1)
                            submitStatus.first_ac_time = submitTime;

                        if (is_frozen)
                        {
                            // 封榜期间：仅标记冻结，不更新通过与罚时
                            submitStatus.state = 2;
                            teamMap[teamName].get_has_frozen() = true;
                        }
                        else
                        {
                            // 非封榜：立即生效
                            submitStatus.state = 1;
                            teamMap[teamName].get_time_punishment() += submitTime + submitStatus.error_count * 20;
                            teamMap[teamName].add_solved_time(submitStatus.first_ac_time);
                            teamMap[teamName].get_solved_count()++;
                        }
                    }
                }
                else
                {
                    // 非 AC：仅在首次 AC 之前计入错误
                    if (!already_solved && submitStatus.first_ac_time == -1)
                    {
                        submitStatus.error_count += 1;
                    }

                    // 封榜期间且封榜前未通过的题会被冻结
                    if (is_frozen && !already_solved)
                    {
                        submitStatus.state = 2;
                        teamMap[teamName].get_has_frozen() = true;
                    }

                    if (statusToken->type == TokenType::WRONG_ANSWER)
                    {
                        submitStatus.last_wrong = submitTime;
                        teamMap[teamName].set_last_wrong(problemIdx, submitTime);
                    }
                    else if (statusToken->type == TokenType::TIME_LIMIT_EXCEED)
                    {
                        submitStatus.last_tle = submitTime;
                        teamMap[teamName].set_last_tle(problemIdx, submitTime);
                    }
                    else if (statusToken->type == TokenType::RUNTIME_ERROR)
                    {
                        submitStatus.last_re = submitTime;
                        teamMap[teamName].set_last_re(problemIdx, submitTime);
                    }
                }
                break;
            }

            /**
             * FLUSH
             * 重新排序榜单
             */
            case TokenType::FLUSH: {
                flush();
                std::cout << "[Info]Flush scoreboard.\n";
                break;
            }

            case TokenType::FREEZE: {
                if (is_frozen)
                {
                    std::cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
                    return;
                }
                // 在设置封榜标志前，快照每支队伍每道题的封榜前统计
                for (auto &pair: teamMap)
                {
                    auto &statuses = pair.second.get_submit_status();
                    for (auto &s: statuses)
                    {
                        s.before_freeze_error_count = s.error_count;
                    }
                }
                is_frozen = true;
                std::cout << "[Info]Freeze scoreboard.\n";
                break;
            }

            case TokenType::SCROLL: {
                if (!is_frozen)
                {
                    std::cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
                    return;
                }
                is_frozen = false;
                std::cout << "[Info]Scroll scoreboard.\n";
                flush();
                for (auto iterator = rankingSet.begin(); iterator != rankingSet.end(); ++iterator)
                {
                    std::string teamName = (*iterator)->get_name();
                    team &team_ = teamMap[teamName];
                    std::cout << teamName << " " << team_.get_rank() << " " << team_.get_problem_solved().size() << " "
                              << team_.get_time_punishment() << " ";
                    for (const auto &status: team_.get_submit_status())
                    {
                        std::cout << status << " ";
                    }
                    std::cout << "\n";
                }
                std::set<team *, TeamPtrLess> freezeOrder; // 未解冻的队伍排序（指针集合，使用 TeamPtrLess）
                for (auto &pair: teamMap)
                {
                    team &team_ = pair.second;
                    if (team_.get_has_frozen())
                    {
                        freezeOrder.insert(&team_);
                    }
                }
                while (freezeOrder.size() > 0)
                {
                    unfreeze_process(freezeOrder);
                }
                // 滚榜结束后刷新，输出最终正确排名
                flush();
                for (auto iterator = rankingSet.begin(); iterator != rankingSet.end(); ++iterator)
                {
                    std::string teamName = (*iterator)->get_name();
                    team &team_ = teamMap[teamName];
                    std::cout << teamName << " " << team_.get_rank() << " " << team_.get_problem_solved().size() << " "
                              << team_.get_time_punishment() << " ";
                    for (const auto &status: team_.get_submit_status())
                    {
                        std::cout << status << " ";
                    }
                    std::cout << "\n";
                }
                break;
            }

            case TokenType::QUERY_RANKING: {
                token *nameToken = ts.get();
                std::string teamName = nameToken->value;
                if (teamMap.find(teamName) != teamMap.end())
                {
                    std::cout << "[Info]Complete query ranking.\n";
                    if (is_frozen)
                    {
                        std::cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were "
                                     "scrolled.\n";
                    }
                    std::cout << teamName << " NOW AT RANKING " << teamMap[teamName].get_rank() << "\n";
                }
                else
                {
                    std::cout << "[Error]Query ranking failed: cannot find the team.\n";
                }
                break;
            }

            case TokenType::QUERY_SUBMISSION: {
                token *nameToken = ts.get();
                ts.get(); // WHERE
                token *problemToken = ts.get();
                std::string problemName = problemToken->value.substr(8, problemToken->value.length() - 8);
                ts.get(); // AND
                token *statusToken = ts.get();
                std::string statusName = statusToken->value.substr(7, statusToken->value.length() - 7);
                if (teamMap.find(nameToken->value) != teamMap.end())
                {
                    std::cout << "[Info]Complete query submission.\n";
                    std::string teamName = nameToken->value;
                    auto &team_ = teamMap[teamName];
                    bool is_search_all_problems = (problemName == "ALL");
                    bool is_search_all_status = (statusName == "ALL");
                    auto &statuses = team_.get_submit_status();
                    if (is_search_all_status && is_search_all_problems)
                    {
                        auto &last_submit = team_.get_last_submit();
                        if (last_submit.second == -1)
                            std::cout << "Cannot find any submission." << "\n";
                        else
                            std::cout << teamName << " " << char('A' + last_submit.first.first) << " "
                                      << tokenTypeToStatusString(last_submit.first.second) << " " << last_submit.second
                                      << "\n";
                    }
                    else if (is_search_all_status && !is_search_all_problems)
                    {
                        int tmp = problemName[0] - 'A';
                        if (tmp < 0 || static_cast<size_t>(tmp) >= statuses.size())
                        {
                            std::cout << "Cannot find any submission." << "\n";
                        }
                        else
                        {
                            size_t idx = static_cast<size_t>(tmp);
                            auto &s = statuses[idx];
                            if (s.last_submit_time == -1)
                            {
                                std::cout << "Cannot find any submission." << "\n";
                            }
                            else
                            {
                                std::cout << teamName << " " << problemName << " "
                                          << tokenTypeToStatusString(s.last_submit_type) << " " << s.last_submit_time
                                          << "\n";
                            }
                        }
                    }
                    else if (!is_search_all_status && is_search_all_problems)
                    {
                        // 针对指定状态在所有题目的查询，直接使用 team 级别记录
                        std::string target = statusName;
                        int bestTime = -1;
                        char bestProblem = 'A';
                        if (target == "Accepted")
                        {
                            auto &p = team_.get_last_accept();
                            bestTime = p.second;
                            if (p.first >= 0)
                                bestProblem = char('A' + p.first);
                        }
                        else if (target == "Wrong_Answer")
                        {
                            auto &p = team_.get_last_wrong();
                            bestTime = p.second;
                            if (p.first >= 0)
                                bestProblem = char('A' + p.first);
                        }
                        else if (target == "Time_Limit_Exceed")
                        {
                            auto &p = team_.get_last_tle();
                            bestTime = p.second;
                            if (p.first >= 0)
                                bestProblem = char('A' + p.first);
                        }
                        else if (target == "Runtime_Error")
                        {
                            auto &p = team_.get_last_re();
                            bestTime = p.second;
                            if (p.first >= 0)
                                bestProblem = char('A' + p.first);
                        }

                        if (bestTime == -1)
                            std::cout << "Cannot find any submission." << "\n";
                        else
                            std::cout << teamName << " " << bestProblem << " " << statusName << " " << bestTime << "\n";
                    }
                    else
                    {
                        int tmp = problemName[0] - 'A';
                        if (tmp < 0 || static_cast<size_t>(tmp) >= statuses.size())
                        {
                            std::cout << "Cannot find any submission." << "\n";
                        }
                        else
                        {
                            size_t idx = static_cast<size_t>(tmp);
                            auto &s = statuses[idx];
                            int t = -1;
                            if (statusName == "Accepted")
                                t = s.last_accept;
                            else if (statusName == "Wrong_Answer")
                                t = s.last_wrong;
                            else if (statusName == "Time_Limit_Exceed")
                                t = s.last_tle;
                            else if (statusName == "Runtime_Error")
                                t = s.last_re;

                            if (t == -1)
                                std::cout << "Cannot find any submission." << "\n";
                            else
                                std::cout << teamName << " " << problemName << " " << statusName << " " << t << "\n";
                        }
                    }
                }
                else
                {
                    std::cout << "[Error]Query submission failed: cannot find the team.\n";
                }
                break;
            }

            case TokenType::END: {
                std::cout << "[Info]Competition ends.\n";
                break;
            }

            default:
                break;
        }
    }
};
#endif // PARSER_HPP
