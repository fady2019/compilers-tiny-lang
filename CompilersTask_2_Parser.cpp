#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
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
// Scanner - Solution /////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

enum NodeKind
{
    IF_NODE,
    REPEAT_NODE,
    ASSIGN_NODE,
    READ_NODE,
    WRITE_NODE,
    OPER_NODE,
    NUM_NODE,
    ID_NODE
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *NodeKindStr[] =
    {
        "If", "Repeat", "Assign", "Read", "Write",
        "Oper", "Num", "ID"};

enum ExprDataType
{
    VOID,
    INTEGER,
    BOOLEAN
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *ExprDataTypeStr[] =
    {
        "Void", "Integer", "Boolean"};

#define MAX_CHILDREN 3

struct TreeNode
{
    TreeNode *child[MAX_CHILDREN];
    TreeNode *sibling; // used for sibling statements only

    NodeKind node_kind;

    union
    {
        TokenType oper;
        int num;
        char *id;
    };                           // defined for expression/int/identifier only
    ExprDataType expr_data_type; // defined for expression/int/identifier only

    int line_num;

    TreeNode()
    {
        int i;
        for (i = 0; i < MAX_CHILDREN; i++)
            child[i] = 0;
        sibling = 0;
        expr_data_type = VOID;
    }
};

struct ParseInfo
{
    Token next_token;
};

void PrintTree(TreeNode *node, int sh = 0)
{
    int i, NSH = 3;
    for (i = 0; i < sh; i++)
        printf(" ");

    printf("[%s]", NodeKindStr[node->node_kind]);

    if (node->node_kind == OPER_NODE)
        printf("[%s]", TokenTypeStr[node->oper]);
    else if (node->node_kind == NUM_NODE)
        printf("[%d]", node->num);
    else if (node->node_kind == ID_NODE || node->node_kind == READ_NODE || node->node_kind == ASSIGN_NODE)
        printf("[%s]", node->id);

    if (node->expr_data_type != VOID)
        printf("[%s]", ExprDataTypeStr[node->expr_data_type]);

    printf("\n");

    for (i = 0; i < MAX_CHILDREN; i++)
        if (node->child[i])
            PrintTree(node->child[i], sh + NSH);
    if (node->sibling)
        PrintTree(node->sibling, sh);
}

////////////////////////////////////////////////////////////////////////////////////
// Parser - Solution /////////////////////////////////////////////////////////////////////////

//!--> Grammar Rules Parser
class GrammarParser
{
private:
    CompilerInfo *compilerInfo;
    Token *token;

    void matchToken(TokenType expected_token_type)
    {
        if (this->token->type != expected_token_type)
        {
            ostringstream oss;
            oss << "It's expected to find \"" << TokenTypeStr[expected_token_type] << "\", But it's not found!";
            throw runtime_error(oss.str());
        }

        delete this->token;
        this->token = new Token(getNextToken(this->compilerInfo));
    }

    //^ newexpr -> ( mathexpr ) | number | identifier
    TreeNode *parseNewExpr()
    {
        TreeNode *treeNode = new TreeNode;

        if (this->token->type == NUM)
        {
            treeNode->node_kind = NUM_NODE;
            treeNode->num = atoi(this->token->str);
            treeNode->line_num = this->compilerInfo->in_file.cur_line_num;
            matchToken(this->token->type);
        }
        else if (this->token->type == ID)
        {
            treeNode->node_kind = ID_NODE;
            AllocateAndCopy(&treeNode->id, this->token->str);
            treeNode->line_num = this->compilerInfo->in_file.cur_line_num;
            matchToken(this->token->type);
        }
        else if (this->token->type == LEFT_PAREN)
        {
            matchToken(LEFT_PAREN);
            treeNode = parseMathExpr();
            matchToken(RIGHT_PAREN);
        }
        else
        {
            throw runtime_error("Unexpected Token!");
        }

        return treeNode;
    }

    //^ factor -> newexpr { ^ newexpr }    right associative
    TreeNode *parseFactor()
    {
        TreeNode *treeNode = parseNewExpr();

        if (this->token->type == POWER)
        {
            TreeNode *newTreeNode = new TreeNode;
            newTreeNode->node_kind = OPER_NODE;
            newTreeNode->oper = this->token->type;
            newTreeNode->line_num = this->compilerInfo->in_file.cur_line_num;

            newTreeNode->child[0] = treeNode;
            matchToken(this->token->type);
            newTreeNode->child[1] = parseFactor();

            return newTreeNode;
        }

        return treeNode;
    }

    //^ term -> factor { (*|/) factor }    left associative
    TreeNode *parseTerm()
    {
        TreeNode *treeNode = parseFactor();

        while (this->token->type == TIMES || this->token->type == DIVIDE)
        {
            TreeNode *newTreeNode = new TreeNode;
            newTreeNode->node_kind = OPER_NODE;
            newTreeNode->oper = this->token->type;
            newTreeNode->line_num = this->compilerInfo->in_file.cur_line_num;

            newTreeNode->child[0] = treeNode;
            matchToken(this->token->type);
            newTreeNode->child[1] = parseFactor();

            treeNode = newTreeNode;
        }

        return treeNode;
    }

    //^ mathexpr -> term { (+|-) term }    left associative
    TreeNode *parseMathExpr()
    {
        TreeNode *treeNode = parseTerm();

        while (this->token->type == PLUS || this->token->type == MINUS)
        {
            TreeNode *newTreeNode = new TreeNode;
            newTreeNode->node_kind = OPER_NODE;
            newTreeNode->oper = this->token->type;
            newTreeNode->line_num = this->compilerInfo->in_file.cur_line_num;

            newTreeNode->child[0] = treeNode;
            matchToken(this->token->type);
            newTreeNode->child[1] = parseTerm();

            treeNode = newTreeNode;
        }

        return treeNode;
    }

