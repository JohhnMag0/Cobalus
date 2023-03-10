#include "Headers/error_log.h"
#include "Headers/lexer.h"
#include "Headers/parser.h"

// +++++++++++++++++++++++
// +-----+ GLOBALS +-----+
// +++++++++++++++++++++++

// Buffer for tokens and function to get then
int CurToken;
void getNextToken() {
    CurToken = Tokenizer();
}

// Returns the precedence of operations.
// 1 is the lowest
int getPrecedence() {
    switch(CurToken) {
        default: return -1;
        case TOKEN_ATR: return 2;
        case TOKEN_AND: return 3;
        case TOKEN_OR: return 3;
        case TOKEN_EQUAL: return 5;
        case TOKEN_INEQUAL: return 5;
        case TOKEN_GREATER: return 5;
        case TOKEN_LESS: return 5;
        case TOKEN_GREATEQ: return 5;
        case TOKEN_LESSEQ: return 5;
        case TOKEN_PLUS: return 10;
        case TOKEN_MINUS: return 10;
        case TOKEN_MUL: return 20;
        case TOKEN_DIV: return 20; // highest
    }
}

int isUnary() {
    if (CurToken == TOKEN_MINUS || CurToken ==  TOKEN_NOT) {
        return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/////////                           PARSER                            /////////
///////////////////////////////////////////////////////////////////////////////

// Forward definition
std::unique_ptr<DeclarationAST> ExpressionParser \
    (std::shared_ptr<BlockAST> CurBlock);
std::unique_ptr<DeclarationAST> StatementParser \
    (std::shared_ptr<BlockAST> CurBlock);
std::unique_ptr<DeclarationAST> IdParser\
(std::shared_ptr<BlockAST>);

// number -> double
std::unique_ptr<DeclarationAST> DoubleParser() {
    getNextToken(); // consume double
    return std::make_unique<DoubleAST>(DoubleBuffer);
}

// string
std::unique_ptr<DeclarationAST> StringParser() {
    getNextToken(); // consume string
    return std::make_unique<StringAST>(StringBuffer);
}

// bool
std::unique_ptr<DeclarationAST> BoolParser() {
    if (CurToken == TOKEN_TRUE) {
        getNextToken(); // consume bool
        return std::make_unique<BoolAST>(true);
    }
    getNextToken(); // consume bool
    return std::make_unique<BoolAST>(false);
}

// null
std::unique_ptr<DeclarationAST> NullParser() {
    getNextToken(); // consume null
    return std::make_unique<NullAST>();
}

// parenexpr -> '(' expression ')'
std::unique_ptr<DeclarationAST> ParenParser(std::shared_ptr<BlockAST> CurBlock) 
{
    getNextToken(); // consume '('
    
    auto Expr = ExpressionParser(CurBlock);
    if (!Expr) {
        return nullptr;
    }

    if (CurToken != ')') {
       ErLogs.PushError("", "expected a '('", 1); 
       return nullptr;
    }
    getNextToken(); // consume ')'
    return Expr;
}

// primary -> number
//         |  bool
//         |  string
//         |  null
//         |  parenexpr
//         |  idstmt
std::unique_ptr<DeclarationAST> PrimaryParser(std::shared_ptr<BlockAST> CurBlock) 
{
    switch(CurToken) {
        default:{
            ErLogs.PushError("", "expression not identified", 1); 
            return nullptr;
        }
        case TOKEN_DOUBLE:
            return DoubleParser();
        case TOKEN_STRING:
            return StringParser();
        case TOKEN_FALSE:
            return BoolParser();
        case TOKEN_TRUE:
            return BoolParser();
        case TOKEN_NULL:
            return NullParser();
        case '(':
            return ParenParser(CurBlock);
        case TOKEN_ID:
            return IdParser(CurBlock);
        case ';': {
            getNextToken(); // consume ';'
            return nullptr;
        }
    }
}

// unaryexpr -> '!'|'-' unary
//           |  primary
std::unique_ptr<DeclarationAST> UnaryParser(std::shared_ptr<BlockAST> CurBlock) 
{
    // If the current token is not a operator, it must be a primary
    if (!isUnary() || CurToken == '(' || CurToken == ',') {
        return PrimaryParser(CurBlock);
    }
    
    // If is a unary operator read it
    int Op = CurToken;
    getNextToken(); // consume '!'|'-'
    
    auto Operand = UnaryParser(CurBlock);
    if (!Operand) {
        return nullptr;
    }
    return std::make_unique<UnaryAST>(std::move(Operand), Op);
}

// operation -> number 
//           |  number '+' operation
std::unique_ptr<DeclarationAST> OperationParser(int PrecLHS, 
                                        std::unique_ptr<DeclarationAST> LHS,
                                        std::shared_ptr<BlockAST> CurBlock) 
{
    // Mounts the operation precedence in reverse polish
    while (true) {
        int PrecRHS = getPrecedence();

        // If the precedence is higher on the operation passed(LHS) return it.
        if (PrecRHS < PrecLHS) {
            return LHS;
        }

        // Precedence is higher on the right side. Save current operator
        int Op = CurToken;
        getNextToken(); // consume operator

        // Parse the RHS of the expression 
        auto RHS = UnaryParser(CurBlock);
        if (!RHS) {
            return nullptr;
        }

        // Gets the next operation
        int NextPrec = getPrecedence();

        // If the precedence of the next operation is higher than the current
        // one parses the RHS
        if (PrecRHS < NextPrec) {
            // Removing PrecRHS+1 if error is that
            RHS = OperationParser(PrecRHS+1, std::move(RHS), CurBlock);
            if (!RHS) {
                return nullptr;
            }
        }
    
        // Merge LHS/RHS
        LHS = std::make_unique<OperationAST>(std::move(LHS), std::move(RHS), 
                                            Op);
    }
}

// expression -> operation
std::unique_ptr<DeclarationAST> \
ExpressionParser(std::shared_ptr<BlockAST> CurBlock)
{
    auto LHS = UnaryParser(CurBlock);
    if (!LHS) {
        return nullptr;
    }

    return OperationParser(0, std::move(LHS), CurBlock);
}

// printstmt -> print parenexpr
std::unique_ptr<DeclarationAST> PrintParser(std::shared_ptr<BlockAST> CurBlock) 
{
    getNextToken(); // consume print
    
    if (CurToken != '(') {
       ErLogs.PushError("", "expected a '('", 1); 
       return nullptr;
    }
    
    auto Expr = ParenParser(CurBlock);
    if (!Expr) {
        return nullptr;
    }

    return std::make_unique<PrintAST>(std::move(Expr));
}

// vardecl -> var id '(' = expression ')'?
std::unique_ptr<DeclarationAST> \
VarDeclParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume var
    getNextToken(); // consume identifier

    std::string VarName = Identifier;

    if (CurToken != TOKEN_ATR) {
        return std::make_unique<VarDeclAST>(VarName, 1, nullptr,
                                            CurBlock);
    }

    getNextToken(); // consume '='
    auto Expr = ExpressionParser(CurBlock);
    if (!Expr) {
       ErLogs.PushError("", "expression was not reconized", 1);
       return nullptr;
    }
    return std::make_unique<VarDeclAST>(VarName, 1, std::move(Expr),
                                        CurBlock);
}

// varassign -> id = expression
std::unique_ptr<DeclarationAST>\
VarAssignParser(std::shared_ptr<BlockAST> CurBlock, std::string IdName)
{
    getNextToken(); // consume '='
    auto Expr = ExpressionParser(CurBlock);
    if (!Expr) {
       ErLogs.PushError("", "expression was not reconized", 1);
       return nullptr;
    }
    return std::make_unique<VarDeclAST>(IdName, 0, std::move(Expr),
                                        CurBlock);
}

// callfunc -> id( expression? )
std::unique_ptr<DeclarationAST>\
CallFuncParser(std::shared_ptr<BlockAST> CurBlock, std::string IdName)
{
    getNextToken(); // consume '('
    std::unique_ptr<CallFuncAST> Caller =
       std::make_unique<CallFuncAST>(IdName, CurBlock);

   while(true) {
       if(CurToken == ')') {
           getNextToken(); // consume ')'
           break;
       } else if (CurToken == ',') {
           getNextToken(); // consume ','
       }
       auto Expr = ExpressionParser(CurBlock);
       if(!Expr) {
           return nullptr;
       }
       Caller->SetVar(std::move(Expr));
   }

   return std::move(Caller);
}

// idstmt -> varassign
//        -> variable
//        -> callfunc
std::unique_ptr<DeclarationAST>\
IdParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume id
    std::string IdName = Identifier;

    if (CurToken == TOKEN_ATR) {
        auto Var = VarAssignParser(CurBlock, IdName);
        return Var;
    }
    if (CurToken == '(') {
        auto Call = CallFuncParser(CurBlock, IdName);
        return Call;
    }

    // If every thing fails is a variable
    return std::make_unique<VarValAST>(IdName, CurBlock);
}

