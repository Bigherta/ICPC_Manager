#pragma once
#ifndef TEAM_HPP
#define TEAM_HPP
#include <algorithm>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include "token.hpp"
class team
{
private:
    std::string name; // 队伍名称
    int rank; // 当前排名
    int solved_count; // 通过题目数
    int time_punishment; // 总罚时
    bool has_frozen = false; // 是否被冻结
    struct ProblemStatus
    {
        int state = 0; // 0-未通过 1-已通过 2-被冻结
        int before_freeze_error_count = 0; // 最后一次封榜前的错误提交次数
        int error_count = 0; // 错误提交次数
        int submit_count = 0; // 总提交次数
        int first_ac_time = -1; // 首次通过时间
        int last_accept = -1; // 最后一次通过时间
        int last_wrong = -1; // 最后一次错误时间
        int last_re = -1; // 最后一次运行时错误时间
        int last_tle = -1; // 最后一次超时错误时间
        int last_submit_time = -1; // 该题最近一次提交时间（用于 ALL 状态查询）
        TokenType last_submit_type = TokenType::UNKNOWN; // 该题最近一次提交类型
        friend std::ostream &operator<<(std::ostream &os, const ProblemStatus &obj)
        {
            if (obj.state == 0)
            {
                if (obj.error_count == 0)
                {
                    os << '.';
                }
                else
                {
                    os << '-' << obj.error_count;
                }
            }
            else if (obj.state == 1)
            {
                if (obj.error_count == 0)
                {
                    os << '+';
                }
                else
                {
                    os << '+' << obj.error_count;
                }
            }
            else
            {
                int post_freeze_submits = obj.submit_count - obj.before_freeze_error_count;
                if (obj.before_freeze_error_count == 0)
                {
                    os << "0/" << post_freeze_submits;
                }
                else
                {
                    os << '-' << obj.before_freeze_error_count << '/' << post_freeze_submits;
                }
            }
            return os;
        }
        friend bool operator<(const ProblemStatus &a, const ProblemStatus &b)
        {
            return a.first_ac_time < b.first_ac_time;
        }
    };
    std::pair<std::pair<int, TokenType>, int> last_submit = {
            {-1, TokenType::UNKNOWN}, -1}; // first.first: 题目序号，first.second: 提交类型，second最后一次提交时间
    std::pair<int, int> last_accept = {-1, -1}; // first: 题目序号， second最后一次通过时间
    std::pair<int, int> last_wrong = {-1, -1}; // first: 题目序号， second最后一次错误时间
    std::pair<int, int> last_re = {-1, -1}; // first: 题目序号， second最后一次运行时错误时间
    std::pair<int, int> last_tle = {-1, -1}; // first: 题目序号， second最后一次超时错误时间
    std::vector<ProblemStatus> problem_submit_status;
    std::vector<int> problem_solved; // 已通过的题目首次通过时间（有序）
public:
    team() : name(""), rank(0), solved_count(0), time_punishment(0){};
    team(const std::string &team_name) : name(team_name), rank(0), solved_count(0), time_punishment(0){};
    std::string get_name() const { return name; }
    int &get_rank() { return rank; }
    int get_rank() const { return rank; }
    int &get_solved_count() { return solved_count; }
    int get_solved_count() const { return solved_count; }
    std::vector<ProblemStatus> &get_submit_status() { return problem_submit_status; }
    std::vector<int> &get_problem_solved() { return problem_solved; }
    const std::vector<int> &get_problem_solved() const { return problem_solved; }
    void add_solved_time(int t)
    {
        auto it = std::lower_bound(problem_solved.begin(), problem_solved.end(), t);
        problem_solved.insert(it, t);
    }
    int &get_time_punishment() { return time_punishment; }
    int get_time_punishment() const { return time_punishment; }
    bool &get_has_frozen() { return has_frozen; }
    bool get_has_frozen() const { return has_frozen; }
    // last_* accessors
    std::pair<std::pair<int, TokenType>, int> &get_last_submit() { return last_submit; }
    std::pair<int, int> &get_last_accept() { return last_accept; }
    std::pair<int, int> &get_last_wrong() { return last_wrong; }
    std::pair<int, int> &get_last_re() { return last_re; }
    std::pair<int, int> &get_last_tle() { return last_tle; }
    void set_last_submit(int probIdx, TokenType status, int time) { last_submit = {{probIdx, status}, time}; }
    void set_last_accept(int probIdx, int time) { last_accept = {probIdx, time}; }
    void set_last_wrong(int probIdx, int time) { last_wrong = {probIdx, time}; }
    void set_last_re(int probIdx, int time) { last_re = {probIdx, time}; }
    void set_last_tle(int probIdx, int time) { last_tle = {probIdx, time}; }
    friend bool operator<(const team &a, const team &b)
    {
        if (a.solved_count != b.solved_count)
        {
            return a.solved_count > b.solved_count;
        }
        if (a.time_punishment != b.time_punishment)
        {
            return a.time_punishment < b.time_punishment;
        }
        for (auto it_a = a.problem_solved.rbegin(), it_b = b.problem_solved.rbegin();
             it_a != a.problem_solved.rend() && it_b != b.problem_solved.rend(); ++it_a, ++it_b)
        {
            if (*it_a != *it_b)
            {
                return *it_a < *it_b;
            }
        }
        return a.name < b.name;
    }
};

#endif // TEAM_HPP
