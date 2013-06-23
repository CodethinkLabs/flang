//===--- FormatSpec.h - Fortran Format Specifier ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the format specifier class, used by the PRINT statement,
//  et al.
//
//===----------------------------------------------------------------------===//

#ifndef FLANG_AST_FORMATSPEC_H__
#define FLANG_AST_FORMATSPEC_H__

#include "flang/Basic/SourceLocation.h"
#include "flang/Sema/Ownership.h"
#include "flang/Basic/LLVM.h"

namespace flang {

class ASTContext;

class FormatSpec {
protected:
  enum FormatType { FS_DefaultCharExpr, FS_Label, FS_Star };
private:
  FormatType ID;
  SourceLocation Loc;
protected:
  FormatSpec(FormatType id, SourceLocation L)
    : ID(id), Loc(L) {}
  friend class ASTContext;
public:
  SourceLocation getLocation() const { return Loc; }

  FormatType getFormatSpecID() const { return ID; }
  static bool classof(const FormatSpec *) { return true; }
};

class StarFormatSpec : public FormatSpec {
  StarFormatSpec(SourceLocation L);
public:
  static StarFormatSpec *Create(ASTContext &C, SourceLocation Loc);

  static bool classof(const StarFormatSpec *) { return true; }
  static bool classof(const FormatSpec *F) {
    return F->getFormatSpecID() == FS_Star;
  }
};

class DefaultCharFormatSpec : public FormatSpec {
  ExprResult Fmt;
  DefaultCharFormatSpec(SourceLocation L, ExprResult Fmt);
public:
  static DefaultCharFormatSpec *Create(ASTContext &C, SourceLocation Loc,
                                       ExprResult Fmt);

  ExprResult getFormat() const { return Fmt; }

  static bool classof(const DefaultCharFormatSpec *) { return true; }
  static bool classof(const FormatSpec *F) {
    return F->getFormatSpecID() == FS_DefaultCharExpr;
  }
};

class LabelFormatSpec : public FormatSpec {
  ExprResult Label;
  LabelFormatSpec(SourceLocation L, ExprResult Lbl);
public:
  static LabelFormatSpec *Create(ASTContext &C, SourceLocation Loc,
                                 ExprResult Lbl);

  ExprResult getLabel() const { return Label; }

  static bool classof(const LabelFormatSpec *) { return true; }
  static bool classof(const FormatSpec *F) {
    return F->getFormatSpecID() == FS_Label;
  }
};

} // end namespace flang

#endif
