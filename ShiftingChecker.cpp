//=- ShiftingChecker.cpp --------------------------*- C++ -*-==//
// This checker implements rule 04.INT34-C by "SEI CERT C Coding Standard":
// Do not shift an expression by a negative number of bits or by greater than or
// equal to the number of bits that exist in the operand.
//===----------------------------------------------------------------------===//
//

#include "Taint.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;
using namespace taint;

namespace {
class ShiftingChecker : public Checker<check::PreStmt<BinaryOperator>> {
  mutable std::unique_ptr<BuiltinBug> BT;
  void reportBug(const std::string &Msg, CheckerContext &C,
                 std::unique_ptr<BugReporterVisitor> Visitor = nullptr) const;

public:
  void checkPreStmt(const BinaryOperator *B, CheckerContext &C) const;
};
} // end anonymous namespace

static const Expr *getRHSExpr(const ExplodedNode *N) {
  const Stmt *S = N->getLocationAs<PreStmt>()->getStmt();
  if (const auto *BE = dyn_cast<BinaryOperator>(S))
    return BE->getRHS();
  return nullptr;
}

void ShiftingChecker::reportBug(
    const std::string &Msg, CheckerContext &C,
    std::unique_ptr<BugReporterVisitor> Visitor) const {
  if (ExplodedNode *N = C.generateNonFatalErrorNode()) {
    if (!BT)
      BT.reset(new BuiltinBug(this, "Invalid shift operation"));

    auto R = std::make_unique<PathSensitiveBugReport>(*BT, Msg.c_str(), N);
    R->addVisitor(std::move(Visitor));
    bugreporter::trackExpressionValue(N, getRHSExpr(N), *R);
    C.emitReport(std::move(R));
  }
}

void ShiftingChecker::checkPreStmt(const BinaryOperator *B,
                                   CheckerContext &C) const {
  BinaryOperator::Opcode Op = B->getOpcode();

  // If B isn't a shifting operation, leave.
  if (Op != BO_Shl && Op != BO_Shr && Op != BO_ShlAssign && Op != BO_ShrAssign)
    return;

  // When B is a shifting operator,
  // check for negative shifting or
  // shifting with overflow.
  Expr *RightSideExpr = B->getRHS();
  QualType TypeOfRHS = B->getRHS()->getType();

  if (!TypeOfRHS->isScalarType())
    return;

  if (TypeOfRHS->isSignedIntegerType() && C.isNegative(RightSideExpr)) {
    reportBug("Shifting by a negative value", C);
    return;
  }

  if (!TypeOfRHS->isIntegerType() || !TypeOfRHS->isIntegerType())
    return;

  ASTContext &ACtx = C.getASTContext();

  const Expr *RHS = B->getRHS();
  const LocationContext *LC = C.getLocationContext();
  SVal RhsSVal = C.getState()->getSVal(RHS, LC);

  const Expr *LHS = B->getLHS();
  LHS = LHS->IgnoreImpCasts();

  // Extract type of the left hand side.
  const QualType TypeOfLHS = LHS->getType();

  if (TypeOfLHS.isNull())
    return;

  // Second paragraph in this rule rationale states following:
  // "The precision of an integer type is the number of bits it uses to
  // represent values, excluding any sign and padding bits. For unsigned integer
  // types, the width and the precision are the same; whereas for signed integer
  // types, the width is one greater than the precision."
  uint64_t TypePrecision =
    TypeOfLHS->isSignedIntegerType()
      ? ACtx.getTypeInfo(TypeOfLHS.getTypePtr()).Width - 1
      : ACtx.getTypeInfo(TypeOfLHS.getTypePtr()).Width;

  // Make symbolic integer value that represents maximum value which can be used
  // in shifting of the type of the left hand side.
  llvm::APInt TypePrecisionAsLLVMInteger = llvm::APInt(
      ACtx.getTypeInfo(TypeOfRHS.getTypePtr()).Width, TypePrecision);

  SValBuilder &SVB = C.getSValBuilder();
  nonloc::ConcreteInt MaxShiftingSVal =
      SVB.makeIntVal(clang::IntegerLiteral::Create(
          C.getASTContext(), TypePrecisionAsLLVMInteger, B->getRHS()->getType(),
          SourceLocation()));

  SVal InvalidShiftOperation =
      SVB.evalBinOp(C.getState(), BO_GE, RhsSVal, MaxShiftingSVal, ACtx.BoolTy);

  Optional<DefinedSVal> DV = InvalidShiftOperation.getAs<DefinedSVal>();

  if (!DV)
    return;

  ConstraintManager &CM = C.getConstraintManager();
  ProgramStateRef StateTrue, StateFalse;
  // StateTrue represents state in which symbolic value that represents valid
  // shifting operation is true
  // StateFalse represents state in which symbolic value that represents valid
  // shifting operation is false
  std::tie(StateTrue, StateFalse) = CM.assumeDual(C.getState(), *DV);

  if (!StateFalse) {
    assert(StateTrue);
    reportBug("Invalid shift operation. Type '" +
                  TypeOfLHS.getUnqualifiedType().getAsString() +
                  "' can't be shifted by more than '" +
                  std::to_string(TypePrecision - 1) + "' bits",
              C);
    return;
  }

  bool TaintedD = isTainted(C.getState(), *DV);
  if (StateFalse && StateTrue) {
    (TaintedD == true)
        ? reportBug("Shifing with a tainted value", C,
                    std::make_unique<taint::TaintBugVisitor>(*DV))
        : reportBug("Shifting with an undefined value", C);
    return;
  }
}

void ento::registerShiftingChecker(CheckerManager &mgr) {
  mgr.registerChecker<ShiftingChecker>();
}

bool ento::shouldRegisterShiftingChecker(const CheckerManager &mgr) {
  return true;
}
