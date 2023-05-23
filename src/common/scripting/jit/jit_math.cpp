
#include "jitintern.h"
#include "basics.h"

/////////////////////////////////////////////////////////////////////////////
// String instructions.

static void ConcatString(FString* to, FString* first, FString* second)
{
	*to = *first + *second;
}

void JitCompiler::EmitCONCAT()
{
	cc.CreateCall(GetNativeFunc<void, FString*, FString*, FString*>("__ConcatString", ConcatString), { cc.CreateLoad(regS[A]), cc.CreateLoad(regS[B]), cc.CreateLoad(regS[C]) });
}

static int StringLength(FString* str)
{
	return static_cast<int>(str->Len());
}

void JitCompiler::EmitLENS()
{
	cc.CreateStore(cc.CreateCall(GetNativeFunc<int, FString*>("__StringLength", StringLength), { cc.CreateLoad(regS[B]) }), regD[A]);
}

#if 0
static int StringCompareNoCase(FString* first, FString* second)
{
	return first->CompareNoCase(*second);
}

static int StringCompare(FString* first, FString* second)
{
	return first->Compare(*second);
}

void JitCompiler::EmitCMPS()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {

		auto call = CreateCall<int, FString*, FString*>(static_cast<bool>(A & CMP_APPROX) ? StringCompareNoCase : StringCompare);

		auto result = newResultInt32();
		call->setRet(0, result);

		if (static_cast<bool>(A & CMP_BK)) call->setArg(0, asmjit::imm_ptr(&konsts[B]));
		else                               call->setArg(0, regS[B]);

		if (static_cast<bool>(A & CMP_CK)) call->setArg(1, asmjit::imm_ptr(&konsts[C]));
		else                               call->setArg(1, regS[C]);

		int method = A & CMP_METHOD_MASK;
		if (method == CMP_EQ) {
			cc.test(result, result);
			if (check) cc.jz(fail);
			else       cc.jnz(fail);
		}
		else if (method == CMP_LT) {
			cc.cmp(result, 0);
			if (check) cc.jl(fail);
			else       cc.jnl(fail);
		}
		else {
			cc.cmp(result, 0);
			if (check) cc.jle(fail);
			else       cc.jnle(fail);
		}
	});
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Integer math.

