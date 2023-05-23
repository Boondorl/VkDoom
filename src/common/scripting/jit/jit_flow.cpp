
#include "jitintern.h"

void JitCompiler::EmitTEST()
{
	int i = (int)(ptrdiff_t)(pc - sfunc->Code);

	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(LoadD(A), ConstValueD(BC)), GetLabel(i + 2), continuebb);
	cc.SetInsertPoint(continuebb);
}

void JitCompiler::EmitTESTN()
{
	int bc = BC;
	int i = (int)(ptrdiff_t)(pc - sfunc->Code);

	auto continuebb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpEQ(LoadD(A), ConstValueD(-bc)), GetLabel(i + 2), continuebb);
	cc.SetInsertPoint(continuebb);
}

void JitCompiler::EmitJMP()
{
	auto dest = pc + JMPOFS(pc) + 1;
	int i = (int)(ptrdiff_t)(dest - sfunc->Code);
	cc.CreateBr(GetLabel(i));
	cc.SetInsertPoint(nullptr);
}

void JitCompiler::EmitIJMP()
{
	int base = (int)(ptrdiff_t)(pc - sfunc->Code) + 1;
	IRValue* val = LoadD(A);

	for (int i = 0; i < (int)BCs; i++)
	{
		if (sfunc->Code[base +i].op == OP_JMP)
		{
			int target = base + i + JMPOFS(&sfunc->Code[base + i]) + 1;

			IRBasicBlock* elsebb = irfunc->createBasicBlock({});
			cc.CreateCondBr(cc.CreateICmpEQ(val, ConstValueD(i)), GetLabel(target), elsebb);
			cc.SetInsertPoint(elsebb);
		}
	}
	pc += BCs;

	// This should never happen. It means we are jumping to something that is not a JMP instruction!
	EmitThrowException(X_OTHER);
}

static void ValidateCall(DObject *o, VMFunction *f, int b)
{
	FScopeBarrier::ValidateCall(o->GetClass(), f, b - 1);
}

