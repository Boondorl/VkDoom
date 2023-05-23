
#include "jit.h"
#include "jitintern.h"
#include "printf.h"

extern PString *TypeString;
extern PStruct *TypeVector2;
extern PStruct *TypeVector3;
extern PStruct* TypeVector4;
extern PStruct* TypeQuaternion;
extern PStruct* TypeFQuaternion;

IRContext* JitGetIRContext();

JitFuncPtr JitCompile(VMScriptFunction* sfunc)
{
#if 0
	if (strcmp(sfunc->PrintableName.GetChars(), "StatusScreen.drawNum") != 0)
		return nullptr;
#endif

	try
	{
		IRContext* context = JitGetIRContext();
		JitCompiler compiler(context, sfunc);
		IRFunction* func = compiler.Codegen();
		context->codegen();
		return reinterpret_cast<JitFuncPtr>(context->getPointerToFunction(func));
	}
	catch (...)
	{
		Printf("%s: Unexpected JIT error encountered\n", sfunc->PrintableName.GetChars());
		throw;
	}
}

void JitDumpLog(FILE* file, VMScriptFunction* sfunc)
{
	try
	{
		IRContext* context = JitGetIRContext();
		JitCompiler compiler(context, sfunc);
		IRFunction* func = compiler.Codegen();
		std::string text = context->getFunctionAssembly(func);
		fwrite(text.data(), text.size(), 1, file);
	}
	catch (const std::exception& e)
	{
		FString err;
		err.Format("Unexpected JIT error: %s\n", e.what());
		fwrite(err.GetChars(), err.Len(), 1, file);
		fclose(file);

		I_FatalError("Unexpected JIT error: %s\n", e.what());
	}
}

/*
static void OutputJitLog(const char *text)
{
	// Write line by line since I_FatalError seems to cut off long strings
	const char *pos = text;
	const char *end = pos;
	while (*end)
	{
		if (*end == '\n')
		{
			FString substr(pos, (int)(ptrdiff_t)(end - pos));
			Printf("%s\n", substr.GetChars());
			pos = end + 1;
		}
		end++;
	}
	if (pos != end)
		Printf("%s\n", pos);
}
*/

/////////////////////////////////////////////////////////////////////////////

static const char *OpNames[NUM_OPS] =
{
#define xx(op, name, mode, alt, kreg, ktype)	#op,
#include "vmops.h"
#undef xx
};

IRFunction* JitCompiler::Codegen()
{
	Setup();

	int lastLine = -1;

	pc = sfunc->Code;
	auto end = pc + sfunc->CodeSize;
	while (pc != end)
	{
		int i = (int)(ptrdiff_t)(pc - sfunc->Code);
		op = pc->op;

		if (labels[i].block) // This is already a known jump target
		{
			if (cc.GetInsertBlock())
				cc.CreateBr(labels[i].block);
			cc.SetInsertPoint(labels[i].block);
		}
		else
		{
			if (cc.GetInsertBlock())
				cc.SetInsertPoint(irfunc->createBasicBlock({}));

			// Save start location in case GetLabel gets called later
			labels[i].block = cc.GetInsertBlock();
			if (!labels[i].block->code.empty())
				labels[i].instbefore = labels[i].block->code.back();
		}

		int curLine = sfunc->PCToLine(pc);
		if (curLine != lastLine)
		{
			lastLine = curLine;

			/*auto label = cc.newLabel();
			cc.bind(label);

			JitLineInfo info;
			info.Label = label;
			info.LineNumber = curLine;
			LineInfo.Push(info);*/
		}

#if 0
		if (op != OP_PARAM && op != OP_PARAMI && op != OP_VTBL)
		{
			FString lineinfo;
			lineinfo.Format("; line %d: %02x%02x%02x%02x %s", curLine, pc->op, pc->a, pc->b, pc->c, OpNames[op]);
			cc.comment("", 0);
			cc.comment(lineinfo.GetChars(), lineinfo.Len());
		}

		labels[i].cursor = cc.getCursor();
#endif

		EmitOpcode();

		pc++;
	}

	BindLabels();

	/*
	auto code = cc.getCode ();
	for (unsigned int j = 0; j < LineInfo.Size (); j++)
	{
		auto info = LineInfo[j];

		if (!code->isLabelValid (info.Label))
		{
			continue;
		}

		info.InstructionIndex = code->getLabelOffset (info.Label);

		LineInfo[j] = info;
	}

	std::stable_sort(LineInfo.begin(), LineInfo.end(), [](const JitLineInfo &a, const JitLineInfo &b) { return a.InstructionIndex < b.InstructionIndex; });
	*/

	return irfunc;
}