void JitCompiler::EmitSLL_RR()
{
	cc.CreateStore(cc.CreateShl(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitSLL_RI()
{
	cc.CreateStore(cc.CreateShl(cc.CreateLoad(regD[B]), ircontext->getConstantInt(C)), regD[A]);
}

void JitCompiler::EmitSLL_KR()
{
	cc.CreateStore(cc.CreateShl(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitSRL_RR()
{
	cc.CreateStore(cc.CreateLShr(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitSRL_RI()
{
	cc.CreateStore(cc.CreateLShr(cc.CreateLoad(regD[B]), ircontext->getConstantInt(C)), regD[A]);
}

void JitCompiler::EmitSRL_KR()
{
	cc.CreateStore(cc.CreateLShr(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitSRA_RR()
{
	cc.CreateStore(cc.CreateAShr(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitSRA_RI()
{
	cc.CreateStore(cc.CreateAShr(cc.CreateLoad(regD[B]), ircontext->getConstantInt(C)), regD[A]);
}

void JitCompiler::EmitSRA_KR()
{
	cc.CreateStore(cc.CreateAShr(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitADD_RR()
{
	cc.CreateStore(cc.CreateAdd(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitADD_RK()
{
	cc.CreateStore(cc.CreateAdd(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
}

void JitCompiler::EmitADDI()
{
	cc.CreateStore(cc.CreateAdd(cc.CreateLoad(regD[B]), ircontext->getConstantInt(Cs)), regD[A]);
}

void JitCompiler::EmitSUB_RR()
{
	cc.CreateStore(cc.CreateSub(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitSUB_RK()
{
	cc.CreateStore(cc.CreateSub(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
}

void JitCompiler::EmitSUB_KR()
{
	cc.CreateStore(cc.CreateSub(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitMUL_RR()
{
	cc.CreateStore(cc.CreateMul(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitMUL_RK()
{
	cc.CreateStore(cc.CreateMul(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
}

void JitCompiler::EmitDIV_RR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateSDiv(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitDIV_RK()
{
	if (konstd[C] != 0)
	{
		cc.CreateStore(cc.CreateSDiv(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
	}
	else
	{
		EmitThrowException(X_DIVISION_BY_ZERO);
	}
}

void JitCompiler::EmitDIV_KR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateSDiv(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[B])), regD[A]);
}

void JitCompiler::EmitDIVU_RR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateUDiv(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitDIVU_RK()
{
	if (konstd[C] != 0)
	{
		cc.CreateStore(cc.CreateUDiv(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
	}
	else
	{
		EmitThrowException(X_DIVISION_BY_ZERO);
	}
}

void JitCompiler::EmitDIVU_KR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateUDiv(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[B])), regD[A]);
}

void JitCompiler::EmitMOD_RR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateSRem(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitMOD_RK()
{
	if (konstd[C] != 0)
	{
		cc.CreateStore(cc.CreateSRem(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
	}
	else
	{
		EmitThrowException(X_DIVISION_BY_ZERO);
	}
}

void JitCompiler::EmitMOD_KR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateSRem(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[B])), regD[A]);
}

void JitCompiler::EmitMODU_RR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateURem(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitMODU_RK()
{
	if (konstd[C] != 0)
	{
		cc.CreateStore(cc.CreateURem(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
	}
	else
	{
		EmitThrowException(X_DIVISION_BY_ZERO);
	}
}

void JitCompiler::EmitMODU_KR()
{
	auto exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(cc.CreateLoad(regD[C]), ircontext->getConstantInt(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateURem(ircontext->getConstantInt(konstd[B]), cc.CreateLoad(regD[B])), regD[A]);
}

void JitCompiler::EmitAND_RR()
{
	cc.CreateStore(cc.CreateAnd(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitAND_RK()
{
	cc.CreateStore(cc.CreateAnd(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
}

void JitCompiler::EmitOR_RR()
{
	cc.CreateStore(cc.CreateOr(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitOR_RK()
{
	cc.CreateStore(cc.CreateOr(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
}

void JitCompiler::EmitXOR_RR()
{
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regD[B]), cc.CreateLoad(regD[C])), regD[A]);
}

void JitCompiler::EmitXOR_RK()
{
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regD[B]), ircontext->getConstantInt(konstd[C])), regD[A]);
}

#if 0
void JitCompiler::EmitMIN_RR()
{
	auto rc = CheckRegD(C, A);
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.cmp(rc, regD[A]);
	cc.cmovl(regD[A], rc);
}

void JitCompiler::EmitMIN_RK()
{
	auto rc = newTempInt32();
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.mov(rc, asmjit::imm(konstd[C]));
	cc.cmp(rc, regD[A]);
	cc.cmovl(regD[A], rc);
}

void JitCompiler::EmitMAX_RR()
{
	auto rc = CheckRegD(C, A);
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.cmp(rc, regD[A]);
	cc.cmovg(regD[A], rc);
}

void JitCompiler::EmitMAX_RK()
{
	auto rc = newTempInt32();
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.mov(rc, asmjit::imm(konstd[C]));
	cc.cmp(rc, regD[A]);
	cc.cmovg(regD[A], rc);
}

void JitCompiler::EmitMINU_RR()
{
	auto rc = CheckRegD(C, A);
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.cmp(rc, regD[A]);
	cc.cmovb(regD[A], rc);
}

void JitCompiler::EmitMINU_RK()
{
	auto rc = newTempInt32();
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.mov(rc, asmjit::imm(konstd[C]));
	cc.cmp(rc, regD[A]);
	cc.cmovb(regD[A], rc);
}

void JitCompiler::EmitMAXU_RR()
{
	auto rc = CheckRegD(C, A);
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.cmp(rc, regD[A]);
	cc.cmova(regD[A], rc);
}

void JitCompiler::EmitMAXU_RK()
{
	auto rc = newTempInt32();
	if (A != B)
		cc.mov(regD[A], regD[B]);
	cc.mov(rc, asmjit::imm(konstd[C]));
	cc.cmp(rc, regD[A]);
	cc.cmova(regD[A], rc);
}
#endif

void JitCompiler::EmitABS()
{
	IRValue* srcB = cc.CreateLoad(regD[B]);
	IRValue* tmp = cc.CreateAShr(srcB, ircontext->getConstantInt(31));
	cc.CreateStore(cc.CreateSub(cc.CreateXor(tmp, srcB), tmp), regD[A]);
}

void JitCompiler::EmitNEG()
{
	cc.CreateStore(cc.CreateNeg(cc.CreateLoad(regD[B])), regD[A]);
}

void JitCompiler::EmitNOT()
{
	cc.CreateStore(cc.CreateNot(cc.CreateLoad(regD[B])), regD[A]);
}

#if 0
void JitCompiler::EmitEQ_R()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], regD[C]);
		if (check) cc.je(fail);
		else       cc.jne(fail);
	});
}

void JitCompiler::EmitEQ_K()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], konstd[C]);
		if (check) cc.je(fail);
		else       cc.jne(fail);
	});
}

void JitCompiler::EmitLT_RR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], regD[C]);
		if (check) cc.jl(fail);
		else       cc.jnl(fail);
	});
}

void JitCompiler::EmitLT_RK()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], konstd[C]);
		if (check) cc.jl(fail);
		else       cc.jnl(fail);
	});
}

void JitCompiler::EmitLT_KR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(&konstd[B]));
		cc.cmp(asmjit::x86::ptr(tmp), regD[C]);
		if (check) cc.jl(fail);
		else       cc.jnl(fail);
	});
}

void JitCompiler::EmitLE_RR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], regD[C]);
		if (check) cc.jle(fail);
		else       cc.jnle(fail);
	});
}

void JitCompiler::EmitLE_RK()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], konstd[C]);
		if (check) cc.jle(fail);
		else       cc.jnle(fail);
	});
}

void JitCompiler::EmitLE_KR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(&konstd[B]));
		cc.cmp(asmjit::x86::ptr(tmp), regD[C]);
		if (check) cc.jle(fail);
		else       cc.jnle(fail);
	});
}

void JitCompiler::EmitLTU_RR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], regD[C]);
		if (check) cc.jb(fail);
		else       cc.jnb(fail);
	});
}

void JitCompiler::EmitLTU_RK()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], konstd[C]);
		if (check) cc.jb(fail);
		else       cc.jnb(fail);
	});
}

void JitCompiler::EmitLTU_KR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(&konstd[B]));
		cc.cmp(asmjit::x86::ptr(tmp), regD[C]);
		if (check) cc.jb(fail);
		else       cc.jnb(fail);
	});
}

