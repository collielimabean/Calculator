#pragma once

#include <string>
#include <vector>


enum class EvaluationError
{
    OK,
    InvalidCharacters,
    UnknownOperator,
    MismatchedParantheses,
    TooManyInputs,
    NotEnoughInputs
};

const std::wstring GetFriendlyEvaluationError(const EvaluationError e);

class SimpleCalculator
{
public:
    SimpleCalculator();
    EvaluationError Evaluate(const std::wstring& text, double& result);
};

