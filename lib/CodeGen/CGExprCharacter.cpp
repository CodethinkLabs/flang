//===--- CGExprCharacter.cpp - Emit LLVM Code for Character Exprs --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit Expr nodes with character types as LLVM code.
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "flang/AST/ASTContext.h"
#include "flang/AST/ExprVisitor.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include <string.h>

namespace flang {
namespace CodeGen {

#define MANGLE_CHAR_FUNCTION(Str, Type) \
  (Str "_char1")

class CharacterExprEmitter
  : public ConstExprVisitor<CharacterExprEmitter, CharacterValueTy> {
  CodeGenFunction &CGF;
  CGBuilderTy &Builder;
  llvm::LLVMContext &VMContext;
public:

  CharacterExprEmitter(CodeGenFunction &cgf);

  CharacterValueTy EmitExpr(const Expr *E);
  CharacterValueTy VisitCharacterConstantExpr(const CharacterConstantExpr *E);
  CharacterValueTy VisitVarExpr(const VarExpr *E);
  CharacterValueTy VisitReturnedValueExpr(const ReturnedValueExpr *E);
  CharacterValueTy VisitSubstringExpr(const SubstringExpr *E);
};

CharacterExprEmitter::CharacterExprEmitter(CodeGenFunction &cgf)
  : CGF(cgf), Builder(cgf.getBuilder()),
    VMContext(cgf.getLLVMContext()) {
}

CharacterValueTy CharacterExprEmitter::EmitExpr(const Expr *E) {
  return Visit(E);
}

CharacterValueTy CharacterExprEmitter::VisitCharacterConstantExpr(const CharacterConstantExpr *E) {
  return CharacterValueTy(Builder.CreateGlobalStringPtr(E->getValue()),
                          llvm::ConstantInt::get(CGF.getModule().SizeTy,
                                                 strlen(E->getValue())));
}

CharacterValueTy CharacterExprEmitter::VisitVarExpr(const VarExpr *E) {
  auto VD = E->getVarDecl();
  return CharacterValueTy(Builder.CreateConstInBoundsGEP2_32(CGF.GetVarPtr(VD), 0, 0),
                          CGF.GetCharacterTypeLength(VD->getType()));
}

CharacterValueTy CharacterExprEmitter::VisitReturnedValueExpr(const ReturnedValueExpr *E) {
  return CharacterValueTy(); //FIXME
}

CharacterValueTy CharacterExprEmitter::VisitSubstringExpr(const SubstringExpr *E) {
  auto Str = EmitExpr(E->getTarget());
  return Str;//FIXME
}

void CodeGenFunction::EmitCharacterAssignment(const Expr *LHS, const Expr *RHS) {
  auto CharType = getContext().CharacterTy;
  auto Dest = EmitCharacterExpr(LHS);
  if(isa<BinaryExpr>(RHS)) {
    // '//'
  }
  else if(isa<CallExpr>(RHS)) {
    // can store the result directly in LHS
  }
  auto Src = EmitCharacterExpr(RHS);

  auto Func = CGM.GetRuntimeFunction2(MANGLE_CHAR_FUNCTION("assignment", CharType),
                                      CharType, CharType);
  EmitCall2(Func, Dest, Src);
}

llvm::Value *CodeGenFunction::GetCharacterTypeLength(QualType T) {
  llvm::ConstantInt::get(CGM.SizeTy,
                         getTypes().GetCharacterTypeLength(T));
}

CharacterValueTy CodeGenFunction::EmitCharacterExpr(const Expr *E) {
  CharacterExprEmitter EV(*this);
  return EV.EmitExpr(E);
}

llvm::Value *CodeGenFunction::CreateCharacterAggregate(CharacterValueTy Value) {
  llvm::Value *Result = llvm::UndefValue::get(
                          getTypes().GetCharacterType(Value.Ptr->getType()));
  Result = Builder.CreateInsertValue(Result, Value.Ptr, 0, "ptr");
  return Builder.CreateInsertValue(Result, Value.Len, 1, "len");
}


}
} // end namespace flang