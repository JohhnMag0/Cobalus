#include "error_log.h"
#include "exec.h" 
#include "vcm.h"

// Compare values
int TypesMatch(int R, int L) {
    if (R == 0 && L == 2) {
        return 0;
    }
    if (R == 2 && L == 0) {
        return 0;
    }
    if (R == 1 && L == 2) {
        return 0;
    }
    if (R == 2 && L == 1) {
        return 0;
    }
    return 1;
}


int Calculus::EmptyStack() {
    if (Calc.empty()) {
        PushError("", "illegal instruction stack of execution is empty", 2);        
        return 1;
    }
    return 0;
}

void Calculus::PushCalc(Value byte) {
    #ifdef DEBUG
        switch(byte.index()) {
            case 0:{
                printf("  %g\n", std::get<double>(byte));
                break;
            }
            case 1: {
                printf("  %d\n", std::get<bool>(byte));
                break;
            }
            case 2: {
                printf("  %s\n", std::get<std::string>(byte).c_str());
                break;
            }
            case 4:{
                printf("  null\n");
                break;
            }
            default: {
                printf("ERROR\n");
                break;
            }
        }
    #endif
    Calc.push_back(byte);
}

// double + double
// strint + string
void Calculus::addData() {
    if (EmptyStack()) {
        return;
    }

    Value Right = Calc.back();
    Calc.pop_back();

    Value Left = Calc.back();
    Calc.pop_back();

    if (Right.index() != Left.index()) {
        PushError("", "types don't match", 2);        
        return;
    } 

    #ifdef DEBUG
    if(!Right.index()){
        printf("  %g\n", std::get<double>(Left));
        printf("  +\n");
        printf("  %g\n", std::get<double>(Right));
    }
    if (Right.index() == 2) {
        printf("  %s\n", std::get<std::string>(Left).c_str());
        printf("  +\n");
        printf("  %s\n", std::get<std::string>(Right).c_str());
    }
    #endif

    if (!Left.index()) {
        Right = std::get<double>(Left) + std::get<double>(Right);
        Calc.push_back(Right);
        return;
    }
    if (Left.index() == 2) {
        Right = std::get<std::string>(Left) + std::get<std::string>(Right);
        Calc.push_back(Right);
        return;
    }
}

// double - double
void Calculus::subData() {
    if (EmptyStack()) {
        return;
    }

    Value Right = Calc.back();
    Calc.pop_back();

    Value Left = Calc.back();
    Calc.pop_back();

    if (Right.index() != Left.index()) {
        PushError("", "types don't match", 2);        
        return;
    }
    if (Right.index() == 2 || Left.index() == 2) {
        PushError("", "illegal instruction in strings", 2);        
        return;
    }

    #ifdef DEBUG 
        printf("  %g\n", std::get<double>(Left));
        printf("  -\n");
        printf("  %g\n", std::get<double>(Right));
    #endif
    
    Right = std::get<double>(Left) - std::get<double>(Right);
    Calc.push_back(Right);
}

// double * double
void Calculus::mulData() {
    if (EmptyStack()) {
        return;
    }

    Value Right = Calc.back();
    Calc.pop_back();

    Value Left = Calc.back();
    Calc.pop_back();
    
    if (Right.index() != Left.index()) {
        PushError("", "types don't match", 2);        
        return;
    }
    if (Right.index() == 2 || Left.index() == 2) {
        PushError("", "illegal instruction in strings", 2);        
        return;
    }

    #ifdef DEBUG 
        printf("  %g\n", std::get<double>(Left));
        printf("  *\n");
        printf("  %g\n", std::get<double>(Right));
    #endif
    
    Right = std::get<double>(Left) * std::get<double>(Right);
    Calc.push_back(Right);
}

// double / double
void Calculus::divData() {
    if (EmptyStack()) {
        return;
    }

    Value Right = Calc.back();
    Calc.pop_back();

    Value Left = Calc.back();
    Calc.pop_back();
    
    if (Right.index() != Left.index()) {
        PushError("", "types don't match", 2);        
        return;
    }
    if (Right.index() == 2 || Left.index() == 2) {
        PushError("", "illegal instruction in strings", 2);        
        return;
    }

    #ifdef DEBUG 
        printf("  %g\n", std::get<double>(Left));
        printf("  /\n");
        printf("  %g\n", std::get<double>(Right));
    #endif
    
    Right = std::get<double>(Left) / std::get<double>(Right);
    Calc.push_back(Right);
}