void JitCompiler::EmitLEU_RR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], regD[C]);
		if (check) cc.jbe(fail);
		else       cc.jnbe(fail);
	});
}

void JitCompiler::EmitLEU_RK()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regD[B], konstd[C]);
		if (check) cc.jbe(fail);
		else       cc.jnbe(fail);
	});
}

void JitCompiler::EmitLEU_KR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(&konstd[B]));
		cc.cmp(asmjit::x86::ptr(tmp), regD[C]);
		if (check) cc.jbe(fail);
		else       cc.jnbe(fail);
	});
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Double-precision floating point math.

void JitCompiler::EmitADDF_RR()
{
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
}

void JitCompiler::EmitADDF_RK()
{
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B]), ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C])), regF[A]);
}

void JitCompiler::EmitSUBF_RR()
{
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
}

void JitCompiler::EmitSUBF_RK()
{
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B]), ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C])), regF[A]);
}

void JitCompiler::EmitSUBF_KR()
{
	cc.CreateStore(cc.CreateFSub(ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[B]), cc.CreateLoad(regF[C])), regF[A]);
}

void JitCompiler::EmitMULF_RR()
{
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
}

void JitCompiler::EmitMULF_RK()
{
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B]), ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C])), regF[A]);
}

void JitCompiler::EmitDIVF_RR()
{
	IRBasicBlock* exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateFCmpUEQ(cc.CreateLoad(regF[C]), ircontext->getConstantFloat(ircontext->getDoubleTy(), 0.0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
}

void JitCompiler::EmitDIVF_RK()
{
	if (konstf[C] == 0.)
	{
		EmitThrowException(X_DIVISION_BY_ZERO);
	}
	else
	{
		cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B]), ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C])), regF[A]);
	}
}

void JitCompiler::EmitDIVF_KR()
{
	IRBasicBlock* exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateFCmpUEQ(cc.CreateLoad(regF[C]), ircontext->getConstantFloat(ircontext->getDoubleTy(), 0.0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateFDiv(ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[B]), cc.CreateLoad(regF[C])), regF[A]);
}

static double DoubleModF(double a, double b)
{
	return a - floor(a / b) * b;
}

void JitCompiler::EmitMODF_RR()
{
	IRBasicBlock* exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateFCmpUEQ(cc.CreateLoad(regF[C]), ircontext->getConstantFloat(ircontext->getDoubleTy(), 0.0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateCall(GetNativeFunc<double, double, double>("__DoubleModF", DoubleModF), { cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C]) }), regF[A]);
}

void JitCompiler::EmitMODF_RK()
{
	if (konstf[C] == 0.)
	{
		EmitThrowException(X_DIVISION_BY_ZERO);
	}
	else
	{
		cc.CreateStore(cc.CreateCall(GetNativeFunc<double, double, double>("__DoubleModF", DoubleModF), { cc.CreateLoad(regF[B]), ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C]) }), regF[A]);
	}
}

void JitCompiler::EmitMODF_KR()
{
	IRBasicBlock* exceptionbb = EmitThrowExceptionLabel(X_DIVISION_BY_ZERO);
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateFCmpUEQ(cc.CreateLoad(regF[C]), ircontext->getConstantFloat(ircontext->getDoubleTy(), 0.0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateStore(cc.CreateCall(GetNativeFunc<double, double, double>("__DoubleModF", DoubleModF), { ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[B]), cc.CreateLoad(regF[C]) }), regF[A]);
}

void JitCompiler::EmitPOWF_RR()
{
	cc.CreateStore(cc.CreateCall(GetNativeFunc<double, double, double>("__g_pow", g_pow), { cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C]) }), regF[A]);
}

void JitCompiler::EmitPOWF_RK()
{
	cc.CreateStore(cc.CreateCall(GetNativeFunc<double, double, double>("__g_pow", g_pow), { cc.CreateLoad(regF[B]), ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C]) }), regF[A]);
}

void JitCompiler::EmitPOWF_KR()
{
	cc.CreateStore(cc.CreateCall(GetNativeFunc<double, double, double>("__g_pow", g_pow), { ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[B]), cc.CreateLoad(regF[C]) }), regF[A]);
}

#if 0

void JitCompiler::EmitMINF_RR()
{
	auto rc = CheckRegF(C, A);
	if (A != B)
		cc.movsd(regF[A], regF[B]);
	cc.minpd(regF[A], rc);  // minsd requires SSE 4.1
}

void JitCompiler::EmitMINF_RK()
{
	auto rb = CheckRegF(B, A);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, asmjit::imm_ptr(&konstf[C]));
	cc.movsd(regF[A], asmjit::x86::qword_ptr(tmp));
	cc.minpd(regF[A], rb); // minsd requires SSE 4.1
}

void JitCompiler::EmitMAXF_RR()
{
	auto rc = CheckRegF(C, A);
	if (A != B)
		cc.movsd(regF[A], regF[B]);
	cc.maxpd(regF[A], rc); // maxsd requires SSE 4.1
}

void JitCompiler::EmitMAXF_RK()
{
	auto rb = CheckRegF(B, A);
	auto tmp = newTempIntPtr();
	cc.mov(tmp, asmjit::imm_ptr(&konstf[C]));
	cc.movsd(regF[A], asmjit::x86::qword_ptr(tmp));
	cc.maxpd(regF[A], rb); // maxsd requires SSE 4.1
}

