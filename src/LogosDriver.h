#pragma once

#include <any>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include "LogosBaseVisitor.h"

using namespace antlr4;
using namespace llvm;

/**
 * LogosDriver - Compilador de Logica Proposicional a LLVM IR
 * 
 * Este driver implementa un visitor que recorre el AST generado por ANTLR4
 * y genera codigo LLVM IR correspondiente para operaciones logicas.
 * 
 * Operaciones soportadas:
 *   - AND (&&) : Conjuncion logica
 *   - OR  (||) : Disyuncion logica
 *   - XOR (^^) : Disyuncion exclusiva
 *   - NOT (!)  : Negacion logica
 *   - IMPLIES (=>) : Implicacion logica (p => q es equivalente a !p || q)
 *   - IFF (<=>) : Bicondicional (p <=> q es equivalente a (p => q) && (q => p))
 */
class LogosDriver : public LogosBaseVisitor {
private:
  // Tabla de simbolos para variables
  std::map<std::string, Value*> symbols;
  
  // Conjunto de constantes (no pueden ser reasignadas)
  std::set<std::string> constants;

  // Componentes LLVM
  LLVMContext context;
  std::unique_ptr<Module> module;
  std::unique_ptr<IRBuilder<>> irBuilder;
  
  // Para printf
  Value *formatStrTrue;
  Value *formatStrFalse;
  Value *formatStrNewline;
  FunctionCallee printfFunc;
  Function *mainFunc;
  
  // Configuracion
  bool enableOptimizations;
  bool enableJIT;
  
  // Pass managers para optimizaciones
  std::unique_ptr<FunctionPassManager> TheFPM;
  std::unique_ptr<LoopAnalysisManager> TheLAM;
  std::unique_ptr<FunctionAnalysisManager> TheFAM;
  std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
  std::unique_ptr<ModuleAnalysisManager> TheMAM;
  std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
  std::unique_ptr<StandardInstrumentations> TheSI;

public:
  LogosDriver(bool optimize = false, bool jit = false)
      : module(std::make_unique<Module>("Logos", context)),
        irBuilder(std::make_unique<IRBuilder<>>(context)),
        enableOptimizations(optimize),
        enableJIT(jit) {
    
    if (enableOptimizations) {
      TheFPM = std::make_unique<FunctionPassManager>();
      TheLAM = std::make_unique<LoopAnalysisManager>();
      TheFAM = std::make_unique<FunctionAnalysisManager>();
      TheCGAM = std::make_unique<CGSCCAnalysisManager>();
      TheMAM = std::make_unique<ModuleAnalysisManager>();
      ThePIC = std::make_unique<PassInstrumentationCallbacks>();
      TheSI = std::make_unique<StandardInstrumentations>(context, true);
      TheSI->registerCallbacks(*ThePIC, TheMAM.get());

      // Agregar passes de optimizacion
      TheFPM->addPass(InstCombinePass());
      TheFPM->addPass(ReassociatePass());
      TheFPM->addPass(GVNPass());
      TheFPM->addPass(SimplifyCFGPass());

      PassBuilder PB;
      PB.registerModuleAnalyses(*TheMAM);
      PB.registerFunctionAnalyses(*TheFAM);
      PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
    }
  }

