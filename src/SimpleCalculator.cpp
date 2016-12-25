#include "Calculator.h"
#include <cwctype>
#include <stack>
#include <map>
#include <cmath>


enum class TokenType
{
    Number, Add, Sub, Mult, Div, Exp,
    LParanthesis, RParenthesis
};

struct Token
{
    TokenType type;
    double val;

    Token() { }

    Token(TokenType t, double val = 0)
    {
        this->type = t;
        this->val = val;
    }
};

std::map<wchar_t, Token> Operators = {
    { L'+', Token(TokenType::Add) },
    { L'-', Token(TokenType::Sub) },
    { L'*', Token(TokenType::Mult) },
    { L'/', Token(TokenType::Div) },
    { L'^', Token(TokenType::Exp) },
    { L'(', Token(TokenType::LParanthesis) },
    { L')', Token(TokenType::RParenthesis) }
};

std::map<TokenType, int> OperatorPrecendence = {
    { TokenType::Add, 0 },
    { TokenType::Sub, 0 },
    { TokenType::Mult, 1 },
    { TokenType::Div, 1 },
    { TokenType::Exp, 2 }
};

EvaluationError Tokenize(const std::wstring& text, std::vector<Token>& tokens);
EvaluationError ConvertToRPN(const std::vector<Token>& tokens, std::vector<Token>& rpn);

SimpleCalculator::SimpleCalculator()
{
}

EvaluationError SimpleCalculator::Evaluate(const std::wstring& text, double& result)
{
    EvaluationError error;
    // note: this can probably be improved by scanning as we go
    // but at this point, performance not critical

    std::vector<Token> tokens;
    if ((error = Tokenize(text, tokens)) != EvaluationError::OK)
        return error;

    // transform to RPN
    std::vector<Token> rpn;
    if ((error = ConvertToRPN(tokens, rpn)) != EvaluationError::OK)
        return error;

    // evaluate rpn expression
    std::stack<double> eval_stack;
    for (auto& t : rpn)
    {
        if (t.type == TokenType::Number)
        {
            eval_stack.push(t.val);
            continue;
        }

        // not enough input arguments
        // for the simple calculator, only 2 operands
        if (eval_stack.size() < 2)
            return EvaluationError::NotEnoughInputs;

        double arg1 = eval_stack.top();
        eval_stack.pop();
        double arg2 = eval_stack.top();
        eval_stack.pop();

        double arg3 = 0;
        switch (t.type)
        {
            case TokenType::Add:
                arg3 = arg2 + arg1;
                break;
            case TokenType::Sub:
                arg3 = arg2 - arg1;
                break;
            case TokenType::Mult:
                arg3 = arg2 * arg1;
                break;
            case TokenType::Div:
                arg3 = arg2 / arg1;
                break;
            case TokenType::Exp:
                arg3 = std::pow(arg2, arg1);
                break;
        }

        eval_stack.push(arg3);
    }

    // too many input arguments
    if (eval_stack.size() > 1)
        return EvaluationError::TooManyInputs;

    result = eval_stack.top();
    return EvaluationError::OK;
}