#endif

void JitCompiler::EmitATAN2()
{
	cc.CreateStore(cc.CreateFMul(cc.CreateCall(GetNativeFunc<double, double, double>("__g_atan2", g_atan2), { cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C]) }), ircontext->getConstantFloat(ircontext->getFloatTy(), 180 / M_PI)), regF[A]);
}

void JitCompiler::EmitFLOP()
{
	if (C == FLOP_NEG)
	{
		cc.CreateStore(cc.CreateFNeg(cc.CreateLoad(regF[B])), regF[A]);
	}
	else
	{
		IRValue* v = cc.CreateLoad(regF[B]);

		if (C == FLOP_TAN_DEG)
		{
			v = cc.CreateFMul(v, ircontext->getConstantFloat(ircontext->getFloatTy(), M_PI / 180));
		}

		typedef double(*FuncPtr)(double);
		const char* funcname = "";
		FuncPtr func = nullptr;
		switch (C)
		{
		default: I_Error("Unknown OP_FLOP subfunction");
		case FLOP_ABS:		func = fabs; funcname = "__fabs"; break;
		case FLOP_EXP:		func = g_exp; funcname = "__g_exp"; break;
		case FLOP_LOG:		func = g_log; funcname = "__g_log"; break;
		case FLOP_LOG10:	func = g_log10; funcname = "__g_log10"; break;
		case FLOP_SQRT:		func = g_sqrt; funcname = "__g_sqrt"; break;
		case FLOP_CEIL:		func = ceil; funcname = "__ceil"; break;
		case FLOP_FLOOR:	func = floor; funcname = "__floor"; break;
		case FLOP_ACOS:		func = g_acos; funcname = "__g_acos"; break;
		case FLOP_ASIN:		func = g_asin; funcname = "__g_asin"; break;
		case FLOP_ATAN:		func = g_atan; funcname = "__g_atan"; break;
		case FLOP_COS:		func = g_cos; funcname = "__g_cos"; break;
		case FLOP_SIN:		func = g_sin; funcname = "__g_sin"; break;
		case FLOP_TAN:		func = g_tan; funcname = "__g_tan"; break;
		case FLOP_ACOS_DEG:	func = g_acos; funcname = "__g_acos"; break;
		case FLOP_ASIN_DEG:	func = g_asin; funcname = "__g_asin"; break;
		case FLOP_ATAN_DEG:	func = g_atan; funcname = "__g_atan"; break;
		case FLOP_COS_DEG:	func = g_cosdeg; funcname = "__g_cosdeg"; break;
		case FLOP_SIN_DEG:	func = g_sindeg; funcname = "__g_sindeg"; break;
		case FLOP_TAN_DEG:	func = g_tan; funcname = "__g_tan"; break;
		case FLOP_COSH:		func = g_cosh; funcname = "__g_cosh"; break;
		case FLOP_SINH:		func = g_sinh; funcname = "__g_sinh"; break;
		case FLOP_TANH:		func = g_tanh; funcname = "__g_tanh"; break;
		case FLOP_ROUND:	func = round; funcname = "__round"; break;
		}

		IRValue* result = cc.CreateCall(GetNativeFunc<double, double>(funcname, func), { v });

		if (C == FLOP_ACOS_DEG || C == FLOP_ASIN_DEG || C == FLOP_ATAN_DEG)
		{
			result = cc.CreateFMul(result, ircontext->getConstantFloat(ircontext->getFloatTy(), 180 / M_PI));
		}

		cc.CreateStore(result, regF[A]);
	}
}

#if 0

void JitCompiler::EmitEQF_R()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		bool approx = static_cast<bool>(A & CMP_APPROX);
		if (!approx)
		{
			cc.ucomisd(regF[B], regF[C]);
			if (check) {
				cc.jp(success);
				cc.je(fail);
			}
			else {
				cc.jp(fail);
				cc.jne(fail);
			}
		}
		else
		{
			auto tmp = newTempXmmSd();

			const int64_t absMaskInt = 0x7FFFFFFFFFFFFFFF;
			auto absMask = cc.newDoubleConst(asmjit::kConstScopeLocal, reinterpret_cast<const double&>(absMaskInt));
			auto absMaskXmm = newTempXmmPd();

			auto epsilon = cc.newDoubleConst(asmjit::kConstScopeLocal, VM_EPSILON);
			auto epsilonXmm = newTempXmmSd();

			cc.movsd(tmp, regF[B]);
			cc.subsd(tmp, regF[C]);
			cc.movsd(absMaskXmm, absMask);
			cc.andpd(tmp, absMaskXmm);
			cc.movsd(epsilonXmm, epsilon);
			cc.ucomisd(epsilonXmm, tmp);

			if (check) cc.ja(fail);
			else       cc.jna(fail);
		}
	});
}

