#pragma once
#include <string>
#include <vector>

inline const std::vector<std::string>& SecurityQuestions()
{
    static std::vector<std::string> questions = {
        "What is/was your mother's maiden name?",
        "What city did your parents meet?",
        "What year did you graduate high school?",
        "What was the name of your first supervisor?",
        "What was the model of your first car?"
    };
    return questions;
}