void JitCompiler::EmitOpcode()
{
	switch (op)
	{
		#define xx(op, name, mode, alt, kreg, ktype)	case OP_##op: Emit##op(); break;
		#include "vmops.h"
		#undef xx

	default:
		I_FatalError("JIT error: Unknown VM opcode %d\n", op);
		break;
	}
}

void JitCompiler::BindLabels()
{
#if 0
	asmjit::CBNode *cursor = cc.getCursor();
	unsigned int size = labels.Size();
	for (unsigned int i = 0; i < size; i++)
	{
		const OpcodeLabel &label = labels[i];
		if (label.inUse)
		{
			cc.setCursor(label.cursor);
			cc.bind(label.label);
		}
	}
	cc.setCursor(cursor);
#endif
}

void JitCompiler::CheckVMFrame()
{
	if (!vmframe)
	{
		vmframe = irfunc->createAlloca(ircontext->getInt8Ty(), ircontext->getConstantInt(sfunc->StackSize), "vmframe");
	}
}

IRValue* JitCompiler::GetCallReturns()
{
	if (!callReturns)
	{
		callReturns = irfunc->createAlloca(ircontext->getInt8Ty(), ircontext->getConstantInt(sizeof(VMReturn) * MAX_RETURNS), "callReturns");
	}
	return callReturns;
}

IRBasicBlock* JitCompiler::GetLabel(size_t pos)
{
	if (labels[pos].instbefore)
	{
		IRValue* instbefore = labels[pos].instbefore;
		IRBasicBlock* block = labels[pos].block;

		// If inst isn't the first then we need to split the basicblock. Jump targets can only be the start of a basicblock.
		if (instbefore)
		{
			auto it = block->code.end();
			for (size_t i = 0; i < block->code.size(); i++)
			{
				if (block->code[i] == instbefore)
				{
					it = block->code.begin() + (i + 1);
					break;
				}
			}

			// Split basic block into two
			IRBasicBlock* newlabelbb = irfunc->createBasicBlock({});
			newlabelbb->code.insert(newlabelbb->code.begin(), it, block->code.end());
			block->code.erase(it, block->code.end());

			// Jump from prev block to next
			IRBasicBlock* old = cc.GetInsertBlock();
			cc.SetInsertPoint(block);
			cc.CreateBr(newlabelbb);
			cc.SetInsertPoint(old);
		}

		labels[pos].instbefore = nullptr;
	}
	else if (!labels[pos].block)
	{
		labels[pos].block = irfunc->createBasicBlock({});
	}

	return labels[pos].block;
}

void JitCompiler::Setup()
{
	//static const char *marks = "=======================================================";
	//cc.comment("", 0);
	//cc.comment(marks, 56);

	//FString funcname;
	//funcname.Format("Function: %s", sfunc->PrintableName.GetChars());
	//cc.comment(funcname.GetChars(), funcname.Len());

	//cc.comment(marks, 56);
	//cc.comment("", 0);

	irfunc = ircontext->createFunction(GetFunctionType5<int, VMFunction*, void*, int, void*, int>(), sfunc->PrintableName.GetChars());
	args = irfunc->args[1];
	numargs = irfunc->args[2];
	ret = irfunc->args[3];
	numret = irfunc->args[4];

	konstd = sfunc->KonstD;
	konstf = sfunc->KonstF;
	konsts = sfunc->KonstS;
	konsta = sfunc->KonstA;

	labels.Resize(sfunc->CodeSize);

	cc.SetInsertPoint(irfunc->createBasicBlock("entry"));

	CreateRegisters();
	IncrementVMCalls();
	SetupFrame();
}

