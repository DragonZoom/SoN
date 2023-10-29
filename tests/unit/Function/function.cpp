#include "gtest/gtest.h"

#include <Function.hpp>
#include <memory>
using namespace son;

TEST(Function, FunctionEmpty) {
  ASSERT_TRUE(true);
  FunctionType fnTy(ValueType::Void, {ValueType::Int32});
  Function F("Hello", fnTy);

  EXPECT_NE(F.getStart(), nullptr);
  EXPECT_NE(F.getEnd(), nullptr);
  EXPECT_EQ(F.getNumArgs(), 1);
  EXPECT_NE(F.getArg(0), nullptr);
}

std::unique_ptr<Function> buildTest0() {
  FunctionType fnTy(ValueType::Void, {});
  auto &&fPtr = std::make_unique<Function>("Test0", fnTy);
  auto &F = *fPtr;

  //                 +-----+
  //                 |  0  |
  //                 +-----+
  //                    |
  //                    V
  //                 +-----+    +-----+
  //          +----->|  1  |--->|  2  |
  //          |      +-----+    +-----+
  //          |         |
  //          |         V
  //          |      +-----+
  //          -------|  3  |
  //                 +-----+

  auto n0 = F.getStart();
  auto n2 = F.getEnd();

  auto range01 = F.createNode<RegionNode>();
  auto noJmp = F.createNode<JmpNode>(n0);
  range01->addCFInput(noJmp);

  auto n1Counter0 = F.createNode<ConstantNode>(ValueType::Int32, 0);
  auto n1CounterPhi = F.createNode<PhiNode>(range01, 2, ValueType::Int32);
  n1CounterPhi->setVal(0, n1Counter0);
  auto n1Counter3 = F.createNode<AddNode>(
      n1CounterPhi, F.createNode<ConstantNode>(ValueType::Int32, 1));
  n1CounterPhi->setVal(1, n1Counter3);

  // cond (i == 10)
  auto loopLastCount = F.createNode<ConstantNode>(ValueType::Int32, 10);
  auto n1Cond = F.createNode<CmpEQNode>(n1CounterPhi, loopLastCount);

  // if (i == 10)
  auto n1If = F.createNode<IfNode>(range01, n1Cond);
  auto ifTrue = F.createNode<IfTrueNode>(n1If);
  auto ifFalse = F.createNode<IfFalseNode>(n1If);
  auto loopBodyRegion = F.createNode<RegionNode>();
  loopBodyRegion->addCFInput(ifFalse);
  // Jmp loopBody -> loopHeader
  auto backEdge = F.createNode<JmpNode>(loopBodyRegion);
  range01->addCFInput(backEdge);

  // final
  F.getEnd()->addCFInput(ifTrue);

  //=-------------------------------------------
  // check CFG
  // 0: start
  EXPECT_EQ(F.getStart()->opCount(), 0);
  EXPECT_EQ(F.getStart()->usersCount(), 1);
  EXPECT_EQ(F.getStart()->users().count(noJmp), 1);
  EXPECT_EQ(range01->operand(0), noJmp);
  // 0-3-1: check range + phi
  EXPECT_EQ(range01->opCount(), 2);
  EXPECT_EQ(range01->operand(1), backEdge);
  //                    inpuCF + inputVal1 + inputVal
  EXPECT_EQ(n1CounterPhi->opCount(), 3);
  EXPECT_EQ(n1CounterPhi->operand(0), range01);
  EXPECT_EQ(n1CounterPhi->operand(1), n1Counter0); // init
  EXPECT_EQ(n1CounterPhi->operand(2), n1Counter3); // loop
  //   uses
  EXPECT_EQ(range01->usersCount(), 2);
  EXPECT_EQ(range01->users().count(n1CounterPhi), 1);
  EXPECT_EQ(range01->users().count(n1If), 1);
  EXPECT_EQ(n1CounterPhi->usersCount(), 2);
  EXPECT_EQ(n1CounterPhi->users().count(n1Counter3), 1);
  EXPECT_EQ(n1CounterPhi->users().count(n1Cond), 1);
  // if
  EXPECT_EQ(n1If->opCount(), 2);
  EXPECT_EQ(n1If->operand(0), range01);
  EXPECT_EQ(n1If->operand(1), n1Cond);
  EXPECT_EQ(n1If->usersCount(), 2);
  EXPECT_EQ(n1If->users().count(ifTrue), 1);
  EXPECT_EQ(n1If->users().count(ifFalse), 1);
  // cond
  EXPECT_EQ(n1Cond->opCount(), 2);
  EXPECT_EQ(n1Cond->operand(0), n1CounterPhi);
  EXPECT_EQ(n1Cond->operand(1), loopLastCount);
  EXPECT_EQ(n1Cond->usersCount(), 1);
  EXPECT_EQ(n1Cond->users().count(n1If), 1);
  // end
  EXPECT_EQ(F.getEnd()->opCount(), 1);
  EXPECT_EQ(F.getEnd()->operand(0), ifTrue);
  EXPECT_EQ(F.getEnd()->usersCount(), 0);

  return fPtr;
}

