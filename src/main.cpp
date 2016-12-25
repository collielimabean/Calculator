#include <iostream>
#include "Calculator.h"
#include <memory>

SimpleCalculator calc;

int main(void)
{
    std::wstring user_input = L"";
    while (true)
    {
        double result = 0;
        std::wcout << L">> ";
        std::getline(std::wcin, user_input);

        if (std::wcin.eof())
            break;

        if (user_input.empty())
            continue;
        
        auto error = calc.Evaluate(user_input, result);
        if (error == EvaluationError::OK)
            std::wcout << result << std::endl;
        else
            std::wcout << GetFriendlyEvaluationError(error) << std::endl;
    }

    return 0;
}