// - double
void Calculus::invsigData() {
    if (EmptyStack()) {
        return;
    }

    Value Expr = Calc.back();
    Calc.pop_back();
   
    if (Expr.index() == 1) {
        PushError("", "illegal instruction on booleans", 2);        
        return;
    }
    if (Expr.index() == 2) {
        PushError("", "illegal instruction in strings", 2);        
        return;
    }
    
    #ifdef DEBUG 
        printf("  -\n");
        printf("  %g\n", std::get<double>(Expr));
    #endif

    Calc.push_back(-std::get<double>(Expr));
}

// ! double
// ! bool
void Calculus::negData() {
    if (EmptyStack()) {
        return;
    }

    Value Expr = Calc.back();
    Calc.pop_back();
    
    if (Expr.index() == 2) {
        PushError("", "illegal instruction in strings", 2);        
        return;
    }
    
    #ifdef DEBUG 
    if (!Expr.index()){
        printf("  !\n");
        printf("  %g\n", std::get<double>(Expr));
    }
    if (Expr.index() == 1) { 
        printf("  !\n");
        if (!std::get<bool>(Expr)){
            printf("  !\n");
            printf("  true\n");
        } else {
            printf("  !\n");
            printf("  false\n");
        }
    }
    #endif
    
    if (!Expr.index()){
        if(!std::get<double>(Expr)) {
            Value tmp = 1.0;
            Calc.push_back(tmp);
            return;
        } else {
            Value tmp = 0.0;
            Calc.push_back(tmp);
            return;
        }
    }
    if (Expr.index() == 1) {
        if(!std::get<bool>(Expr)) {
            Value tmp = true;
            Calc.push_back(tmp);
            return;
        } else {
            Value tmp = false;
            Calc.push_back(tmp);
            return;
        }
    }
}

void Calculus::eqData() {
    if (EmptyStack()) {
        return;
    }

    Value Right = Calc.back();
    Calc.pop_back();

    Value Left = Calc.back();
    Calc.pop_back();
    
    if (!TypesMatch(Left.index(), Right.index())) {
        PushError("", "types don't match", 2);        
        return;
    }

    #ifdef DEBUG 
     if (Right.index() == 2) {
        printf("  %s\n", std::get<std::string>(Left).c_str());
        printf("  ==\n");
        printf("  %s\n", std::get<std::string>(Right).c_str());
     }
     if (Right.index() == 1 && Left.index() == 1){
        printf("  %d\n", std::get<bool>(Left));
        printf("  ==\n");
        printf("  %d\n", std::get<bool>(Right));
     }
     if (Right.index() == 0 && Left.index() == 0) {
        printf("  %g\n", std::get<double>(Left));
        printf("  ==\n");
        printf("  %g\n", std::get<double>(Right));
     }
     if (Right.index() == 1 && Left.index() == 0) {
        printf("  %d\n", std::get<bool>(Left));
        printf("  ==\n");
        printf("  %g\n", std::get<double>(Right));
     }
     if (Right.index() == 0 && Left.index() == 1) {
        printf("  %g\n", std::get<double>(Left));
        printf("  ==\n");
        printf("  %d\n", std::get<bool>(Right));
     }
    #endif
    
    if (Right.index() == 0 && Left.index() == 0){
        Right = (std::get<double>(Left) == std::get<double>(Right));
        Calc.push_back(Right);
        return;
    }
    if (Right.index() == 2){
        Right = std::get<std::string>(Left) == std::get<std::string>(Right);
        Calc.push_back(Right);
        return;
    }
    if (Right.index() == 0 && Right.index() == 1){
        Right = std::get<double>(Left) == std::get<bool>(Right);
        Calc.push_back(Right);
        return;
    }
    if (Right.index() == 1 && Right.index() == 0){
        Right = std::get<bool>(Left) == std::get<double>(Right);
        Calc.push_back(Right);
        return;
    }
    if (Right.index() == 1 && Right.index() == 1){
        Right = std::get<bool>(Left) == std::get<bool>(Right);
        Calc.push_back(Right);
        return;
    }
    return;
}

void Calculus::PrintTop() {
    if (EmptyStack()) {
        return;
    }

    Value tmp = Calc.back();
    Calc.pop_back();
    
    switch(tmp.index()) {
        case 0: {
            printf("%g\n", std::get<double>(tmp));
            break;
        }
        case 1: {
            if(std::get<bool>(tmp)) {
                printf("true\n");
            } else {
                printf("false\n");
            }
            break;
        }
        case 2: {
            printf("'%s'\n", std::get<std::string>(tmp).c_str());
            break;
        }
        default: {
            printf("null\n");
            break;
        }
    }
}