std::unique_ptr<Function> buildTest1() {
  FunctionType fnTy(ValueType::Void, {ValueType::Int32});
  auto &&fPtr = std::make_unique<Function>("Test1", fnTy);
  auto &F = *fPtr;

  //                 +-----+
  //                 |  A  |
  //                 +-----+
  //                    |
  //                    V
  //                 +-----+
  //       ----------|  B  |---------
  //       |         +-----+        |
  //       |                        |
  //       V                        V
  //     +-----+     +-----+     +-----+
  //     |  C  |  ---|  E  |<----|  F  |
  //     +-----+  |  +-----+     +-----+
  //        |     |                 |
  //        |     V                 |
  //        |  +-----+     +-----+  |
  //        +->|  D  |<----|  G  |<-+
  //           +-----+     +-----+
  //
  // A - start
  // D - end
  auto A = F.getStart();
  auto D = F.getEnd();

  auto BIfRhs = F.createNode<ConstantNode>(ValueType::Int32, 66);
  auto BCond = F.createNode<CmpLENode>(F.getArg(0), BIfRhs);

  // B if
  auto BIf = F.createNode<IfNode>(A, BCond);
  auto BIfTrue = F.createNode<IfTrueNode>(BIf);
  auto BIfFalse = F.createNode<IfFalseNode>(BIf);
  // C
  auto CMulTimes = F.createNode<ConstantNode>(ValueType::Int32, 5);
  auto CRetVal = F.createNode<MulNode>(F.getArg(0), CMulTimes);

  // F cond
  auto FIfRhs = F.createNode<ConstantNode>(ValueType::Int32, -10);
  auto FCond = F.createNode<CmpGENode>(F.getArg(0), BIfRhs);
  // F if
  auto BFalseRegF = F.createNode<RegionNode>();
  BFalseRegF->addCFInput(BIfFalse);
  auto FIf = F.createNode<IfNode>(BFalseRegF, FCond);
  auto FIfTrue = F.createNode<IfTrueNode>(FIf);
  auto FIfFalse = F.createNode<IfFalseNode>(FIf);

  // E
  auto ERhs = F.createNode<ConstantNode>(ValueType::Int32, 5);
  auto ERetVal = F.createNode<RemNode>(F.getArg(0), ERhs);

  // G
  auto GRhs = F.createNode<ConstantNode>(ValueType::Int32, 3);
  auto GRetVal = F.createNode<DivNode>(F.getArg(0), ERhs);

  // final phi for ret value at D node
  //   range
  auto DRange = F.createNode<RegionNode>();
  DRange->addCFInput(BIfTrue);  // C
  DRange->addCFInput(FIfTrue);  // E
  DRange->addCFInput(FIfFalse); // G
  //   phi
  auto DPhi = F.createNode<PhiNode>(DRange, 3, ValueType::Int32);
  DPhi->setVal(0, CRetVal);
  DPhi->setVal(1, ERetVal);
  DPhi->setVal(2, GRetVal);

  // ret
  auto ret = F.createNode<RetNode>(DRange, DPhi);

  // final
  F.getEnd()->addCFInput(ret);

  return fPtr;
}