    //^ expr -> mathexpr [ (<|=) mathexpr ]
    TreeNode *parseExpr()
    {
        TreeNode *treeNode = parseMathExpr();

        if (this->token->type == EQUAL || this->token->type == LESS_THAN)
        {
            TreeNode *newTreeNode = new TreeNode;
            newTreeNode->node_kind = OPER_NODE;
            newTreeNode->oper = this->token->type;
            newTreeNode->line_num = this->compilerInfo->in_file.cur_line_num;

            newTreeNode->child[0] = treeNode;
            matchToken(this->token->type);
            newTreeNode->child[1] = parseMathExpr();

            return newTreeNode;
        }

        return treeNode;
    }

    //^ writestmt -> write expr
    TreeNode *parseWriteStmt()
    {
        TreeNode *treeNode = new TreeNode;
        treeNode->node_kind = WRITE_NODE;
        treeNode->line_num = this->compilerInfo->in_file.cur_line_num;

        matchToken(WRITE);
        treeNode->child[0] = parseExpr();

        return treeNode;
    }

    //^ readstmt -> read identifier
    TreeNode *parseReadStmt()
    {
        TreeNode *treeNode = new TreeNode;
        treeNode->node_kind = READ_NODE;
        treeNode->line_num = this->compilerInfo->in_file.cur_line_num;

        matchToken(READ);
        if (this->token->type == ID)
        {
            AllocateAndCopy(&treeNode->id, this->token->str);
        }
        matchToken(ID);

        return treeNode;
    }

    //^ assignstmt -> identifier := expr
    TreeNode *parseAssignStmt()
    {
        TreeNode *treeNode = new TreeNode;
        treeNode->node_kind = ASSIGN_NODE;
        treeNode->line_num = this->compilerInfo->in_file.cur_line_num;

        if (this->token->type == ID)
        {
            AllocateAndCopy(&treeNode->id, this->token->str);
        }
        matchToken(ID);
        matchToken(ASSIGN);
        treeNode->child[0] = parseExpr();

        return treeNode;
    }

    //^ repeatstmt -> repeat stmtseq until expr
    TreeNode *parseRepeatStmt()
    {
        TreeNode *treeNode = new TreeNode;
        treeNode->node_kind = REPEAT_NODE;
        treeNode->line_num = this->compilerInfo->in_file.cur_line_num;

        matchToken(REPEAT);
        treeNode->child[0] = parseStmtSeq();
        matchToken(UNTIL);
        treeNode->child[1] = parseExpr();

        return treeNode;
    }

    //^ ifstmt -> if exp then stmtseq [ else stmtseq ] end
    TreeNode *parseIfStmt()
    {
        TreeNode *treeNode = new TreeNode;
        treeNode->node_kind = IF_NODE;
        treeNode->line_num = this->compilerInfo->in_file.cur_line_num;

        matchToken(IF);
        treeNode->child[0] = parseExpr();
        matchToken(THEN);
        treeNode->child[1] = parseStmtSeq();
        if (this->token->type == ELSE)
        {
            matchToken(ELSE);
            treeNode->child[2] = parseStmtSeq();
        }
        matchToken(END);

        return treeNode;
    }

    //^ stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
    TreeNode *parseStmt()
    {
        switch (this->token->type)
        {
        case IF:
            return parseIfStmt();
        case REPEAT:
            return parseRepeatStmt();
        case ID:
            return parseAssignStmt();
        case READ:
            return parseReadStmt();
        case WRITE:
            return parseWriteStmt();
        default:
            throw runtime_error("Unexpected Token!");
        }

        return 0;
    }

    //^ stmtseq -> stmt { ; stmt }
    TreeNode *parseStmtSeq()
    {
        TreeNode *firstTreeNode = parseStmt();
        TreeNode *lastTreeNode = firstTreeNode;

        while (this->token->type != ENDFILE && this->token->type != END && this->token->type != ELSE && this->token->type != UNTIL)
        {
            matchToken(SEMI_COLON);
            TreeNode *nextTreeNode = parseStmt();
            lastTreeNode->sibling = nextTreeNode;
            lastTreeNode = nextTreeNode;
        }

        return firstTreeNode;
    }

public:
    GrammarParser(CompilerInfo *compilerInfo)
    {
        this->compilerInfo = compilerInfo;
    }

    //^ program -> stmtseq
    TreeNode *parseProgram()
    {
        this->token = new Token(getNextToken(this->compilerInfo));
        return parseStmtSeq();
    }

    ~GrammarParser()
    {
        delete this->token;
    }
};

//!--> Delete Tress
void deleteTree(TreeNode *treeRoot)
{
    if (treeRoot->node_kind == ID_NODE || treeRoot->node_kind == READ_NODE || treeRoot->node_kind == ASSIGN_NODE)
        if (treeRoot->id)
            delete[] treeRoot->id;

    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (treeRoot->child[i])
        {
            deleteTree(treeRoot->child[i]);
        }
    }

    if (treeRoot->sibling)
    {
        deleteTree(treeRoot->sibling);
    }

    delete treeRoot;
}

int main(int argc, char const *argv[])
{
    try
    {
        CompilerInfo *compilerInfo = new CompilerInfo("./input.txt", "./output.txt", "./debug.txt");
        GrammarParser grammarParser(compilerInfo);
        TreeNode *programParseTreeRoot = grammarParser.parseProgram();
        PrintTree(programParseTreeRoot);
        deleteTree(programParseTreeRoot);
        delete compilerInfo;
    }
    catch (const runtime_error &err)
    {
        cout << "\nError: " << err.what() << endl;
    }

    return 0;
}