#include "global.h"
#include "block.h"
#include <clocale>
#include <memory>

// Enum for definition of Blocks
// By definition the blocks of functions can only be implemented inside the 
// global block or the class block
enum BlockState {
    GLOBAL,
    FUNC,
    FUNCLOOP,
    LOOP,
    COMMON,
};

///////////////////////////////////////////////////////////////////////////////
/////////                           AST CLASS                         /////////
///////////////////////////////////////////////////////////////////////////////

// Declarations are the top of the grammar, everything falls in a declaration
class DeclarationAST {
    public:
        DeclarationAST() = default;
        virtual ~DeclarationAST() = default;
        virtual void codegen() = 0;
};

// Statements are the second class. Consider that every line will be a 
// statement
class StatementAST : public DeclarationAST {
    public:
        virtual void codegen() = 0;
};

// Expression will define numberic expressions and calls
class ExpressionAST : public StatementAST {
    public:
        virtual void codegen() = 0;
};

// Values
class DoubleAST : public ExpressionAST {
    double DoubleValue;
    public:
        DoubleAST(double DoubleValue) : DoubleValue(DoubleValue) {}
        
        void codegen() override;
};

class StringAST : public ExpressionAST {
    std::string StringValue;
    public:
        StringAST(std::string StringValue) : StringValue(StringValue) {}

        void codegen() override;
};

class BoolAST : public ExpressionAST {
    bool BoolValue;
    public:
        BoolAST(bool BoolValue) : BoolValue(BoolValue) {}

        void codegen() override;
};

class NullAST : public ExpressionAST {
    public:
        NullAST() {}

        void codegen() override;
};

// Define Binary operation
class OperationAST : public ExpressionAST {
    std::unique_ptr<DeclarationAST> LHS;
    std::unique_ptr<DeclarationAST> RHS;
    int Op;

    public:
        OperationAST(std::unique_ptr<DeclarationAST> LHS,
                     std::unique_ptr<DeclarationAST> RHS,
                     int Op) 
        : LHS(std::move(LHS)), RHS(std::move(RHS)), Op(Op) {}
    
        void codegen() override;
};

// Define Unary operation
class UnaryAST : public ExpressionAST {
    std::unique_ptr<DeclarationAST> Expr;
    int Op;

    public:
        UnaryAST(std::unique_ptr<DeclarationAST> Expr, int Op)
            : Expr(std::move(Expr)), Op(Op) {}

        void codegen() override;
};

// Built-in Function
class PrintAST : public StatementAST {
    std::unique_ptr<DeclarationAST> Expr;

    public:
        PrintAST(std::unique_ptr<DeclarationAST> Expr) 
            : Expr(std::move(Expr)) {}
        
        void codegen() override;
};

// Variable declaration
class VarDeclAST : public StatementAST {
    std::unique_ptr<DeclarationAST> Expr;
    std::shared_ptr<BlockAST> ParentBlock;
    std::string Variable;
    int Decl;

    public:
        VarDeclAST(std::string Variable, int Decl, 
                   std::unique_ptr<DeclarationAST> Expr, 
                   std::shared_ptr<BlockAST> ParentBlock) 
            : Expr(std::move(Expr)), ParentBlock(ParentBlock),
              Variable(Variable), Decl(Decl) {}
    
        void codegen() override;
};

// Variable value
class VarValAST : public StatementAST {
    std::shared_ptr<BlockAST> ParentBlock;
    std::string Variable;

    public:
        VarValAST(std::string Variable, std::shared_ptr<BlockAST> ParentBlock)
            : ParentBlock(ParentBlock), Variable(Variable) {}
        
        void codegen() override;
};

// Struct to implement inside the block
class InsideAST : public StatementAST {
    std::unique_ptr<DeclarationAST> Chain;
    std::unique_ptr<DeclarationAST> Exec;

    public:
        InsideAST(std::unique_ptr<DeclarationAST> Chain, 
                  std::unique_ptr<DeclarationAST> Exec)
            : Chain(std::move(Chain)), Exec(std::move(Exec)) {}
        