TEST(Function, FunctionTest0) {
  auto &&fPtr = buildTest0();
  auto &F = *fPtr;

  // verify
  EXPECT_TRUE(F.verify());
}

TEST(Function, FunctionTest1) {
  auto &&fPtr = buildTest1();
  auto &F = *fPtr;

  // verify
  EXPECT_TRUE(F.verify());
}

TEST(Function, dfs) {
  auto &&fPtr = buildTest0();
  auto &F = *fPtr;

  auto &&[order, dfsParents, dfsNumbers] = F.dfs();
  EXPECT_EQ(order.size(), dfsParents.size());
  EXPECT_EQ(order[0], F.getStart());
  EXPECT_EQ(dfsParents[0], 0);
}

TEST(Function, idom) {
  FunctionType fnTy(ValueType::Void, {ValueType::Int32});
  auto &&fPtr = std::make_unique<Function>("Test1", fnTy);
  auto &F = *fPtr;

  //   A1
  //  / \
  // B2   C5
  //  \ / \
  //   D3<--F6
  //   |
  //   E4
  auto StartJmp = F.createNode<JmpNode>(F.getStart());

  auto A = F.createNode<RegionNode>();
  A->addCFInput(StartJmp);
  auto AIf =
      F.createNode<IfNode>(A, F.createNode<ConstantNode>(ValueType::Int1, 1));
  auto AIfTrue = F.createNode<IfTrueNode>(AIf);
  auto AIfFalse = F.createNode<IfFalseNode>(AIf);

  auto B = F.createNode<RegionNode>();
  B->addCFInput(AIfTrue);
  auto BJmp = F.createNode<JmpNode>(B);

  auto C = F.createNode<RegionNode>();
  C->addCFInput(AIfFalse);

  auto CIf =
      F.createNode<IfNode>(C, F.createNode<ConstantNode>(ValueType::Int1, 1));
  auto CIfTrue = F.createNode<IfTrueNode>(CIf);
  auto CIfFalse = F.createNode<IfFalseNode>(CIf);

  auto FR = F.createNode<RegionNode>();
  FR->addCFInput(CIfFalse);
  auto FRJmp = F.createNode<JmpNode>(FR);

  auto D = F.createNode<RegionNode>();
  D->addCFInput(CIfTrue);
  D->addCFInput(BJmp);
  D->addCFInput(FRJmp);
  auto DJmp = F.createNode<JmpNode>(D);

  // final
  F.getEnd()->addCFInput(DJmp);

  EXPECT_TRUE(F.verify());

  auto &&dfsResult = F.dfs();
  auto &&semi = F.semiDominators(dfsResult);
  auto &&idom = F.iDominators(dfsResult, semi);

  std::vector<RegionNodeBase *> refOrder = {F.getStart(), A, B, D,
                                            F.getEnd(),   C, FR};
  auto &&[order, dfsParents, dfsNumbers] = dfsResult;
  EXPECT_EQ(order.size(), refOrder.size());
  EXPECT_EQ(order, refOrder);

  // get pointers to idom
  std::vector<Node *> refIDom = {F.getStart(), F.getStart(), A, A, D, A, C};
  std::vector<Node *> obtaindeIDom(idom.size());
  std::transform(idom.begin(), idom.end(), obtaindeIDom.begin(), [ord = &order](auto id){
    return ord->at(id);
  });

  EXPECT_EQ(obtaindeIDom.size(), refIDom.size());
  EXPECT_EQ(obtaindeIDom, refIDom);
  EXPECT_EQ(semi, idom);
}