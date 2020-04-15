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
class ShiftingChecker : public Checker< check::PreStmt<BinaryOperator> > {
  mutable std::unique_ptr<BuiltinBug> BT;
  void reportBug(const char *Msg, CheckerContext &C,
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
    const char *Msg, CheckerContext &C,
    std::unique_ptr<BugReporterVisitor> Visitor) const {
  if (ExplodedNode *N = C.generateErrorNode()) {
    if (!BT)
      BT.reset(new BuiltinBug(this, "Improper shifting"));

    auto R = std::make_unique<PathSensitiveBugReport>(*BT, Msg, N);
    R->addVisitor(std::move(Visitor));
    bugreporter::trackExpressionValue(N, getRHSExpr(N), *R);
    C.emitReport(std::move(R));
  }
}

void ShiftingChecker::checkPreStmt(const BinaryOperator *B,
                                  CheckerContext &C) const {
  BinaryOperator::Opcode Op = B->getOpcode();
  if (Op != BO_Shl &&
      Op != BO_Shr &&
      Op != BO_ShlAssign &&
      Op != BO_ShrAssign)
    return;
  // If we get here, check for negative shifting or
  // shifting with overflow.
  Expr* rightSide = B->getRHS();
  QualType typeOfRHS = B->getRHS()->getType();

  if (!typeOfRHS->isScalarType())
    return;

  // SVal rightSide = C.getSVal(B->getRHS());
  // Optional<DefinedSVal> DV = rightSide.getAs<DefinedSVal>();


  // Shifting-by-undefined is handled in the generic checking for uses of
  // undefined values.
  //  if (!DV)
  //  return;

  // Check for improper shifting.
  // ConstraintManager &CM = C.getConstraintManager();
  // ProgramStateRef stateNotImproper, stateImproper;
  // std::tie(stateNotImproper, stateImproper) = CM.assumeInclusiveRange(C.getState(), 0 < *DV);



  // if (!stateNotImproper) {
  //   assert(stateImproper);
  //   reportBug("Shifting by a negative or value too large", stateImproper, C);
  //   return;
  // }


  if(typeOfRHS->isSignedIntegerType() && C.isNegative(rightSide)){
    reportBug("Shifting by a negative value", C);
    return;
  }

  // bool TaintedD = isTainted(C.getState(), *DV);
  // if ((stateNotImproper && stateImproper && TaintedD)) {
  //   reportBug("Shifting by a tainted value, possibly improper", C,
  //             llvm::make_unique<taint::TaintBugVisitor>(*DV));
  //   return;
  // }

  // If we get here, then the righthand side should not be improper. We abandon the
  // improper shifting     case for now.
  //C.addTransition(stateNotImproper);
}

void ento::registerShiftingChecker(CheckerManager &mgr) {
  mgr.registerChecker<ShiftingChecker>();
}

bool ento::shouldRegisterShiftingChecker(const CheckerManager &mgr) {
  return true;
}
