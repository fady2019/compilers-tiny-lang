#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <string>
#include <unordered_map>

using namespace std;

enum class TokenType
{
    IF,
    THEN,
    ELSE,
    END,
    REPEAT,
    UNTIL,
    READ,
    WRITE,
    ASSIGN, // :=
    EQUAL,  // ==
    LESS_THAN,
    SEMI_COLON,
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    POWER,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    ID,
    NUM,
    ENDFILE,
    ERROR
};

// Token structure
struct Token
{
    int lineNumber;
    string value;   // lexeme
    TokenType type; // token
};

// Function to classify tokens
TokenType classifyToken(const string &token)
{
    if (token == "if")
    {
        return TokenType::IF;
    }
    else if (token == "then")
    {
        return TokenType::THEN;
    }
    else if (token == "else")
    {
        return TokenType::ELSE;
    }
    else if (token == "end")
    {
        return TokenType::END;
    }
    else if (token == "repeat")
    {
        return TokenType::REPEAT;
    }
    else if (token == "until")
    {
        return TokenType::UNTIL;
    }
    else if (token == "read")
    {
        return TokenType::READ;
    }
    else if (token == "write")
    {
        return TokenType::WRITE;
    }
    else if (token == ";")
    {
        return TokenType::SEMI_COLON;
    }
    else if (token == ":=")
    {
        return TokenType::ASSIGN;
    }
    else if (token == "==")
    {
        return TokenType::EQUAL;
    }
    else if (token == "<")
    {
        return TokenType::LESS_THAN;
    }
    else if (token == "+")
    {
        return TokenType::PLUS;
    }
    else if (token == "-")
    {
        return TokenType::MINUS;
    }
    else if (token == "*")
    {
        return TokenType::TIMES;
    }
    else if (token == "/")
    {
        return TokenType::DIVIDE;
    }
    else if (token == "^")
    {
        return TokenType::POWER;
    }
    else if (token == ";")
    {
        return TokenType::SEMI_COLON;
    }
    else if (token == "()")
    {
        return TokenType::LEFT_PAREN;
    }
    else if (token == ")")
    {
        return TokenType::RIGHT_PAREN;
    }
    else if (token == "{")
    {
        return TokenType::LEFT_BRACE;
    }
    else if (token == "}")
    {
        return TokenType::RIGHT_BRACE;
    }
    else if (std::isdigit(token[0]))
    {
        return TokenType::NUM;
    }
    else if (token == "error")
    {
        return TokenType::ERROR;
    }
    else if (token == "endfile")
    {
        return TokenType::ENDFILE;
    }
    else if (! std::isdigit(token[0])) // for variable names and labels
    {
        return TokenType::ID;
    }
}

int main()
{
    ifstream inputFile("input.txt");
    if (!inputFile)
    {
        std::cerr << "Error: Unable to open input file.\n";
        return 1;
    }

    // Output tokens to a text file
    std::ofstream outputFile("output.txt");
    if (!outputFile)
    {
        std::cerr << "Error: Unable to create output file.\n";
        return 1;
    }

    std::vector<Token> tokens;
    std::string line;
    int lineCount = 1;

    while (std::getline(inputFile, line))
    {
        std::stringstream ss(line);
        std::string token;

        while (ss >> token)
        {
            TokenType type = classifyToken(token); // take it char by char
            tokens.push_back({lineCount, token, type});
        }

        lineCount++;
    }

    // Write tokens to the output file
    for (const auto &token : tokens)
    {
        outputFile << "[" << token.lineNumber << "] " << token.value << " (";
        switch (token.type)
        {
        case TokenType::IF:
            outputFile << "IF";
            break;
        case TokenType::THEN:
            outputFile << "THEN";
            break;
        case TokenType::ELSE:
            outputFile << "ELSE";
            break;
        case TokenType::END:
            outputFile << "END";
            break;
        case TokenType::REPEAT:
            outputFile << "REPEAT";
            break;
        case TokenType::UNTIL:
            outputFile << "UNTIL";
            break;
        case TokenType::READ:
            outputFile << "READ";
            break;
        case TokenType::WRITE:
            outputFile << "WRITE";
            break;
        case TokenType::ASSIGN:
            outputFile << "ASSIGN";
            break;
        case TokenType::EQUAL:
            outputFile << "EQUAL";
            break;
        case TokenType::LESS_THAN:
            outputFile << "LESS_THAN";
            break;
        case TokenType::PLUS:
            outputFile << "PLUS";
            break;
        case TokenType::MINUS:
            outputFile << "MINUS";
            break;
        case TokenType::TIMES:
            outputFile << "TIMES";
            break;
        case TokenType::DIVIDE:
            outputFile << "DIVIDE";
            break;
        case TokenType::POWER:
            outputFile << "POWER";
            break;
        case TokenType::SEMI_COLON:
            outputFile << "SEMI_COLON";
            break;
        case TokenType::LEFT_PAREN:
            outputFile << "LEFT_PAREN";
            break;
        case TokenType::RIGHT_PAREN:
            outputFile << "RIGHT_PAREN";
            break;
        case TokenType::ID:
            outputFile << "ID";
            break;
        case TokenType::NUM:
            outputFile << "NUM";
            break;
        case TokenType::ERROR:
            outputFile << "ERROR";
            break;
        case TokenType::ENDFILE:
            outputFile << "ENDFILE";
            break;
        case TokenType::LEFT_BRACE:
            outputFile << "LEFT_BRACE";
            break;
        case TokenType::RIGHT_BRACE:
            outputFile << "RIGHT_BRACE";
            break;
            // Add more cases as needed
        }
        outputFile << ")\n";
    }

    inputFile.close();
    outputFile.close();

    return 0;
}