        void codegen() override;
};

class IfAST : public StatementAST {
    std::unique_ptr<DeclarationAST> Cond;
    std::unique_ptr<DeclarationAST> IfBlock;
    std::unique_ptr<DeclarationAST> ElseBlock;

    public:
        IfAST (std::unique_ptr<DeclarationAST> Cond,
               std::unique_ptr<DeclarationAST> IfBlock,
               std::unique_ptr<DeclarationAST> ElseBlock) 
        : Cond(std::move(Cond)), IfBlock(std::move(IfBlock)),
        ElseBlock(std::move(ElseBlock)) {}

        void codegen() override;
};

class WhileAST : public StatementAST {
    std::unique_ptr<DeclarationAST> Cond;
    std::unique_ptr<DeclarationAST> Loop;

    public:
        WhileAST(std::unique_ptr<DeclarationAST> Cond,
                std::unique_ptr<DeclarationAST> Loop)
        : Cond(std::move(Cond)), Loop(std::move(Loop)) {}

        void codegen() override;
};

class ForAST : public StatementAST {
        std::unique_ptr<DeclarationAST> Var;
        std::unique_ptr<DeclarationAST> Cond;
        std::unique_ptr<DeclarationAST> Iterator;
        std::unique_ptr<DeclarationAST> Loop;

        public:
            ForAST(std::unique_ptr<DeclarationAST> Var,
                   std::unique_ptr<DeclarationAST> Cond,
                   std::unique_ptr<DeclarationAST> Iterator,
                   std::unique_ptr<DeclarationAST> Loop)
                : Var(std::move(Var)), Cond(std::move(Cond)),
                  Iterator(std::move(Iterator)),
                  Loop(std::move(Loop)) {}

        void codegen() override;
};

class BreakAST : public StatementAST {
    public:
        BreakAST() {}

        void codegen() override;
};


class FunctionAST : public DeclarationAST {
    std::string Name;
    std::vector<std::string> Var;
    std::unique_ptr<DeclarationAST> Exec;
    std::shared_ptr<BlockAST> Env;
    std::shared_ptr<BlockAST> ParentBlock;

    public:
        FunctionAST(std::string Name,
                    std::shared_ptr<BlockAST> ParentBlock)
            : Name(Name), ParentBlock(ParentBlock) {}

        bool SetVar(std::string PlaceHolder) {
            if(std::find(Var.begin(), Var.end(), PlaceHolder) == Var.end()) {
                Var.push_back(PlaceHolder);
                return true;
            }
            return false;
        }

        void SetExec(std::unique_ptr<DeclarationAST> ExecBlock) {
            Exec = std::move(ExecBlock);
        }

        void SetEnv(std::shared_ptr<BlockAST> FuncEnv) {
            Env = FuncEnv;
        }

        int VarSize() {
            return Var.size();
        }

        void codegen() override;
};

class CallFuncAST : public StatementAST {
    std::string FuncName;
    std::vector<std::unique_ptr<DeclarationAST>> VarVal;
    std::shared_ptr<BlockAST> ParentBlock;

    public:
        CallFuncAST(std::string FuncName,
                    std::shared_ptr<BlockAST> ParentBlock)
            : FuncName(FuncName), ParentBlock(ParentBlock) {}

        void SetVar(std::unique_ptr<DeclarationAST> Val) {
            VarVal.push_back(std::move(Val));
        }

        void codegen() override;
};

class ReturnAST : public StatementAST {
    std::unique_ptr<DeclarationAST> RetVal;

    public:
        ReturnAST(std::unique_ptr<DeclarationAST> RetVal) : RetVal(std::move(RetVal)) {}

        void codegen() override;
};


///////////////////////////////////////////////////////////////////////////////
/////////                           FUNCTIONS                         /////////
///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<DeclarationAST> Parser(std::shared_ptr<BlockAST> GlobalAST);