void JitCompiler::EmitEQF_K()
{
	using namespace asmjit;
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		bool approx = static_cast<bool>(A & CMP_APPROX);
		if (!approx) {
			auto konstTmp = newTempIntPtr();
			cc.mov(konstTmp, asmjit::imm_ptr(&konstf[C]));
			cc.ucomisd(regF[B], x86::qword_ptr(konstTmp));
			if (check) {
				cc.jp(success);
				cc.je(fail);
			}
			else {
				cc.jp(fail);
				cc.jne(fail);
			}
		}
		else {
			auto konstTmp = newTempIntPtr();
			auto subTmp = newTempXmmSd();

			const int64_t absMaskInt = 0x7FFFFFFFFFFFFFFF;
			auto absMask = cc.newDoubleConst(kConstScopeLocal, reinterpret_cast<const double&>(absMaskInt));
			auto absMaskXmm = newTempXmmPd();

			auto epsilon = cc.newDoubleConst(kConstScopeLocal, VM_EPSILON);
			auto epsilonXmm = newTempXmmSd();

			cc.mov(konstTmp, asmjit::imm_ptr(&konstf[C]));

			cc.movsd(subTmp, regF[B]);
			cc.subsd(subTmp, x86::qword_ptr(konstTmp));
			cc.movsd(absMaskXmm, absMask);
			cc.andpd(subTmp, absMaskXmm);
			cc.movsd(epsilonXmm, epsilon);
			cc.ucomisd(epsilonXmm, subTmp);

			if (check) cc.ja(fail);
			else       cc.jna(fail);
		}
	});
}

void JitCompiler::EmitLTF_RR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		if (static_cast<bool>(A & CMP_APPROX)) I_Error("CMP_APPROX not implemented for LTF_RR.\n");

		cc.ucomisd(regF[C], regF[B]);
		if (check) cc.ja(fail);
		else       cc.jna(fail);
	});
}

void JitCompiler::EmitLTF_RK()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		if (static_cast<bool>(A & CMP_APPROX)) I_Error("CMP_APPROX not implemented for LTF_RK.\n");

		auto constTmp = newTempIntPtr();
		auto xmmTmp = newTempXmmSd();
		cc.mov(constTmp, asmjit::imm_ptr(&konstf[C]));
		cc.movsd(xmmTmp, asmjit::x86::qword_ptr(constTmp));

		cc.ucomisd(xmmTmp, regF[B]);
		if (check) cc.ja(fail);
		else       cc.jna(fail);
	});
}

void JitCompiler::EmitLTF_KR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		if (static_cast<bool>(A & CMP_APPROX)) I_Error("CMP_APPROX not implemented for LTF_KR.\n");

		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(&konstf[B]));

		cc.ucomisd(regF[C], asmjit::x86::qword_ptr(tmp));
		if (check) cc.ja(fail);
		else       cc.jna(fail);
	});
}

void JitCompiler::EmitLEF_RR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		if (static_cast<bool>(A & CMP_APPROX)) I_Error("CMP_APPROX not implemented for LEF_RR.\n");

		cc.ucomisd(regF[C], regF[B]);
		if (check) cc.jae(fail);
		else       cc.jnae(fail);
	});
}

void JitCompiler::EmitLEF_RK()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		if (static_cast<bool>(A & CMP_APPROX)) I_Error("CMP_APPROX not implemented for LEF_RK.\n");

		auto constTmp = newTempIntPtr();
		auto xmmTmp = newTempXmmSd();
		cc.mov(constTmp, asmjit::imm_ptr(&konstf[C]));
		cc.movsd(xmmTmp, asmjit::x86::qword_ptr(constTmp));

		cc.ucomisd(xmmTmp, regF[B]);
		if (check) cc.jae(fail);
		else       cc.jnae(fail);
	});
}

void JitCompiler::EmitLEF_KR()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		if (static_cast<bool>(A & CMP_APPROX)) I_Error("CMP_APPROX not implemented for LEF_KR.\n");

		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(&konstf[B]));

		cc.ucomisd(regF[C], asmjit::x86::qword_ptr(tmp));
		if (check) cc.jae(fail);
		else       cc.jnae(fail);
	});
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Vector math. (2D)

void JitCompiler::EmitNEGV2()
{
	auto mask = ircontext->getConstantFloat(ircontext->getDoubleTy(), -0.0);
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regF[B]), mask), regF[A]);
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regF[B + 1]), mask), regF[A + 1]);
}

void JitCompiler::EmitADDV2_RR()
{
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B + 1]), cc.CreateLoad(regF[C + 1])), regF[A + 1]);
}

void JitCompiler::EmitSUBV2_RR()
{
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B + 1]), cc.CreateLoad(regF[C + 1])), regF[A + 1]);
}

void JitCompiler::EmitDOTV2_RR()
{
	cc.CreateStore(
		cc.CreateFAdd(
			cc.CreateFMul(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])),
			cc.CreateFMul(cc.CreateLoad(regF[B + 1]), cc.CreateLoad(regF[C + 1]))),
		regF[A]);
}

void JitCompiler::EmitMULVF2_RR()
{
	IRValue* rc = cc.CreateLoad(regF[C]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
}

void JitCompiler::EmitMULVF2_RK()
{
	IRValue* rc = ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
}

void JitCompiler::EmitDIVVF2_RR()
{
	IRValue* rc = cc.CreateLoad(regF[C]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
}

void JitCompiler::EmitDIVVF2_RK()
{
	IRValue* rc = ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
}

void JitCompiler::EmitLENV2()
{
	IRValue* x = cc.CreateLoad(regF[B]);
	IRValue* y = cc.CreateLoad(regF[B + 1]);
	IRValue* dotproduct = cc.CreateFAdd(cc.CreateFMul(x, x), cc.CreateFMul(y, y));
	IRValue* len = cc.CreateCall(GetNativeFunc<double, double>("__g_sqrt", g_sqrt), { dotproduct });
	cc.CreateStore(len, regF[A]);
}

#if 0
void JitCompiler::EmitEQV2_R()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		EmitVectorComparison<2> (check, fail, success);
	});
}
#endif