// inside -> statement
std::unique_ptr<DeclarationAST> InsideParser(std::shared_ptr<BlockAST> CurBlock) {
    if (CurToken == '}') {
        return std::make_unique<NullAST>();
    }

    auto Stmt = StatementParser(CurBlock);
    if (CurToken == ';') {
        getNextToken(); // consume ';'
    }
    if (!Stmt) {
        return nullptr;
    }

    return std::make_unique<InsideAST>(InsideParser(CurBlock), 
                                       std::move(Stmt));
}

// block -> '{' inside '}'
std::unique_ptr<DeclarationAST> BlockParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume '{'

    std::shared_ptr<BlockAST> CodeBlock =
        std::make_shared<BlockAST>(CurBlock, COMMON);
    if (CurBlock->ReturnState() != GLOBAL) {
        CodeBlock->ChangeState(CurBlock->ReturnState());
    }

    auto Inside = InsideParser(CodeBlock);

    if (CurToken != '}') {
       ErLogs.PushError("", "expected a '}'", 1); 
       return nullptr;
    }
    getNextToken(); // consume '}'
    return Inside;
}

// ifstmt -> 'if' parenexpr statement '(' 'else' statement ')'?
std::unique_ptr<DeclarationAST> IfParser(std::shared_ptr<BlockAST> CurBlock) {
    getNextToken(); // consume if

    auto Cond = ParenParser(CurBlock);
    if (!Cond) {
        ErLogs.PushError("if", "expected a expression", 1);
    }

    auto IfBlock = StatementParser(CurBlock);
    if (!IfBlock) {
        return nullptr;
    }

    if (CurToken == ';') {
        getNextToken(); // consume ';'
    }

    if (CurToken == TOKEN_ELSE) {
        getNextToken(); // consume else
        auto ElseBlock = StatementParser(CurBlock);
        if (!ElseBlock) {
            return nullptr;
        }
        return std::make_unique<IfAST>(std::move(Cond), std::move(IfBlock),
            std::move(ElseBlock));
    }

    return std::make_unique<IfAST>(std::move(Cond), std::move(IfBlock), nullptr);
}