EvaluationError Tokenize(const std::wstring& text, std::vector<Token>& tokens)
{
    /*
     * For this simple calculator, we can safely throw out any alphabet stuff.
     * This is a FSM with 2 states: "number parsing" and "operator parsing".
     * "number parsing" just grabs all the digits (plus a decimal point as well).
     *     Moves to operator parsing state if an operator or whitespace is encountered.
     * "operator parsing" looks at the current char and does a lookup. If it's an operator,
     *     add it to the tokens list and move to the next character. Whitespace, keep going.
     *     A number/digit? Move to the number parsing state.
     */

    int start_index = 0;
    bool parsing_number = false;
    for (size_t i = 0; i < text.length(); i++)
    {
        wchar_t curr = text[i];

        if (std::iswalpha(curr))
            return EvaluationError::InvalidCharacters;

        bool token_is_space = std::iswspace(curr) != 0;
        bool token_is_op = Operators.count(curr) > 0;

        if (!parsing_number)
        {
            // decimal value check (e.g. 45.33) and neg value check (-3)
            if (std::iswdigit(curr) > 0 || curr == L'.'
                || (Operators[curr].type == TokenType::Sub && i + 1 < text.length() && std::iswdigit(text[i + 1])))
            {
                parsing_number = true;
                start_index = i;
            }
            else if (token_is_op)
            {
                tokens.push_back(Operators[curr]);
            }
            else if (token_is_space)
            {
                continue;
            }
            else
            {
                return EvaluationError::UnknownOperator;
            }
        }
        else
        {
            bool number_consumed = token_is_space || token_is_op;

            if (number_consumed)
            {
                parsing_number = false;

                // add number to token list
                double val = std::wcstod(text.substr(start_index, i - start_index).c_str(), nullptr);
                tokens.push_back(Token(TokenType::Number, val));

                if (token_is_op)
                {
                    tokens.push_back(Operators[curr]);
                }
            }
        }
    }

    // if parsing number, but reached end of string
    if (parsing_number)
    {
        double val = std::wcstod(text.substr(start_index).c_str(), nullptr);
        tokens.push_back(Token(TokenType::Number, val));
    }

    return EvaluationError::OK;
}

EvaluationError ConvertToRPN(const std::vector<Token>& tokens, std::vector<Token>& rpn)
{
    // shunting yard algorithm //
    std::stack<Token> op_stack;

    for (auto& t : tokens)
    {
        switch (t.type)
        {
            case TokenType::Number:
                rpn.push_back(t);
                continue;

            case TokenType::LParanthesis:
                op_stack.push(t);
                continue;

            case TokenType::RParenthesis:
            {
                bool left_paren_found = false;
                while (!op_stack.empty() && !left_paren_found)
                {
                    auto& popped_op = op_stack.top();
                    op_stack.pop();

                    if (popped_op.type != TokenType::LParanthesis)
                        rpn.push_back(popped_op);
                    else
                    {
                        left_paren_found = true;
                        break;
                    }
                }

                if (!left_paren_found)
                    return EvaluationError::MismatchedParantheses; 
            }
                break;

            default:
                if (!op_stack.empty())
                {
                    bool is_left_assoc = t.type != TokenType::Exp;
                    auto& top_op = op_stack.top();

                    if (top_op.type != TokenType::LParanthesis)
                    {
                        if ((is_left_assoc && OperatorPrecendence[t.type] <= OperatorPrecendence[top_op.type])
                            || (!is_left_assoc && OperatorPrecendence[t.type] < OperatorPrecendence[top_op.type]))
                        {
                            rpn.push_back(top_op);
                            op_stack.pop();
                        }
                    }
                }

                op_stack.push(t);
                break;
        }
    }

    while (!op_stack.empty())
    {
        auto& popped_op = op_stack.top();
        op_stack.pop();

        if (popped_op.type != TokenType::LParanthesis)
            rpn.push_back(popped_op);
        else
            return EvaluationError::MismatchedParantheses;
    }

    return EvaluationError::OK;
}

const std::wstring GetFriendlyEvaluationError(const EvaluationError e)
{
    switch (e)
    {
    case EvaluationError::OK:
        return L"OK";
    case EvaluationError::InvalidCharacters:
        return L"Invalid characters were detected in the expression.";
    case EvaluationError::UnknownOperator:
        return L"An unknown operator was supplied.";
    case EvaluationError::MismatchedParantheses:
        return L"Mismatched parantheses were detected!";
    case EvaluationError::TooManyInputs:
        return L"Too many inputs for a given operation were supplied, e.g. 1 3 + 4";
    case EvaluationError::NotEnoughInputs:
        return L"Not enough inputs for the given expression, e.g.  1 - 2 +";
    default:
        return L"Invalid EvaluationError supplied!";
    }
}