void JitCompiler::EmitEQV2_K()
{
	I_Error("EQV2_K is not used.");
}

/////////////////////////////////////////////////////////////////////////////
// Vector math. (3D)

void JitCompiler::EmitNEGV3()
{
	auto mask = ircontext->getConstantFloat(ircontext->getDoubleTy(), -0.0);
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regF[B]), mask), regF[A]);
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regF[B + 1]), mask), regF[A + 1]);
	cc.CreateStore(cc.CreateXor(cc.CreateLoad(regF[B + 2]), mask), regF[A + 2]);
}

void JitCompiler::EmitADDV3_RR()
{
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B + 1]), cc.CreateLoad(regF[C + 1])), regF[A + 1]);
	cc.CreateStore(cc.CreateFAdd(cc.CreateLoad(regF[B + 2]), cc.CreateLoad(regF[C + 2])), regF[A + 2]);
}

void JitCompiler::EmitSUBV3_RR()
{
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])), regF[A]);
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B + 1]), cc.CreateLoad(regF[C + 1])), regF[A + 1]);
	cc.CreateStore(cc.CreateFSub(cc.CreateLoad(regF[B + 2]), cc.CreateLoad(regF[C + 2])), regF[A + 2]);
}

void JitCompiler::EmitDOTV3_RR()
{
	cc.CreateStore(
		cc.CreateFAdd(
			cc.CreateFAdd(
				cc.CreateFMul(cc.CreateLoad(regF[B]), cc.CreateLoad(regF[C])),
				cc.CreateFMul(cc.CreateLoad(regF[B + 1]), cc.CreateLoad(regF[C + 1]))),
			cc.CreateFMul(cc.CreateLoad(regF[B + 2]), cc.CreateLoad(regF[C + 2]))),
		regF[A]);
}

void JitCompiler::EmitCROSSV_RR()
{
	IRValue* a0 = cc.CreateLoad(regF[B]);
	IRValue* a1 = cc.CreateLoad(regF[B + 1]);
	IRValue* a2 = cc.CreateLoad(regF[B + 2]);
	IRValue* b0 = cc.CreateLoad(regF[C]);
	IRValue* b1 = cc.CreateLoad(regF[C + 1]);
	IRValue* b2 = cc.CreateLoad(regF[C + 2]);
	cc.CreateStore(cc.CreateFSub(cc.CreateFMul(a1, b2), cc.CreateFMul(a2, b1)), regF[A]);
	cc.CreateStore(cc.CreateFSub(cc.CreateFMul(a2, b0), cc.CreateFMul(a0, b2)), regF[A + 1]);
	cc.CreateStore(cc.CreateFSub(cc.CreateFMul(a0, b1), cc.CreateFMul(a1, b0)), regF[A + 2]);
}

void JitCompiler::EmitMULVF3_RR()
{
	IRValue* rc = cc.CreateLoad(regF[C]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B + 2]), rc), regF[A + 2]);
}

void JitCompiler::EmitMULVF3_RK()
{
	IRValue* rc = ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
	cc.CreateStore(cc.CreateFMul(cc.CreateLoad(regF[B + 2]), rc), regF[A + 2]);
}

void JitCompiler::EmitDIVVF3_RR()
{
	IRValue* rc = cc.CreateLoad(regF[C]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B + 2]), rc), regF[A + 2]);
}

void JitCompiler::EmitDIVVF3_RK()
{
	IRValue* rc = ircontext->getConstantFloat(ircontext->getDoubleTy(), konstf[C]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B]), rc), regF[A]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B + 1]), rc), regF[A + 1]);
	cc.CreateStore(cc.CreateFDiv(cc.CreateLoad(regF[B + 2]), rc), regF[A + 2]);
}

void JitCompiler::EmitLENV3()
{
	IRValue* x = cc.CreateLoad(regF[B]);
	IRValue* y = cc.CreateLoad(regF[B + 1]);
	IRValue* z = cc.CreateLoad(regF[B + 2]);
	IRValue* dotproduct = cc.CreateFAdd(cc.CreateFAdd(cc.CreateFMul(x, x), cc.CreateFMul(y, y)), cc.CreateFMul(z, z));
	IRValue* len = cc.CreateCall(GetNativeFunc<double, double>("__g_sqrt", g_sqrt), { dotproduct });
	cc.CreateStore(len, regF[A]);
}

#if 0
void JitCompiler::EmitEQV3_R()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		EmitVectorComparison<3> (check, fail, success);
	});
}
#endif
	
void JitCompiler::EmitEQV3_K()
{
	I_Error("EQV3_K is not used.");
}

/////////////////////////////////////////////////////////////////////////////
// Vector math. (4D/Quaternion)

void JitCompiler::EmitNEGV4()
{
	auto mask = cc.newDoubleConst(asmjit::kConstScopeLocal, -0.0);
	auto maskXmm = newTempXmmSd();
	cc.movsd(maskXmm, mask);
	cc.movsd(regF[A], regF[B]);
	cc.xorpd(regF[A], maskXmm);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.xorpd(regF[A + 1], maskXmm);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.xorpd(regF[A + 2], maskXmm);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.xorpd(regF[A + 3], maskXmm);
}

