#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

bool StartsWith(const char *a, const char *b)
{
    int nb = strlen(b);
    return strncmp(a, b, nb) == 0;
}

void Copy(char *a, const char *b, int n = 0)
{
    if (n > 0)
    {
        strncpy(a, b, n);
        a[n] = 0;
    }
    else
        strcpy(a, b);
}

void AllocateAndCopy(char **a, const char *b)
{
    if (b == 0)
    {
        *a = 0;
        return;
    }
    int n = strlen(b);
    *a = new char[n + 1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE *file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char *str)
    {
        file = 0;
        if (str)
            file = fopen(str, "r");
        cur_line_size = 0;
        cur_ind = 0;
        cur_line_num = 0;
    }
    ~InFile()
    {
        if (file)
            fclose(file);
    }

    void SkipSpaces()
    {
        while (cur_ind < cur_line_size)
        {
            char ch = line_buf[cur_ind];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
                break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char *str)
    {
        while (true)
        {
            SkipSpaces();
            while (cur_ind >= cur_line_size)
            {
                if (!GetNewLine())
                    return false;
                SkipSpaces();
            }

            if (StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind += strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind = 0;
        line_buf[0] = 0;
        if (!fgets(line_buf, MAX_LINE_LENGTH, file))
            return false;
        cur_line_size = strlen(line_buf);
        if (cur_line_size == 0)
            return false; // End of file
        cur_line_num++;
        return true;
    }

    char *GetNextTokenStr()
    {
        SkipSpaces();
        while (cur_ind >= cur_line_size)
        {
            if (!GetNewLine())
                return 0;
            SkipSpaces();
        }
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind += num;
    }
};

struct OutFile
{
    FILE *file;
    OutFile(const char *str)
    {
        file = 0;
        if (str)
            file = fopen(str, "w");
    }
    ~OutFile()
    {
        if (file)
            fclose(file);
    }

    void Out(const char *s)
    {
        fprintf(file, "%s\n", s);
        fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char *in_str, const char *out_str, const char *debug_str)
        : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType
{
    IF,
    THEN,
    ELSE,
    END,
    REPEAT,
    UNTIL,
    READ,
    WRITE,
    ASSIGN,
    EQUAL,
    LESS_THAN,
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    POWER,
    SEMI_COLON,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    ID,
    NUM,
    ENDFILE,
    ERROR
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *TokenTypeStr[] =
    {
        "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
        "Assign", "Equal", "LessThan",
        "Plus", "Minus", "Times", "Divide", "Power",
        "SemiColon",
        "LeftParen", "RightParen",
        "LeftBrace", "RightBrace",
        "ID", "Num",
        "EndFile", "Error"};

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN + 1];

    Token()
    {
        str[0] = 0;
        type = ERROR;
    }
    Token(TokenType _type, const char *_str)
    {
        type = _type;
        Copy(str, _str);
    }
};

const Token reserved_words[] =
    {
        Token(IF, "if"),
        Token(THEN, "then"),
        Token(ELSE, "else"),
        Token(END, "end"),
        Token(REPEAT, "repeat"),
        Token(UNTIL, "until"),
        Token(READ, "read"),
        Token(WRITE, "write")};
const int num_reserved_words = sizeof(reserved_words) / sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[] =
    {
        Token(ASSIGN, ":="),
        Token(EQUAL, "="),
        Token(LESS_THAN, "<"),
        Token(PLUS, "+"),
        Token(MINUS, "-"),
        Token(TIMES, "*"),
        Token(DIVIDE, "/"),
        Token(POWER, "^"),
        Token(SEMI_COLON, ";"),
        Token(LEFT_PAREN, "("),
        Token(RIGHT_PAREN, ")"),
        Token(LEFT_BRACE, "{"),
        Token(RIGHT_BRACE, "}")};
const int num_symbolic_tokens = sizeof(symbolic_tokens) / sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch) { return (ch >= '0' && ch <= '9'); }
inline bool IsLetter(char ch) { return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')); }
inline bool IsLetterOrUnderscore(char ch) { return (IsLetter(ch) || ch == '_'); }

////////////////////////////////////////////////////////////////////////////////////
// Solution /////////////////////////////////////////////////////////////////////////

int getSymbolicTokenIdx(char *tokenStr)
{
    for (int i = 0; i < num_symbolic_tokens; i++)
        if (StartsWith(tokenStr, symbolic_tokens[i].str))
            return i;

    return -1;
}

int getSymbolicTokenIdx(TokenType type)
{
    for (int i = 0; i < num_symbolic_tokens; i++)
        if (symbolic_tokens[i].type == type)
            return i;

    return -1;
}

int getReservedWordTokenIdx(char *word)
{
    for (int i = 0; i < num_reserved_words; i++)
        if (Equals(word, reserved_words[i].str))
            return i;

    return -1;
}

Token GetNextToken(CompilerInfo *compiler)
{
    char *tokenStr = compiler->in_file.GetNextTokenStr();

    if (!tokenStr)
        return Token(ENDFILE, "");

    Token token;

    int symbolicTokenIdx = getSymbolicTokenIdx(tokenStr);

    if (symbolicTokenIdx != -1)
    {
        Token symbolicToken = symbolic_tokens[symbolicTokenIdx];

        if (symbolicToken.type == LEFT_BRACE)
        {
            compiler->in_file.Advance(strlen(symbolicToken.str));

            int rightBraceIdx = getSymbolicTokenIdx(RIGHT_BRACE);

            if (rightBraceIdx != -1 && compiler->in_file.SkipUpto(symbolic_tokens[rightBraceIdx].str))
                return GetNextToken(compiler);
        }

        token.type = symbolicToken.type;
        Copy(token.str, symbolicToken.str);
    }
    else if (IsLetterOrUnderscore(tokenStr[0]))
    {
        int invalidLetterOrUnderscorePosition = 1;

        while (IsLetterOrUnderscore(tokenStr[invalidLetterOrUnderscorePosition]))
            invalidLetterOrUnderscorePosition++;

        Copy(token.str, tokenStr, invalidLetterOrUnderscorePosition);

        int reservedWordTokenIdx = getReservedWordTokenIdx(token.str);

        if (reservedWordTokenIdx != -1)
            token.type = reserved_words[reservedWordTokenIdx].type;
        else
            token.type = ID;
    }

    if (strlen(token.str) > 0)
        compiler->in_file.Advance(strlen(token.str));

    return token;
}

int main(int argc, char const *argv[])
{
    CompilerInfo *compilerInfo = new CompilerInfo("./input.txt", "./output.txt", "./debug.txt");
    Token next_token = GetNextToken(compilerInfo);
    int lineNum = compilerInfo->in_file.cur_line_num;
    const char *strPtr = next_token.str;
    cout << '[' << lineNum << "] " << strPtr << " (" << TokenTypeStr[next_token.type] << ")";

    return 0;
}