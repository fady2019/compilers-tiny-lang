#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

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

bool StartsWith(const char *a, const char *b)
{
    int nb = strlen(b);
    return strncmp(a, b, nb) == 0;
}

bool Equals(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

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

int getReservedWordTokenIdx(const char *word)
{
    for (int i = 0; i < num_reserved_words; i++)
        if (Equals(word, reserved_words[i].str))
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

Token getNextToken(CompilerInfo *compiler);

Token coverSymbolicToken(CompilerInfo *compiler, int symbolicTokenIdx)
{
    Token symbolicToken = symbolic_tokens[symbolicTokenIdx];

    if (symbolicToken.type == LEFT_BRACE)
    {
        compiler->in_file.Advance(strlen(symbolicToken.str));

        int rightBraceIdx = getSymbolicTokenIdx(RIGHT_BRACE);

        if (rightBraceIdx != -1 && compiler->in_file.SkipUpto(symbolic_tokens[rightBraceIdx].str))
            return getNextToken(compiler);
        else
            return Token();
    }

    if (strlen(symbolicToken.str) > 0)
        compiler->in_file.Advance(strlen(symbolicToken.str));

    return Token(symbolicToken.type, symbolicToken.str);
}

void coverStringToken(char *tokenStr, Token &token)
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

void coverDigitToken(char *tokenStr, Token &token)
{
    int invalidDigitPosition = 1;

    while (IsDigit(tokenStr[invalidDigitPosition]))
        invalidDigitPosition++;

    Copy(token.str, tokenStr, invalidDigitPosition);
    token.type = NUM;
}

Token getNextToken(CompilerInfo *compiler)
{
    char *tokenStr = compiler->in_file.GetNextTokenStr();

    if (!tokenStr)
        return Token(ENDFILE, "");

    Token token;

    int symbolicTokenIdx = getSymbolicTokenIdx(tokenStr);

    if (symbolicTokenIdx != -1)
        return coverSymbolicToken(compiler, symbolicTokenIdx);
    else if (IsLetterOrUnderscore(tokenStr[0]))
        coverStringToken(tokenStr, token);
    else if (IsDigit(tokenStr[0]))
        coverDigitToken(tokenStr, token);

    if (strlen(token.str) > 0)
        compiler->in_file.Advance(strlen(token.str));

    return token;
}

Token coverSymbolicToken(CompilerInfo *compiler, int symbolicTokenIdx)
{
    Token symbolicToken = symbolic_tokens[symbolicTokenIdx];

    if (symbolicToken.type == LEFT_BRACE)
    {
        compiler->in_file.Advance(strlen(symbolicToken.str));

        int rightBraceIdx = getSymbolicTokenIdx(RIGHT_BRACE);

        if (rightBraceIdx != -1 && compiler->in_file.SkipUpto(symbolic_tokens[rightBraceIdx].str))
            return getNextToken(compiler);
        else
            return Token();
    }

    if (strlen(symbolicToken.str) > 0)
        compiler->in_file.Advance(strlen(symbolicToken.str));

    return Token(symbolicToken.type, symbolicToken.str);
}

class Lexer
{
private:
    InFile in_file;

public:
    Lexer(const char *filename) : in_file(filename) {}

    Token readNextToken()
    {
        char *tokenStr = in_file.GetNextTokenStr();

        if (!tokenStr)
            return Token(ENDFILE, "");

        Token token;

        int symbolicTokenIdx = getSymbolicTokenIdx(tokenStr);

        if (symbolicTokenIdx != -1)
            return coverSymbolicToken(this, symbolicTokenIdx);
        else if (IsLetterOrUnderscore(tokenStr[0]))
            coverStringToken(tokenStr, token);
        else if (IsDigit(tokenStr[0]))
            coverDigitToken(tokenStr, token);

        if (strlen(token.str) > 0)
            in_file.Advance(strlen(token.str));

        return token;
    }
};

class Parser
{
private:
    Lexer lexer;

    void match(TokenType expectedType)
    {
        Token token = lexer.readNextToken();

        if (token.type != expectedType)
        {
            cerr << "Error: Unexpected token '" << token.str << "' of type " << token.type
                 << ". Expected type " << expectedType << endl;
            exit(EXIT_FAILURE);
        }
    }

public:
    Parser(const char *filename) : lexer(filename) {}

    // Parse the entire program
    void parseProgram()
    {
        parseStmtSeq();
    }

private:
    void parseStmtSeq()
    {
        parseStmt();
        while (lexer.readNextToken().type == SEMI_COLON)
        {
            match(SEMI_COLON);
            parseStmt();
        }
    }

    void parseNewExpr()
    {
        Token nextToken = lexer.readNextToken();

        if (nextToken.type == LEFT_PAREN)
        {
            match(LEFT_PAREN);
            parseMathExpr();
            match(RIGHT_PAREN);
        }
        else if (nextToken.type == NUM || nextToken.type == ID)
        {
            match(nextToken.type);
        }
        else
        {
            cerr << "Error: Unexpected token '" << nextToken.str << "' of type " << nextToken.type << " in factor." << endl;
            exit(EXIT_FAILURE);
        }
    }

    void parseFactor()
    {
        parseNewExpr();
        while (lexer.readNextToken().type == POWER)
        {
            match(POWER);
            parseNewExpr();
        }
    }

    void parseTerm()
    {
        parseFactor();
        while (lexer.readNextToken().type == TIMES || lexer.readNextToken().type == DIVIDE)
        {
            match(lexer.readNextToken().type);
            parseFactor();
        }
    }

    void parseMathExpr()
    {
        parseTerm();
        while (lexer.readNextToken().type == PLUS || lexer.readNextToken().type == MINUS)
        {
            match(lexer.readNextToken().type);
            parseTerm();
        }
    }

    void parseExpr()
    {
        parseMathExpr();
        if (lexer.readNextToken().type == LESS_THAN || lexer.readNextToken().type == EQUAL)
        {
            match(lexer.readNextToken().type);
            parseMathExpr();
        }
    }

    void parseRepeatStmt()
    {
        match(REPEAT);
        parseStmtSeq();
        match(UNTIL);
        parseExpr();
    }

    void parseReadStmt()
    {
        match(READ);
        match(ID);
    }

    void parseWriteStmt()
    {
        match(WRITE);
        parseExpr();
    }

    void parseStmt()
    {
        Token nextToken = lexer.readNextToken();
        switch (nextToken.type)
        {
            case IF:
                parseIfStmt();
                break;
            case REPEAT:
                parseRepeatStmt();
                break;
            case ID:
                parseAssignStmt();
                break;
            case READ:
                parseReadStmt();
                break;
            case WRITE:
                parseWriteStmt();
                break;
            default:
                cerr << "Error: Unexpected token '" << nextToken.str << "' of type " << nextToken.type
                     << " in statement." << endl;
                exit(EXIT_FAILURE);
        }
    }

    void parseIfStmt()
    {
        match(IF);
        parseExpr();
        match(THEN);
        parseStmtSeq();
        if (lexer.readNextToken().type == ELSE)
        {
            match(ELSE);
            parseStmtSeq();
        }
        match(END);
    }

    void parseAssignStmt()
    {
        match(ID);
        match(ASSIGN);
        parseExpr();
    }
};

int main()
{

    const char *inputFileName = "input.txt";

    Parser parser(inputFileName);
    parser.parseProgram();

    cout << "Parsing completed successfully." << endl;

    return 0;
}