void JitCompiler::EmitADDV4_RR()
{
	auto rc0 = CheckRegF(C, A);
	auto rc1 = CheckRegF(C + 1, A + 1);
	auto rc2 = CheckRegF(C + 2, A + 2);
	auto rc3 = CheckRegF(C + 3, A + 3);
	cc.movsd(regF[A], regF[B]);
	cc.addsd(regF[A], rc0);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.addsd(regF[A + 1], rc1);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.addsd(regF[A + 2], rc2);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.addsd(regF[A + 3], rc3);
}

void JitCompiler::EmitSUBV4_RR()
{
	auto rc0 = CheckRegF(C, A);
	auto rc1 = CheckRegF(C + 1, A + 1);
	auto rc2 = CheckRegF(C + 2, A + 2);
	auto rc3 = CheckRegF(C + 3, A + 3);
	cc.movsd(regF[A], regF[B]);
	cc.subsd(regF[A], rc0);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.subsd(regF[A + 1], rc1);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.subsd(regF[A + 2], rc2);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.subsd(regF[A + 3], rc3);
}

void JitCompiler::EmitDOTV4_RR()
{
	auto rb1 = CheckRegF(B + 1, A);
	auto rb2 = CheckRegF(B + 2, A);
	auto rb3 = CheckRegF(B + 3, A);
	auto rc0 = CheckRegF(C, A);
	auto rc1 = CheckRegF(C + 1, A);
	auto rc2 = CheckRegF(C + 2, A);
	auto rc3 = CheckRegF(C + 3, A);
	auto tmp = newTempXmmSd();
	cc.movsd(regF[A], regF[B]);
	cc.mulsd(regF[A], rc0);
	cc.movsd(tmp, rb1);
	cc.mulsd(tmp, rc1);
	cc.addsd(regF[A], tmp);
	cc.movsd(tmp, rb2);
	cc.mulsd(tmp, rc2);
	cc.addsd(regF[A], tmp);
	cc.movsd(tmp, rb3);
	cc.mulsd(tmp, rc3);
	cc.addsd(regF[A], tmp);
}

void JitCompiler::EmitMULVF4_RR()
{
	auto rc = CheckRegF(C, A, A + 1, A + 2, A + 3);
	cc.movsd(regF[A], regF[B]);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.mulsd(regF[A], rc);
	cc.mulsd(regF[A + 1], rc);
	cc.mulsd(regF[A + 2], rc);
	cc.mulsd(regF[A + 3], rc);
}

void JitCompiler::EmitMULVF4_RK()
{
	auto tmp = newTempIntPtr();
	cc.movsd(regF[A], regF[B]);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.mov(tmp, asmjit::imm_ptr(&konstf[C]));
	cc.mulsd(regF[A], asmjit::x86::qword_ptr(tmp));
	cc.mulsd(regF[A + 1], asmjit::x86::qword_ptr(tmp));
	cc.mulsd(regF[A + 2], asmjit::x86::qword_ptr(tmp));
	cc.mulsd(regF[A + 3], asmjit::x86::qword_ptr(tmp));
}

void JitCompiler::EmitDIVVF4_RR()
{
	auto rc = CheckRegF(C, A, A + 1, A + 2, A + 3);
	cc.movsd(regF[A], regF[B]);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.divsd(regF[A], rc);
	cc.divsd(regF[A + 1], rc);
	cc.divsd(regF[A + 2], rc);
	cc.divsd(regF[A + 3], rc);
}

void JitCompiler::EmitDIVVF4_RK()
{
	auto tmp = newTempIntPtr();
	cc.movsd(regF[A], regF[B]);
	cc.movsd(regF[A + 1], regF[B + 1]);
	cc.movsd(regF[A + 2], regF[B + 2]);
	cc.movsd(regF[A + 3], regF[B + 3]);
	cc.mov(tmp, asmjit::imm_ptr(&konstf[C]));
	cc.divsd(regF[A], asmjit::x86::qword_ptr(tmp));
	cc.divsd(regF[A + 1], asmjit::x86::qword_ptr(tmp));
	cc.divsd(regF[A + 2], asmjit::x86::qword_ptr(tmp));
	cc.divsd(regF[A + 3], asmjit::x86::qword_ptr(tmp));
}

void JitCompiler::EmitLENV4()
{
	auto rb1 = CheckRegF(B + 1, A);
	auto rb2 = CheckRegF(B + 2, A);
	auto rb3 = CheckRegF(B + 3, A);
	auto tmp = newTempXmmSd();
	cc.movsd(regF[A], regF[B]);
	cc.mulsd(regF[A], regF[B]);
	cc.movsd(tmp, rb1);
	cc.mulsd(tmp, rb1);
	cc.addsd(regF[A], tmp);
	cc.movsd(tmp, rb2);
	cc.mulsd(tmp, rb2);
	cc.addsd(regF[A], tmp);
	cc.movsd(tmp, rb3);
	cc.mulsd(tmp, rb3);
	cc.addsd(regF[A], tmp);
	CallSqrt(regF[A], regF[A]);
}

void JitCompiler::EmitEQV4_R()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		EmitVectorComparison<4> (check, fail, success);
	});
}

void JitCompiler::EmitEQV4_K()
{
	I_Error("EQV4_K is not used.");
}