// whilestmt -> 'while' parenexpr stmt
std::unique_ptr<DeclarationAST> \
WhileParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume while

    auto Cond = ParenParser(CurBlock);
    if (!Cond) {
        ErLogs.PushError("while", "expected a expression", 1);
        return nullptr;
    } 

    // Block need to be in state of Loop
    int CurState = CurBlock->ReturnState(); // Saves the current state
    if (CurState == FUNC) {
        CurBlock->ChangeState(FUNCLOOP);
    } else {
        CurBlock->ChangeState(LOOP);
    }
    auto Loop = StatementParser(CurBlock);
    CurBlock->ChangeState(CurState); // return to the previous state

    return std::make_unique<WhileAST>(std::move(Cond), std::move(Loop));
}

// forstmt -> 'for' '(' statement ';' expression ';' expression ')' statement
std::unique_ptr<DeclarationAST> ForParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume 'for'

    if (CurToken != '(') {
        ErLogs.PushError("for", "expected a ')'", 1);
        return nullptr;
    }
    getNextToken(); // consume '('

    int CurState = CurBlock->ReturnState(); // Saves the current state
    if (CurState == FUNC) {
        CurBlock->ChangeState(FUNCLOOP);
    } else {
        CurBlock->ChangeState(LOOP);
    }

    auto Var = StatementParser(CurBlock);

    if (CurToken != ';') {
        ErLogs.PushError("for", "expected a ';'", 1);
        return nullptr;
    }
    getNextToken(); // consume ';'

    auto Cond = ExpressionParser(CurBlock);

    if (CurToken != ';') {
        ErLogs.PushError("for", "expected a ';'", 1);
        return nullptr;
    }
    getNextToken(); // consume ';'

    auto Interator = StatementParser(CurBlock);

    if (CurToken != ')') {
        ErLogs.PushError("for", "expected a ')'", 1);
        return nullptr;
    }
    getNextToken(); // consume ')'

    auto Loop = StatementParser(CurBlock);

    CurBlock->ChangeState(CurState); // return to the previous state

    return std::make_unique<ForAST>(std::move(Var), std::move(Cond),
                                    std::move(Interator),
                                    std::move(Loop));
}

// breakstmt -> break
std::unique_ptr<DeclarationAST>\
BreakParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume break
    if (CurBlock->ReturnState() != LOOP && CurBlock->ReturnState() != FUNCLOOP) {
        ErLogs.PushError("break", "found in a block without loop", 1);
        return nullptr;
    }
    return std::make_unique<BreakAST>();
}

