
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
	cc.CreateCall(writeBarrier, { OffsetPtr(LoadA(A), ConstD(C)), LoadA(B) });
}

void JitCompiler::EmitSO_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	cc.CreateCall(writeBarrier, { OffsetPtr(LoadA(A), LoadD(C)), LoadA(B) });
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
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, konstd[C]);
	cc.movsd(asmjit::x86::qword_ptr(tmp), regF[B]);
	cc.movsd(asmjit::x86::qword_ptr(tmp, 8), regF[B + 1]);
	cc.movsd(asmjit::x86::qword_ptr(tmp, 16), regF[B + 2]);
	cc.movsd(asmjit::x86::qword_ptr(tmp, 24), regF[B + 3]);
}

void JitCompiler::EmitSV4_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, regD[C]);
	cc.movsd(asmjit::x86::qword_ptr(tmp), regF[B]);
	cc.movsd(asmjit::x86::qword_ptr(tmp, 8), regF[B + 1]);
	cc.movsd(asmjit::x86::qword_ptr(tmp, 16), regF[B + 2]);
	cc.movsd(asmjit::x86::qword_ptr(tmp, 24), regF[B + 3]);
}

void JitCompiler::EmitSFV2()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, konstd[C]);

	auto tmpF = newTempXmmSs();
	cc.cvtsd2ss(tmpF, regF[B]);
	cc.movss(asmjit::x86::qword_ptr(tmp), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 1]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 4), tmpF);
}

void JitCompiler::EmitSFV2_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, regD[C]);

	auto tmpF = newTempXmmSs();
	cc.cvtsd2ss(tmpF, regF[B]);
	cc.movss(asmjit::x86::qword_ptr(tmp), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 1]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 4), tmpF);
}

void JitCompiler::EmitSFV3()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, konstd[C]);
	auto tmpF = newTempXmmSs();
	cc.cvtsd2ss(tmpF, regF[B]);
	cc.movss(asmjit::x86::qword_ptr(tmp), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 1]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 4), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 2]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 8), tmpF);
}

void JitCompiler::EmitSFV3_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, regD[C]);
	auto tmpF = newTempXmmSs();
	cc.cvtsd2ss(tmpF, regF[B]);
	cc.movss(asmjit::x86::qword_ptr(tmp), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 1]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 4), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 2]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 8), tmpF);
}

void JitCompiler::EmitSFV4()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, konstd[C]);
	auto tmpF = newTempXmmSs();
	cc.cvtsd2ss(tmpF, regF[B]);
	cc.movss(asmjit::x86::qword_ptr(tmp), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 1]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 4), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 2]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 8), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 3]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 12), tmpF);
}

void JitCompiler::EmitSFV4_R()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, regA[A]);
	cc.add(tmp, regD[C]);
	auto tmpF = newTempXmmSs();
	cc.cvtsd2ss(tmpF, regF[B]);
	cc.movss(asmjit::x86::qword_ptr(tmp), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 1]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 4), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 2]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 8), tmpF);
	cc.cvtsd2ss(tmpF, regF[B + 3]);
	cc.movss(asmjit::x86::qword_ptr(tmp, 12), tmpF);
}

void JitCompiler::EmitSBIT()
{
	EmitNullPointerThrow(A, X_WRITE_NIL);

	IRValue* ptr = LoadA(A);
	IRValue* value = cc.CreateLoad(ptr);
	
	IRBasicBlock* ifbb = irfunc->createBasicBlock({});
	IRBasicBlock* elsebb = irfunc->createBasicBlock({});
	IRBasicBlock* endbb = irfunc->createBasicBlock({});

	cc.CreateCondBr(cc.CreateICmpNE(LoadD(B), ConstValueD(0)), ifbb, elsebb);
	cc.SetInsertPoint(ifbb);
	cc.CreateStore(cc.CreateOr(value, ircontext->getConstantInt((int)C)), ptr);
	cc.CreateBr(endbb);
	cc.SetInsertPoint(elsebb);
	cc.CreateStore(cc.CreateAnd(value, ircontext->getConstantInt(~(int)C)), ptr);
	cc.CreateBr(endbb);
	cc.SetInsertPoint(endbb);
}