void JitCompiler::SetupFrame()
{
	// the VM version reads this from the stack, but it is constant data
	offsetParams = ((int)sizeof(VMFrame) + 15) & ~15;
	offsetF = offsetParams + (int)(sfunc->MaxParam * sizeof(VMValue));
	offsetS = offsetF + (int)(sfunc->NumRegF * sizeof(double));
	offsetA = offsetS + (int)(sfunc->NumRegS * sizeof(FString));
	offsetD = offsetA + (int)(sfunc->NumRegA * sizeof(void*));
	offsetExtra = (offsetD + (int)(sfunc->NumRegD * sizeof(int32_t)) + 15) & ~15;

	if (sfunc->SpecialInits.Size() == 0 && sfunc->NumRegS == 0 && sfunc->ExtraSpace == 0)
	{
		SetupSimpleFrame();
	}
	else
	{
		SetupFullVMFrame();
	}
}

void JitCompiler::SetupSimpleFrame()
{
	// This is a simple frame with no constructors or destructors. Allocate it on the stack ourselves.

	int argsPos = 0;
	int regd = 0, regf = 0, rega = 0;
	for (unsigned int i = 0; i < sfunc->Proto->ArgumentTypes.Size(); i++)
	{
		const PType *type = sfunc->Proto->ArgumentTypes[i];
		if (sfunc->ArgFlags.Size() && sfunc->ArgFlags[i] & (VARF_Out | VARF_Ref))
		{
			StoreA(Load(ToPtrPtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, a))), rega++);
		}
		else if (type == TypeVector2 || type == TypeFVector2)
		{
			StoreF(Load(ToDoublePtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f))), regf++);
			StoreF(Load(ToDoublePtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f))), regf++);
		}
		else if (type == TypeVector3 || type == TypeFVector3)
		{
			StoreF(Load(ToDoublePtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f))), regf++);
			StoreF(Load(ToDoublePtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f))), regf++);
			StoreF(Load(ToDoublePtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f))), regf++);
		}
		else if (type == TypeVector4 || type == TypeFVector4 || type == TypeQuaternion || type == TypeFQuaternion)
		{
			cc.movsd(regF[regf++], x86::qword_ptr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f)));
			cc.movsd(regF[regf++], x86::qword_ptr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f)));
			cc.movsd(regF[regf++], x86::qword_ptr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f)));
			cc.movsd(regF[regf++], x86::qword_ptr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f)));
		}
		else if (type == TypeFloat64)
		{
			StoreF(Load(ToDoublePtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, f))), regf++);
		}
		else if (type == TypeString)
		{
			I_FatalError("JIT: Strings are not supported yet for simple frames");
		}
		else if (type->isIntCompatible())
		{
			StoreD(Load(ToInt32Ptr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, i))), regd++);
		}
		else
		{
			StoreA(Load(ToPtrPtr(args, argsPos++ * sizeof(VMValue) + offsetof(VMValue, a))), rega++);
		}
	}

	const char *errorDetails = nullptr;

	if (sfunc->NumArgs != argsPos)
	{
		errorDetails = "arguments";
	}
	else if (regd > sfunc->NumRegD)
	{
		errorDetails = "integer registers";
	}
	else if (regf > sfunc->NumRegF)
	{
		errorDetails = "floating point registers";
	}
	else if (rega > sfunc->NumRegA)
	{
		errorDetails = "address registers";
	}

	if (errorDetails)
	{
		I_FatalError("JIT: inconsistent number of %s for function %s", errorDetails, sfunc->PrintableName.GetChars());
	}

	for (int i = regd; i < sfunc->NumRegD; i++)
		StoreD(ConstValueD(0), i);

	for (int i = regf; i < sfunc->NumRegF; i++)
		StoreF(ConstValueF(0.0), i);

	for (int i = rega; i < sfunc->NumRegA; i++)
		StoreA(ConstValueA(nullptr), i);
}

static VMFrameStack *CreateFullVMFrame(VMScriptFunction *func, VMValue *args, int numargs)
{
	VMFrameStack *stack = &GlobalVMStack;
	VMFrame *newf = stack->AllocFrame(func);
	VMFillParams(args, newf, numargs);
	return stack;
}