// Quaternion ops
void FuncMULQQ(void *result, double ax, double ay, double az, double aw, double bx, double by, double bz, double bw)
{
	*reinterpret_cast<DQuaternion*>(result) = DQuaternion(ax, ay, az, aw) * DQuaternion(bx, by, bz, bw);
}

void FuncMULQV3(void *result, double ax, double ay, double az, double aw, double bx, double by, double bz)
{
	*reinterpret_cast<DVector3*>(result) = DQuaternion(ax, ay, az, aw) * DVector3(bx, by, bz);
}

void JitCompiler::EmitMULQQ_RR()
{
	auto stack = GetTemporaryVectorStackStorage();
	auto tmp = newTempIntPtr();
	cc.lea(tmp, stack);

	auto call = CreateCall<void, void*, double, double, double, double, double, double, double, double>(FuncMULQQ);
	call->setArg(0, tmp);
	call->setArg(1, regF[B + 0]);
	call->setArg(2, regF[B + 1]);
	call->setArg(3, regF[B + 2]);
	call->setArg(4, regF[B + 3]);
	call->setArg(5, regF[C + 0]);
	call->setArg(6, regF[C + 1]);
	call->setArg(7, regF[C + 2]);
	call->setArg(8, regF[C + 3]);

	cc.movsd(regF[A + 0], asmjit::x86::qword_ptr(tmp, 0));
	cc.movsd(regF[A + 1], asmjit::x86::qword_ptr(tmp, 8));
	cc.movsd(regF[A + 2], asmjit::x86::qword_ptr(tmp, 16));
	cc.movsd(regF[A + 3], asmjit::x86::qword_ptr(tmp, 24));
}

void JitCompiler::EmitMULQV3_RR()
{
	auto stack = GetTemporaryVectorStackStorage();
	auto tmp = newTempIntPtr();
	cc.lea(tmp, stack);
	
	auto call = CreateCall<void, void*, double, double, double, double, double, double, double>(FuncMULQV3);
	call->setArg(0, tmp);
	call->setArg(1, regF[B + 0]);
	call->setArg(2, regF[B + 1]);
	call->setArg(3, regF[B + 2]);
	call->setArg(4, regF[B + 3]);
	call->setArg(5, regF[C + 0]);
	call->setArg(6, regF[C + 1]);
	call->setArg(7, regF[C + 2]);

	cc.movsd(regF[A + 0], asmjit::x86::qword_ptr(tmp, 0));
	cc.movsd(regF[A + 1], asmjit::x86::qword_ptr(tmp, 8));
	cc.movsd(regF[A + 2], asmjit::x86::qword_ptr(tmp, 16));
}


/////////////////////////////////////////////////////////////////////////////
// Pointer math.

void JitCompiler::EmitADDA_RR()
{
	// Leave null pointers as null pointers

	IRBasicBlock* ifbb = irfunc->createBasicBlock({});
	IRBasicBlock* elsebb = irfunc->createBasicBlock({});
	IRBasicBlock* endbb = irfunc->createBasicBlock({});

	IRValue* nullvalue = ircontext->getConstantInt(ircontext->getInt64Ty(), 0);
	IRValue* ptr = cc.CreateLoad(regA[B]);

	cc.CreateCondBr(cc.CreateICmpEQ(ptr, nullvalue), ifbb, elsebb);
	cc.SetInsertPoint(ifbb);
	cc.CreateStore(ptr, regA[A]);
	cc.CreateBr(endbb);
	cc.SetInsertPoint(elsebb);
	cc.CreateStore(cc.CreateGEP(ptr, { cc.CreateLoad(regD[C]) }), regA[A]);
	cc.CreateBr(endbb);
	cc.SetInsertPoint(endbb);
}

void JitCompiler::EmitADDA_RK()
{
	// Leave null pointers as null pointers

	IRBasicBlock* ifbb = irfunc->createBasicBlock({});
	IRBasicBlock* elsebb = irfunc->createBasicBlock({});
	IRBasicBlock* endbb = irfunc->createBasicBlock({});

	IRValue* nullvalue = ircontext->getConstantInt(ircontext->getInt64Ty(), 0);
	IRValue* ptr = cc.CreateLoad(regA[B]);

	cc.CreateCondBr(cc.CreateICmpEQ(ptr, nullvalue), ifbb, elsebb);
	cc.SetInsertPoint(ifbb);
	cc.CreateStore(ptr, regA[A]);
	cc.CreateBr(endbb);
	cc.SetInsertPoint(elsebb);
	cc.CreateStore(cc.CreateGEP(ptr, { ircontext->getConstantInt(konstd[C]) }), regA[A]);
	cc.CreateBr(endbb);
	cc.SetInsertPoint(endbb);
}

void JitCompiler::EmitSUBA()
{
	cc.CreateStore(cc.CreateGEP(cc.CreateLoad(regA[B]), { cc.CreateNeg(cc.CreateLoad(regD[C])) }), regA[A]);
}

#if 0
void JitCompiler::EmitEQA_R()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		cc.cmp(regA[B], regA[C]);
		if (check) cc.je(fail);
		else       cc.jne(fail);
	});
}

void JitCompiler::EmitEQA_K()
{
	EmitComparisonOpcode([&](bool check, asmjit::Label& fail, asmjit::Label& success) {
		auto tmp = newTempIntPtr();
		cc.mov(tmp, asmjit::imm_ptr(konsta[C].v));
		cc.cmp(regA[B], tmp);
		if (check) cc.je(fail);
		else       cc.jne(fail);
	});
}

#endif