void JitCompiler::EmitSCOPE()
{
	auto continuebb = irfunc->createBasicBlock({});
	auto exceptionbb = EmitThrowExceptionLabel(X_READ_NIL);
	cc.CreateCondBr(cc.CreateICmpEQ(LoadD(A), ConstValueA(0)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
	cc.CreateCall(GetNativeFunc<void, DObject*, VMFunction*, int>("__ValidateCall", ValidateCall), { LoadA(A), ConstA(C), ConstValueD(B) });
}

static void SetString(VMReturn* ret, FString* str)
{
	ret->SetString(*str);
}

void JitCompiler::EmitRET()
{
	if (B == REGT_NIL)
	{
		EmitPopFrame();
		cc.CreateRet(ConstValueD(0));
	}
	else
	{
		int a = A;
		int retnum = a & ~RET_FINAL;

		IRBasicBlock* ifbb = irfunc->createBasicBlock({});
		IRBasicBlock* endifbb = irfunc->createBasicBlock({});

		cc.CreateCondBr(cc.CreateICmpSLT(ConstValueD(retnum), numret), ifbb, endifbb);
		cc.SetInsertPoint(ifbb);

		IRValue* location = Load(ToInt8PtrPtr(ret, retnum * sizeof(VMReturn)));

		int regtype = B;
		int regnum = C;
		switch (regtype & REGT_TYPE)
		{
		case REGT_INT:
			if (regtype & REGT_KONST)
				Store(ConstD(regnum), ToInt32Ptr(location));
			else
				Store(LoadD(regnum), ToInt32Ptr(location));
			break;
		case REGT_FLOAT:
			if (regtype & REGT_KONST)
			{
				if (regtype & REGT_MULTIREG4)
				{
					StoreDouble(ConstF(regnum), ToDoublePtr(location));
					StoreDouble(ConstF(regnum + 1), ToDoublePtr(location, 8));
					StoreDouble(ConstF(regnum + 2), ToDoublePtr(location, 16));
					StoreDouble(ConstF(regnum + 3), ToDoublePtr(location, 24));
				}
				else if (regtype & REGT_MULTIREG3)
				{
					Store(ConstF(regnum), ToDoublePtr(location));
					Store(ConstF(regnum + 1), ToDoublePtr(location, 8));
					Store(ConstF(regnum + 2), ToDoublePtr(location, 16));
				}
				else if (regtype & REGT_MULTIREG2)
				{
					Store(ConstF(regnum), ToDoublePtr(location));
					Store(ConstF(regnum + 1), ToDoublePtr(location, 8));
				}
				else
				{
					Store(ConstF(regnum), ToDoublePtr(location));
				}
			}
			else
			{
				if (regtype & REGT_MULTIREG4)
				{
					StoreDouble(LoadF(regnum), ToDoublePtr(location));
					StoreDouble(LoadF(regnum + 1), ToDoublePtr(location, 8));
					StoreDouble(LoadF(regnum + 2), ToDoublePtr(location, 16));
					StoreDouble(LoadF(regnum + 3), ToDoublePtr(location, 24));
				}
				else if (regtype & REGT_MULTIREG3)
				{
					Store(LoadF(regnum), ToDoublePtr(location));
					Store(LoadF(regnum + 1), ToDoublePtr(location, 8));
					Store(LoadF(regnum + 2), ToDoublePtr(location, 16));
				}
				else if (regtype & REGT_MULTIREG2)
				{
					Store(LoadF(regnum), ToDoublePtr(location));
					Store(LoadF(regnum + 1), ToDoublePtr(location, 8));
				}
				else
				{
					Store(LoadF(regnum), ToDoublePtr(location));
				}
			}
			break;
		case REGT_STRING:
		{
			cc.CreateCall(GetNativeFunc<void, VMReturn*, FString*>("__SetString", SetString), { location, (regtype & REGT_KONST) ? ConstS(regnum) : LoadS(regnum) });
			break;
		}
		case REGT_POINTER:
			if (regtype & REGT_KONST)
			{
				Store(ConstA(regnum), ToInt8PtrPtr(location));
			}
			else
			{
				Store(LoadA(regnum), ToInt8PtrPtr(location));
			}
			break;
		}

		if (a & RET_FINAL)
		{
			EmitPopFrame();
			cc.CreateRet(ConstValueD(retnum + 1));
		}
		else
		{
			cc.CreateBr(endifbb);
		}

		cc.SetInsertPoint(endifbb);

		if (a & RET_FINAL)
		{
			EmitPopFrame();
			cc.CreateRet(numret);
		}
	}
}

void JitCompiler::EmitRETI()
{
	int a = A;
	int retnum = a & ~RET_FINAL;

	IRBasicBlock* ifbb = irfunc->createBasicBlock({});
	IRBasicBlock* endifbb = irfunc->createBasicBlock({});

	cc.CreateCondBr(cc.CreateICmpSLT(ConstValueD(retnum), numret), ifbb, endifbb);
	cc.SetInsertPoint(ifbb);

	IRValue* location = Load(ToInt8PtrPtr(ret, retnum * sizeof(VMReturn)));
	Store(ConstValueD(BCs), ToInt32Ptr(location));

	if (a & RET_FINAL)
	{
		EmitPopFrame();
		cc.CreateRet(ConstValueD(retnum + 1));
	}
	else
	{
		cc.CreateBr(endifbb);
	}

	cc.SetInsertPoint(endifbb);

	if (a & RET_FINAL)
	{
		EmitPopFrame();
		cc.CreateRet(numret);
	}
}

void JitCompiler::EmitTHROW()
{
	EmitThrowException(EVMAbortException(BC));
}

void JitCompiler::EmitBOUND()
{
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	IRBasicBlock* exceptionbb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpUGE(LoadD(A), ConstValueD(BC)), exceptionbb, continuebb);
	cc.SetInsertPoint(exceptionbb);
	cc.CreateCall(GetNativeFunc<void, int, int>("__ThrowArrayOutOfBounds", &JitCompiler::ThrowArrayOutOfBounds), { LoadD(A), ConstValueD(BC) });
	exceptionbb->code.front()->lineNumber = sfunc->PCToLine(pc);
	cc.CreateBr(continuebb);
	cc.SetInsertPoint(continuebb);
}

void JitCompiler::EmitBOUND_K()
{
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	IRBasicBlock* exceptionbb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpUGE(LoadD(A), ConstD(BC)), exceptionbb, continuebb);
	cc.SetInsertPoint(exceptionbb);
	cc.CreateCall(GetNativeFunc<void, int, int>("__ThrowArrayOutOfBounds", &JitCompiler::ThrowArrayOutOfBounds), { LoadD(A), ConstD(BC) });
	exceptionbb->code.front()->lineNumber = sfunc->PCToLine(pc);
	cc.CreateBr(continuebb);
	cc.SetInsertPoint(continuebb);
}

void JitCompiler::EmitBOUND_R()
{
	IRBasicBlock* continuebb = irfunc->createBasicBlock({});
	IRBasicBlock* exceptionbb = irfunc->createBasicBlock({});
	cc.CreateCondBr(cc.CreateICmpUGE(LoadD(A), LoadD(B)), exceptionbb, continuebb);
	cc.SetInsertPoint(exceptionbb);
	cc.CreateCall(GetNativeFunc<void, int, int>("__ThrowArrayOutOfBounds", &JitCompiler::ThrowArrayOutOfBounds), { LoadD(A), LoadD(BC) });
	exceptionbb->code.front()->lineNumber = sfunc->PCToLine(pc);
	cc.CreateBr(continuebb);
	cc.SetInsertPoint(continuebb);
}

void JitCompiler::ThrowArrayOutOfBounds(int index, int size)
{
	if (index >= size)
	{
		ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Size = %u, current index = %u\n", size, index);
	}
	else
	{
		ThrowAbortException(X_ARRAY_OUT_OF_BOUNDS, "Negative current index = %i\n", index);
	}
}