  // ========================================
  // VISITOR: Program
  // ========================================
  virtual std::any visitProgram(LogosParser::ProgramContext *ctx) override {
    // Crear funcion main
    mainFunc = Function::Create(
        FunctionType::get(Type::getInt32Ty(context), false),
        Function::ExternalLinkage, "main", *module);

    // Crear bloque de entrada
    irBuilder->SetInsertPoint(BasicBlock::Create(context, "entry", mainFunc));

    // Preparar printf
    formatStrTrue = irBuilder->CreateGlobalString("true", "str_true");
    formatStrFalse = irBuilder->CreateGlobalString("false", "str_false");
    formatStrNewline = irBuilder->CreateGlobalString("\n", "str_newline");
    
    FunctionType *printfType = FunctionType::get(
        Type::getInt32Ty(context),
        {PointerType::getUnqual(context)},
        true);
    printfFunc = module->getOrInsertFunction("printf", printfType);

    // Visitar todos los statements
    visitChildren(ctx);

    // Crear bloque de salida
    auto exitBB = BasicBlock::Create(context, "exit", mainFunc);
    irBuilder->CreateBr(exitBB);
    irBuilder->SetInsertPoint(exitBB);
    irBuilder->CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0));

    // Aplicar optimizaciones si estan habilitadas
    if (enableOptimizations) {
      TheFPM->run(*mainFunc, *TheFAM);
    }

    // Ejecutar con JIT o imprimir IR
    if (enableJIT) {
      executeJIT();
    } else {
      outs() << *module;
    }

    return std::any();
  }

  // ========================================
  // JIT Execution
  // ========================================
  void executeJIT() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    
    errs() << "; Verificando modulo... ";
    if (verifyModule(*module)) {
      errs() << "Error en el modulo!\n";
      return;
    }
    errs() << "OK\n";
    
    errs() << "; Modulo LLVM generado:\n\n" << *module << "\n";
    
    std::string errStr;
    ExecutionEngine *EE =
        EngineBuilder(std::move(module)).setErrorStr(&errStr).create();
    
    if (!EE) {
      errs() << "Error creando ExecutionEngine: " << errStr << "\n";
      return;
    }
    
    errs() << "\n; Ejecutando con JIT...\n";
    errs() << "; ========================\n";
    errs() << "; SALIDA DEL PROGRAMA:\n";
    errs() << "; ========================\n";
    
    std::vector<GenericValue> noargs;
    GenericValue GV = EE->runFunction(mainFunc, noargs);
    
    errs() << "; ========================\n";
    errs() << "; Codigo de retorno: " << GV.IntVal << "\n";
  }

  // ========================================
  // STATEMENTS
  // ========================================
  
  virtual std::any visitStmtVarDecl(LogosParser::StmtVarDeclContext *ctx) override {
    return visit(ctx->varDecl());
  }
  
  virtual std::any visitStmtConstDecl(LogosParser::StmtConstDeclContext *ctx) override {
    return visit(ctx->constDecl());
  }
  
  virtual std::any visitStmtAssign(LogosParser::StmtAssignContext *ctx) override {
    return visit(ctx->assignStmt());
  }
  
  virtual std::any visitStmtPrint(LogosParser::StmtPrintContext *ctx) override {
    return visit(ctx->printStmt());
  }
  
  virtual std::any visitStmtIf(LogosParser::StmtIfContext *ctx) override {
    return visit(ctx->ifStmt());
  }
  
  virtual std::any visitStmtWhile(LogosParser::StmtWhileContext *ctx) override {
    return visit(ctx->whileStmt());
  }
  
  virtual std::any visitStmtExpr(LogosParser::StmtExprContext *ctx) override {
    return visit(ctx->exprStmt());
  }
  
  virtual std::any visitVarDecl(LogosParser::VarDeclContext *ctx) override {
    std::string varName = ctx->ID()->getText();
    Value *val = std::any_cast<Value*>(visit(ctx->expr()));
    symbols[varName] = val;
    return std::any();
  }
  
  virtual std::any visitConstDecl(LogosParser::ConstDeclContext *ctx) override {
    std::string constName = ctx->ID()->getText();
    Value *val = std::any_cast<Value*>(visit(ctx->expr()));
    symbols[constName] = val;
    constants.insert(constName);
    return std::any();
  }
  
  virtual std::any visitAssignStmt(LogosParser::AssignStmtContext *ctx) override {
    std::string varName = ctx->ID()->getText();
    
    // Verificar que no sea una constante
    if (constants.find(varName) != constants.end()) {
      errs() << "Error: No se puede reasignar la constante '" << varName << "'\n";
      return std::any();
    }
    
    Value *val = std::any_cast<Value*>(visit(ctx->expr()));
    symbols[varName] = val;
    return std::any();
  }
  
  virtual std::any visitPrintStmt(LogosParser::PrintStmtContext *ctx) override {
    Value *val = std::any_cast<Value*>(visit(ctx->expr()));
    
    // Crear bloques para el if-else de impresion
    BasicBlock *thenBB = BasicBlock::Create(context, "print_true", mainFunc);
    BasicBlock *elseBB = BasicBlock::Create(context, "print_false", mainFunc);
    BasicBlock *mergeBB = BasicBlock::Create(context, "print_end", mainFunc);
    
    // Branch condicional basado en el valor
    irBuilder->CreateCondBr(val, thenBB, elseBB);
    
    // Bloque true
    irBuilder->SetInsertPoint(thenBB);
    irBuilder->CreateCall(printfFunc, {formatStrTrue});
    irBuilder->CreateBr(mergeBB);
    
    // Bloque false
    irBuilder->SetInsertPoint(elseBB);
    irBuilder->CreateCall(printfFunc, {formatStrFalse});
    irBuilder->CreateBr(mergeBB);
    
    // Bloque merge - imprimir newline
    irBuilder->SetInsertPoint(mergeBB);
    irBuilder->CreateCall(printfFunc, {formatStrNewline});
    
    return std::any();
  }
  
  virtual std::any visitIfStmt(LogosParser::IfStmtContext *ctx) override {
    Value *cond = std::any_cast<Value*>(visit(ctx->expr()));
    
    BasicBlock *thenBB = BasicBlock::Create(context, "if_then", mainFunc);
    BasicBlock *elseBB = ctx->block().size() > 1 
        ? BasicBlock::Create(context, "if_else", mainFunc) 
        : nullptr;
    BasicBlock *mergeBB = BasicBlock::Create(context, "if_end", mainFunc);
    
    if (elseBB) {
      irBuilder->CreateCondBr(cond, thenBB, elseBB);
    } else {
      irBuilder->CreateCondBr(cond, thenBB, mergeBB);
    }
    
    // Bloque then
    irBuilder->SetInsertPoint(thenBB);
    visit(ctx->block(0));
    irBuilder->CreateBr(mergeBB);
    
    // Bloque else (si existe)
    if (elseBB) {
      irBuilder->SetInsertPoint(elseBB);
      visit(ctx->block(1));
      irBuilder->CreateBr(mergeBB);
    }
    
    irBuilder->SetInsertPoint(mergeBB);
    return std::any();
  }
  
  virtual std::any visitWhileStmt(LogosParser::WhileStmtContext *ctx) override {
    BasicBlock *condBB = BasicBlock::Create(context, "while_cond", mainFunc);
    BasicBlock *bodyBB = BasicBlock::Create(context, "while_body", mainFunc);
    BasicBlock *endBB = BasicBlock::Create(context, "while_end", mainFunc);
    
    irBuilder->CreateBr(condBB);
    
    // Bloque de condicion
    irBuilder->SetInsertPoint(condBB);
    Value *cond = std::any_cast<Value*>(visit(ctx->expr()));
    irBuilder->CreateCondBr(cond, bodyBB, endBB);
    
    // Bloque del cuerpo
    irBuilder->SetInsertPoint(bodyBB);
    visit(ctx->block());
    irBuilder->CreateBr(condBB);
    
    irBuilder->SetInsertPoint(endBB);
    return std::any();
  }
  
  virtual std::any visitBlock(LogosParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }
  
  virtual std::any visitExprStmt(LogosParser::ExprStmtContext *ctx) override {
    visit(ctx->expr());
    return std::any();
  }

  // ========================================
  // EXPRESSIONS - Operaciones Logicas
  // ========================================
  
  // AND: p && q
  virtual std::any visitExprAnd(LogosParser::ExprAndContext *ctx) override {
    Value *lhs = std::any_cast<Value*>(visit(ctx->expr(0)));
    Value *rhs = std::any_cast<Value*>(visit(ctx->expr(1)));
    Value *result = irBuilder->CreateAnd(lhs, rhs, "and_res");
    return std::any(result);
  }
  
  // OR: p || q
  virtual std::any visitExprOr(LogosParser::ExprOrContext *ctx) override {
    Value *lhs = std::any_cast<Value*>(visit(ctx->expr(0)));
    Value *rhs = std::any_cast<Value*>(visit(ctx->expr(1)));
    Value *result = irBuilder->CreateOr(lhs, rhs, "or_res");
    return std::any(result);
  }
  
  // XOR: p ^^ q
  virtual std::any visitExprXor(LogosParser::ExprXorContext *ctx) override {
    Value *lhs = std::any_cast<Value*>(visit(ctx->expr(0)));
    Value *rhs = std::any_cast<Value*>(visit(ctx->expr(1)));
    Value *result = irBuilder->CreateXor(lhs, rhs, "xor_res");
    return std::any(result);
  }
  
  // NOT: !p
  virtual std::any visitExprNot(LogosParser::ExprNotContext *ctx) override {
    Value *val = std::any_cast<Value*>(visit(ctx->expr()));
    Value *result = irBuilder->CreateNot(val, "not_res");
    return std::any(result);
  }
  
  // IMPLIES: p => q  (equivalente a !p || q)
  virtual std::any visitExprImplies(LogosParser::ExprImpliesContext *ctx) override {
    Value *lhs = std::any_cast<Value*>(visit(ctx->expr(0)));
    Value *rhs = std::any_cast<Value*>(visit(ctx->expr(1)));
    
    // p => q es equivalente a !p || q
    Value *notLhs = irBuilder->CreateNot(lhs, "impl_not");
    Value *result = irBuilder->CreateOr(notLhs, rhs, "impl_res");
    return std::any(result);
  }
  
  // IFF (Bicondicional): p <=> q  (equivalente a (p => q) && (q => p))
  // Tambien equivalente a !(p XOR q) o (p && q) || (!p && !q)
  virtual std::any visitExprIff(LogosParser::ExprIffContext *ctx) override {
    Value *lhs = std::any_cast<Value*>(visit(ctx->expr(0)));
    Value *rhs = std::any_cast<Value*>(visit(ctx->expr(1)));
    
    // p <=> q es equivalente a !(p XOR q)
    Value *xorResult = irBuilder->CreateXor(lhs, rhs, "iff_xor");
    Value *result = irBuilder->CreateNot(xorResult, "iff_res");
    return std::any(result);
  }
  
  // Parentesis
  virtual std::any visitExprParen(LogosParser::ExprParenContext *ctx) override {
    return visit(ctx->expr());
  }
  
  // Literal true
  virtual std::any visitExprTrue(LogosParser::ExprTrueContext *ctx) override {
    Value *val = ConstantInt::get(Type::getInt1Ty(context), 1);
    return std::any(val);
  }
  
  // Literal false
  virtual std::any visitExprFalse(LogosParser::ExprFalseContext *ctx) override {
    Value *val = ConstantInt::get(Type::getInt1Ty(context), 0);
    return std::any(val);
  }
  
  // Identificador (variable)
  virtual std::any visitExprId(LogosParser::ExprIdContext *ctx) override {
    std::string varName = ctx->ID()->getText();
    
    if (symbols.find(varName) != symbols.end()) {
      return std::any(symbols[varName]);
    } else {
      errs() << "Error: Variable no definida '" << varName << "'\n";
      // Retornar false como valor por defecto
      Value *val = ConstantInt::get(Type::getInt1Ty(context), 0);
      return std::any(val);
    }
  }
};
