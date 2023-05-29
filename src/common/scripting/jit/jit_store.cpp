
#include "jitintern.h"

void JitCompiler::EmitSB()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(Trunc8(LoadD(B)), ToInt8Ptr(LoadA(A), ConstD(C)));
}

void JitCompiler::EmitSB_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(Trunc8(LoadD(B)), ToInt8Ptr(LoadA(A), LoadD(C)));
}

void JitCompiler::EmitSH()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(Trunc16(LoadD(B)), ToInt16Ptr(LoadA(A), ConstD(C)));
}

void JitCompiler::EmitSH_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(Trunc16(LoadD(B)), ToInt16Ptr(LoadA(A), LoadD(C)));
}

void JitCompiler::EmitSW()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(LoadD(B), ToInt32Ptr(LoadA(A), ConstD(C)));
}

void JitCompiler::EmitSW_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(LoadD(B), ToInt32Ptr(LoadA(A), LoadD(C)));
}

void JitCompiler::EmitSSP()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(FPTrunc(LoadF(B)), ToFloatPtr(LoadA(A), ConstD(C)));
}

void JitCompiler::EmitSSP_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(FPTrunc(LoadF(B)), ToFloatPtr(LoadA(A), LoadD(C)));
}

void JitCompiler::EmitSDP()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(LoadF(B), ToDoublePtr(LoadA(A), ConstD(C)));
}

void JitCompiler::EmitSDP_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	Store(LoadF(B), ToDoublePtr(LoadA(A), LoadD(C)));
}

void JitCompiler::EmitSS()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	cc.CreateCall(stringAssignmentOperator, { OffsetPtr(LoadA(A), ConstD(C)), LoadS(B) });
}

void JitCompiler::EmitSS_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	cc.CreateCall(stringAssignmentOperator, { OffsetPtr(LoadA(A), LoadD(C)), LoadS(B) });
}

void JitCompiler::EmitSO()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* value = LoadA(B);
	cc.CreateStore(value, ToInt8PtrPtr(LoadA(A), ConstD(C)));
	cc.CreateCall(writeBarrier, { value });
}

void JitCompiler::EmitSO_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* value = LoadA(B);
	cc.CreateStore(value, ToInt8PtrPtr(LoadA(A), LoadD(C)));
	cc.CreateCall(writeBarrier, { value });
}

void JitCompiler::EmitSP()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	cc.CreateStore(LoadA(B), ToInt8PtrPtr(LoadA(A), ConstD(C)));
}

void JitCompiler::EmitSP_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	cc.CreateStore(LoadA(B), ToInt8PtrPtr(LoadA(A), LoadD(C)));
}

void JitCompiler::EmitSV2()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToDoublePtr(LoadA(A), ConstD(C));
	Store(LoadF(B), base);
	Store(LoadF(B + 1), OffsetPtr(base, 1));
}

void JitCompiler::EmitSV2_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToDoublePtr(LoadA(A), LoadD(C));
	Store(LoadF(B), base);
	Store(LoadF(B + 1), OffsetPtr(base, 1));
}

void JitCompiler::EmitSV3()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToDoublePtr(LoadA(A), ConstD(C));
	Store(LoadF(B), base);
	Store(LoadF(B + 1), OffsetPtr(base, 1));
	Store(LoadF(B + 2), OffsetPtr(base, 2));
}

void JitCompiler::EmitSV3_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToDoublePtr(LoadA(A), LoadD(C));
	Store(LoadF(B), base);
	Store(LoadF(B + 1), OffsetPtr(base, 1));
	Store(LoadF(B + 2), OffsetPtr(base, 2));
}

void JitCompiler::EmitSV4()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToDoublePtr(LoadA(A), ConstD(C));
	Store(LoadF(B), base);
	Store(LoadF(B + 1), OffsetPtr(base, 1));
	Store(LoadF(B + 2), OffsetPtr(base, 2));
	Store(LoadF(B + 3), OffsetPtr(base, 3));
}