// returstmt -> return expression?
std::unique_ptr<DeclarationAST>\
ReturnParser(std::shared_ptr<BlockAST> CurBlock) {
    getNextToken(); // consume return
    if (CurBlock->ReturnState() != FUNC && CurBlock->ReturnState() != FUNCLOOP) {
        ErLogs.PushError("return", "found in a block without func", 1);
        return nullptr;
    }
    auto Expr = ExpressionParser(CurBlock);
    return std::make_unique<ReturnAST>(std::move(Expr));
}

// statement -> printstmt
//           |  vardecl
//           |  block
//           |  ifstmt
//           |  whilestmt
//           |  forstmt
//           |  breakstmt
//           |  returnstmt
std::unique_ptr<DeclarationAST>\
StatementParser(std::shared_ptr<BlockAST> CurBlock)
{
    switch(CurToken) {
        case TOKEN_PRINT:
            return PrintParser(CurBlock);
        case TOKEN_VAR:
            return VarDeclParser(CurBlock);
        case TOKEN_ID:
            return IdParser(CurBlock); // Change this when function added
        case '{':
            return BlockParser(CurBlock);
        case TOKEN_IF:
            return IfParser(CurBlock);
        case TOKEN_WHILE:
            return WhileParser(CurBlock);
        case TOKEN_FOR:
            return ForParser(CurBlock);
        case TOKEN_BREAK:
            return BreakParser(CurBlock);
        case TOKEN_RET:
            return ReturnParser(CurBlock);
        default: {
            ErLogs.PushError(Identifier, "statement not identified", 1); 
            return nullptr;
        }
    }
}

// function -> func id '(' id? ')' stmt
std::unique_ptr<DeclarationAST>\
FunctionParser(std::shared_ptr<BlockAST> CurBlock)
{
    getNextToken(); // consume func
    if (CurBlock->ReturnState() != GLOBAL) {
        ErLogs.PushError("func", "inside another other block", 1);
    }

    if (CurToken != TOKEN_ID) {
        ErLogs.PushError("", "identifier of function not found", 1);
    }
    getNextToken(); // consume id
    std::string IdName = Identifier;

    if (CurBlock->funcGetOffset(IdName) != -1) {
        ErLogs.PushError("", "function already defined", 1);
    }

    if (CurToken != '(') {
        ErLogs.PushError("", "expected a '(' in func", 1);
        return nullptr;
    }
    getNextToken(); // consume '('

    std::shared_ptr<BlockAST> FuncBlock =
        std::make_shared<BlockAST>(CurBlock, FUNC);

    std::unique_ptr<FunctionAST> Func =
        std::make_unique<FunctionAST>(IdName, CurBlock);

    while(true) {
        if (CurToken == ')') {
            getNextToken(); // consume ')'
            break;
        } else if (CurToken == ',') {
            getNextToken(); // consume ','
        } else if (CurToken == TOKEN_ID) {
            getNextToken(); // consume id
            if (!Func->SetVar(Identifier)) {
                ErLogs.PushError("", "variable already defined", 1);
            }
        } else {
            ErLogs.PushError("", "function not properly defined", 1);
        }
    }

    auto FuncExec = StatementParser(FuncBlock);
    if (!FuncExec) {
        return nullptr;
    }

    Func->SetExec(std::move(FuncExec));
    Func->SetEnv(FuncBlock);

    return std::move(Func);
}

// declaration -> statement
//             |  expression
//             |  function
std::unique_ptr<DeclarationAST> \
DeclarationParser(std::shared_ptr<BlockAST> CurBlock)
{
    switch(CurToken) {
        case TOKEN_FUNC:
            return FunctionParser(CurBlock);
        case TOKEN_DOUBLE:
            return ExpressionParser(CurBlock);
        case TOKEN_NULL:
            return ExpressionParser(CurBlock);
        case TOKEN_STRING:
            return ExpressionParser(CurBlock);
        case TOKEN_TRUE:
            return ExpressionParser(CurBlock);
        case TOKEN_FALSE:
            return ExpressionParser(CurBlock);
        case '(':
            return ExpressionParser(CurBlock);
        case '{':
            return StatementParser(CurBlock);
        default:
            return StatementParser(CurBlock);
    }
}

// program -> declaration
std::unique_ptr<DeclarationAST> Parser(std::shared_ptr<BlockAST> Global) 
{
    if (CurToken == 0 || CurToken == ';'){
        getNextToken(); // Get the first token
    }

    if (CurToken == TOKEN_EOF) {
        return nullptr;
    }
    
    auto Program = DeclarationParser(Global);
    if (!Program) {
        return nullptr;
    }
    return Program;
}
