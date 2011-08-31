//===--- ASTContext.cpp - Context to hold long-lived AST nodes ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements the ASTContext interface.
//
//===----------------------------------------------------------------------===//

#include "flang/AST/ASTContext.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/ErrorHandling.h"
using namespace fortran;

ASTContext::ASTContext(llvm::SourceMgr &SM)
  : SrcMgr(SM), LastSDM(0) {
  TUDecl = TranslationUnitDecl::Create(*this);
  InitBuiltinTypes();
}

ASTContext::~ASTContext() {
  // Release the DenseMaps associated with DeclContext objects.
  // FIXME: Is this the ideal solution?
  ReleaseDeclContextMaps();
}

void ASTContext::InitBuiltinType(QualType &R, BuiltinType::TypeSpec K) {
  BuiltinType *Ty = new (*this, TypeAlignment) BuiltinType(K);
  R = QualType(Ty, 0);
  Types.push_back(Ty);
}

void ASTContext::InitBuiltinTypes() {
  // [R404]
  InitBuiltinType(IntegerTy,         BuiltinType::Integer);
  InitBuiltinType(RealTy,            BuiltinType::Real);
  InitBuiltinType(DoublePrecisionTy, BuiltinType::DoublePrecision);
  InitBuiltinType(ComplexTy,         BuiltinType::Complex);
  InitBuiltinType(CharacterTy,       BuiltinType::Character);
  InitBuiltinType(LogicalTy,         BuiltinType::Logical);
}

QualType ASTContext::getBuiltinQualType(BuiltinType::TypeSpec TS) const {
  switch (TS) {
  default:
    assert(false && "Invalid type spec!");
    break;
  case BuiltinType::Integer:         return IntegerTy;
  case BuiltinType::Real:            return RealTy;
  case BuiltinType::DoublePrecision: return DoublePrecisionTy;
  case BuiltinType::Character:       return CharacterTy;
  case BuiltinType::Logical:         return LogicalTy;
  case BuiltinType::Complex:         return ComplexTy;
  }
  return QualType();
}

//===----------------------------------------------------------------------===//
//                   Type creation/memoization methods
//===----------------------------------------------------------------------===//

QualType ASTContext::getExtQualType(const Type *BaseType, Qualifiers Quals,
                                    Expr *KindSel) const {
  unsigned FastQuals = Quals.getFastQualifiers();
  Quals.removeFastQualifiers();

  // Check if we've already instantiated this type.
  llvm::FoldingSetNodeID ID;
  ExtQuals::Profile(ID, BaseType, Quals, KindSel);
  void *InsertPos = 0;
  if (ExtQuals *EQ = ExtQualNodes.FindNodeOrInsertPos(ID, InsertPos)) {
    assert(EQ->getQualifiers() == Quals);
    return QualType(EQ, FastQuals);
  }

  // If the base type is not canonical, make the appropriate canonical type.
  QualType Canon;
  if (!BaseType->isCanonicalUnqualified()) {
    SplitQualType CanonSplit = BaseType->getCanonicalTypeInternal().split();
    CanonSplit.second.addConsistentQualifiers(Quals);
    Canon = getExtQualType(CanonSplit.first, CanonSplit.second, KindSel);

    // Re-find the insert position.
    (void) ExtQualNodes.FindNodeOrInsertPos(ID, InsertPos);
  }

  ExtQuals *EQ = new (*this, TypeAlignment) ExtQuals(BaseType, Canon, Quals,
                                                     KindSel);
  ExtQualNodes.InsertNode(EQ, InsertPos);
  return QualType(EQ, FastQuals);
}

/// getBuiltinType - Return the uniqued reference to the type for an intrinsic
/// type.
QualType ASTContext::getBuiltinType(BuiltinType::TypeSpec TS, Selector Kind) {
  QualType Ty = getBuiltinQualType(TS);
  if (Kind.getKindExpr().get() == 0)
    return Ty;

  // FIXME: This is gross.
  return getExtQualType(Ty.getTypePtr(), Qualifiers(),Kind.getKindExpr().get());
}

