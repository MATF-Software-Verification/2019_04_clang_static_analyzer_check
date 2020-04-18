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
  void reportBug(const char *Msg, ProgramStateRef StateZero, CheckerContext &C,
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
    const char *Msg, ProgramStateRef StateZero, CheckerContext &C,
    std::unique_ptr<BugReporterVisitor> Visitor) const {
  if (ExplodedNode *N = C.generateErrorNode(StateZero)) {
    if (!BT)
      BT.reset(new BuiltinBug(this, "Invalid shift operation"));

    auto R = std::make_unique<PathSensitiveBugReport>(*BT, Msg, N);
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
    reportBug("Shifting by a negative value", C.getState(), C);
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

  uint64_t BitWidth = ACtx.getTypeInfo(TypeOfLHS.getTypePtr()).Width;

  // Make symbolic integer value that represents maximum value which can be used
  // in shifting of the type of the left hand side.
  llvm::APInt LLVMIntegerBitWidth = llvm::APInt(32, BitWidth);
  SValBuilder &SVB = C.getSValBuilder();
  nonloc::ConcreteInt MaxShiftingSVal = SVB.makeIntVal(
      clang::IntegerLiteral::Create(C.getASTContext(), LLVMIntegerBitWidth,
                                    B->getRHS()->getType(), SourceLocation()));

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
    reportBug("Invalid shift operation", StateFalse, C);
    return;
  }

  bool TaintedD = isTainted(C.getState(), *DV);
  if ((StateFalse && StateTrue && TaintedD)) {
    reportBug("Shifted by a tainted value", StateFalse, C,
              std::make_unique<taint::TaintBugVisitor>(*DV));
    return;
  }

  C.addTransition(StateFalse);
}

void ento::registerShiftingChecker(CheckerManager &mgr) {
  mgr.registerChecker<ShiftingChecker>();
}

bool ento::shouldRegisterShiftingChecker(const CheckerManager &mgr) {
  return true;
}