void JitCompiler::SetupFullVMFrame()
{
	vmframestack = cc.CreateCall(GetNativeFunc<VMFrameStack*, VMScriptFunction*, VMValue*, int>("__CreateFullVMFrame", CreateFullVMFrame), { ConstValueA(sfunc), args, numargs });

	IRValue* Blocks = Load(ToPtrPtr(vmframestack)); // vmframestack->Blocks
	vmframe = Load(ToPtrPtr(Blocks, VMFrameStack::OffsetLastFrame())); // Blocks->LastFrame

	for (int i = 0; i < sfunc->NumRegD; i++)
		StoreD(Load(ToInt32Ptr(vmframe, offsetD + i * sizeof(int32_t))), i);

	for (int i = 0; i < sfunc->NumRegF; i++)
		StoreF(Load(ToDoublePtr(vmframe, offsetF + i * sizeof(double))), i);

	for (int i = 0; i < sfunc->NumRegS; i++)
		StoreS(Load(ToPtrPtr(vmframe, offsetS + i * sizeof(FString))), i);

	for (int i = 0; i < sfunc->NumRegA; i++)
		StoreA(Load(ToPtrPtr(vmframe, offsetA + i * sizeof(void*))), i);
}

static void PopFullVMFrame(VMFrameStack * vmframestack)
{
	vmframestack->PopFrame();
}

void JitCompiler::EmitPopFrame()
{
	if (sfunc->SpecialInits.Size() != 0 || sfunc->NumRegS != 0 || sfunc->ExtraSpace != 0)
	{
		cc.CreateCall(GetNativeFunc<void, VMFrameStack*>("__PopFullVMFrame", PopFullVMFrame), { vmframestack });
	}
}

void JitCompiler::IncrementVMCalls()
{
	// VMCalls[0]++
	IRValue* vmcallsptr = ircontext->getConstantInt(ircontext->getInt32PtrTy(), (uint64_t)VMCalls);
	cc.CreateStore(cc.CreateAdd(cc.CreateLoad(vmcallsptr), ircontext->getConstantInt(1)), vmcallsptr);
}

void JitCompiler::CreateRegisters()
{
	regD.Resize(sfunc->NumRegD);
	regF.Resize(sfunc->NumRegF);
	regA.Resize(sfunc->NumRegA);
	regS.Resize(sfunc->NumRegS);

	FString regname;
	IRType* type;
	IRValue* arraySize = ircontext->getConstantInt(1);

	type = ircontext->getInt32Ty();
	for (int i = 0; i < sfunc->NumRegD; i++)
	{
		regname.Format("regD%d", i);
		regD[i] = irfunc->createAlloca(type, arraySize, regname.GetChars());
	}

	type = ircontext->getDoubleTy();
	for (int i = 0; i < sfunc->NumRegF; i++)
	{
		regname.Format("regF%d", i);
		regF[i] = irfunc->createAlloca(type, arraySize, regname.GetChars());
	}

	type = ircontext->getInt8PtrTy();
	for (int i = 0; i < sfunc->NumRegS; i++)
	{
		regname.Format("regS%d", i);
		regS[i] = irfunc->createAlloca(type, arraySize, regname.GetChars());
	}

	for (int i = 0; i < sfunc->NumRegA; i++)
	{
		regname.Format("regA%d", i);
		regA[i] = irfunc->createAlloca(type, arraySize, regname.GetChars());
	}
}

void JitCompiler::EmitNullPointerThrow(int index, EVMAbortException reason)
{
	auto continuebb = irfunc->createBasicBlock({});
	auto exceptionbb = EmitThrowExceptionLabel(reason);
	cc.CreateCondBr(cc.CreateICmpEQ(LoadA(index), ConstValueA(nullptr)), exceptionbb, continuebb);
	cc.SetInsertPoint(continuebb);
}

void JitCompiler::EmitThrowException(EVMAbortException reason)
{
	cc.CreateCall(GetNativeFunc<void, int>("__ThrowException", &JitCompiler::ThrowException), { ConstValueD(reason) });
}

void JitCompiler::ThrowException(int reason)
{
	ThrowAbortException((EVMAbortException)reason, nullptr);
}

IRBasicBlock* JitCompiler::EmitThrowExceptionLabel(EVMAbortException reason)
{
	auto bb = irfunc->createBasicBlock({});
	auto cursor = cc.GetInsertBlock();
	cc.SetInsertPoint(bb);
	//JitLineInfo info;
	//info.Label = bb;
	//info.LineNumber = sfunc->PCToLine(pc);
	//LineInfo.Push(info);
	EmitThrowException(reason);
	cc.SetInsertPoint(cursor);
	return bb;
}

void JitCompiler::EmitNOP()
{
	// The IR doesn't have a NOP instruction
}