/// getCharacterBuiltinType - Return the uniqued reference to the type for a
/// character type.
BuiltinType *ASTContext::getCharacterBuiltinType(Selector Len, Selector Kind) {
  // Unique pointers, to guarantee there is only one pointer of a particular
  // structure.
  llvm::FoldingSetNodeID ID;
  CharacterBuiltinType::Profile(ID, Len, Kind);

  void *InsertPos = 0;
  if (BuiltinType *BT = BuiltinTypes.FindNodeOrInsertPos(ID, InsertPos))
    return BT;

  CharacterBuiltinType *New = new (*this) CharacterBuiltinType(Len, Kind);
  Types.push_back(New);
  BuiltinTypes.InsertNode(New, InsertPos);
  return New;
}

/// getPointerType - Return the uniqued reference to the type for a pointer to
/// the specified type.
PointerType *ASTContext::getPointerType(const Type *Ty, unsigned NumDims) {
  // Unique pointers, to guarantee there is only one pointer of a particular
  // structure.
  llvm::FoldingSetNodeID ID;
  PointerType::Profile(ID, Ty, NumDims);

  void *InsertPos = 0;
  if (PointerType *PT = PointerTypes.FindNodeOrInsertPos(ID, InsertPos))
    return PT;

  PointerType *New = new (*this) PointerType(Ty, NumDims);
  Types.push_back(New);
  PointerTypes.InsertNode(New, InsertPos);
  return New;
}

/// getArrayType - Return the uniqued reference to the type for an array to the
/// specified type.
ArrayType *ASTContext::getArrayType(const Type *Ty,
                                    const llvm::SmallVectorImpl<Expr*> &Dims) {
  // Unique pointers, to guarantee there is only one pointer of a particular
  // structure.
  llvm::FoldingSetNodeID ID;
  ArrayType::Profile(ID, Ty, Dims);

  void *InsertPos = 0;
  if (ArrayType *AT = ArrayTypes.FindNodeOrInsertPos(ID, InsertPos))
    return AT;

  ArrayType *New = new (*this) ArrayType(Ty, Dims);
  Types.push_back(New);
  ArrayTypes.InsertNode(New, InsertPos);
  return New;
}

const VarDecl *ASTContext::getOrCreateVarDecl(llvm::SMLoc Loc,
                                              const DeclSpec *DS,
                                              const IdentifierInfo *Info) {
  // Unique pointers, to guarantee there is only one pointer of a particular
  // structure.
  llvm::FoldingSetNodeID ID;
  VarDecl::Profile(ID, Info);

  void *InsertPos = 0;
  if (VarDecl *VD = VariableDecls.FindNodeOrInsertPos(ID, InsertPos))
    return VD;

  VarDecl *New = new (*this) VarDecl(Loc, DS, Info);
  VariableDecls.InsertNode(New, InsertPos);
  return New;
}

const VarDecl *ASTContext::getVarDecl(const IdentifierInfo *Info) {
  // Unique pointers, to guarantee there is only one pointer of a particular
  // structure.
  llvm::FoldingSetNodeID ID;
  VarDecl::Profile(ID, Info);

  void *InsertPos = 0;
  return VariableDecls.FindNodeOrInsertPos(ID, InsertPos);
}

/// getTypeDeclTypeSlow - Return the unique reference to the type for the
/// specified type declaration.
QualType ASTContext::getTypeDeclTypeSlow(const TypeDecl *Decl) const {
  assert(Decl && "Passed null for Decl param");
  assert(!Decl->TypeForDecl && "TypeForDecl present in slow case");

  const RecordDecl *Record = dyn_cast<RecordDecl>(Decl);
  if (!Record) {
    llvm_unreachable("TypeDecl without a type?");
    return QualType(Decl->TypeForDecl, 0);
  }

  return getRecordType(Record);
}

QualType ASTContext::getRecordType(const RecordDecl *Decl) const {
  return QualType();
#if 0
  if (Decl->TypeForDecl) return QualType(Decl->TypeForDecl, 0);

  RecordType *newType = new (*this, TypeAlignment) RecordType(Decl);
  Decl->TypeForDecl = newType;
  Types.push_back(newType);
  return QualType(newType, 0);
#endif
}