void JitCompiler::EmitSV4_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToDoublePtr(LoadA(A), LoadD(C));
	Store(LoadF(B), base);
	Store(LoadF(B + 1), OffsetPtr(base, 1));
	Store(LoadF(B + 2), OffsetPtr(base, 2));
	Store(LoadF(B + 3), OffsetPtr(base, 3));
}

void JitCompiler::EmitSFV2()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToFloatPtr(LoadA(A), ConstD(C));
	Store(cc.CreateFPTrunc(LoadF(B), floatTy), base);
	Store(cc.CreateFPTrunc(LoadF(B + 1), floatTy), OffsetPtr(base, 1));
}

void JitCompiler::EmitSFV2_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToFloatPtr(LoadA(A), LoadD(C));
	Store(cc.CreateFPTrunc(LoadF(B), floatTy), base);
	Store(cc.CreateFPTrunc(LoadF(B + 1), floatTy), OffsetPtr(base, 1));
}

void JitCompiler::EmitSFV3()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToFloatPtr(LoadA(A), ConstD(C));
	Store(cc.CreateFPTrunc(LoadF(B), floatTy), base);
	Store(cc.CreateFPTrunc(LoadF(B + 1), floatTy), OffsetPtr(base, 1));
	Store(cc.CreateFPTrunc(LoadF(B + 2), floatTy), OffsetPtr(base, 2));
}

void JitCompiler::EmitSFV3_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToFloatPtr(LoadA(A), LoadD(C));
	Store(cc.CreateFPTrunc(LoadF(B), floatTy), base);
	Store(cc.CreateFPTrunc(LoadF(B + 1), floatTy), OffsetPtr(base, 1));
	Store(cc.CreateFPTrunc(LoadF(B + 2), floatTy), OffsetPtr(base, 2));
}

void JitCompiler::EmitSFV4()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToFloatPtr(LoadA(A), ConstD(C));
	Store(cc.CreateFPTrunc(LoadF(B), floatTy), base);
	Store(cc.CreateFPTrunc(LoadF(B + 1), floatTy), OffsetPtr(base, 1));
	Store(cc.CreateFPTrunc(LoadF(B + 2), floatTy), OffsetPtr(base, 2));
	Store(cc.CreateFPTrunc(LoadF(B + 3), floatTy), OffsetPtr(base, 3));
}

void JitCompiler::EmitSFV4_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	IRValue* base = ToFloatPtr(LoadA(A), LoadD(C));
	Store(cc.CreateFPTrunc(LoadF(B), floatTy), base);
	Store(cc.CreateFPTrunc(LoadF(B + 1), floatTy), OffsetPtr(base, 1));
	Store(cc.CreateFPTrunc(LoadF(B + 2), floatTy), OffsetPtr(base, 2));
	Store(cc.CreateFPTrunc(LoadF(B + 3), floatTy), OffsetPtr(base, 3));
}

void JitCompiler::EmitSBIT()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);

	IRBasicBlock* ifbb = irfunc->createBasicBlock({});
	IRBasicBlock* elsebb = irfunc->createBasicBlock({});
	IRBasicBlock* endbb = irfunc->createBasicBlock({});

	cc.CreateCondBr(cc.CreateICmpNE(LoadD(B), ConstValueD(0)), ifbb, elsebb);

	cc.SetInsertPoint(ifbb);
	IRValue* ptr = LoadA(A);
	Store(cc.CreateOr(Load(ptr), ircontext->getConstantInt(int8Ty, (int)C)), ptr);
	cc.CreateBr(endbb);

	cc.SetInsertPoint(elsebb);
	ptr = LoadA(A);
	Store(cc.CreateAnd(Load(ptr), ircontext->getConstantInt(int8Ty, ~(int)C)), ptr);
	cc.CreateBr(endbb);

	cc.SetInsertPoint(endbb);
}
