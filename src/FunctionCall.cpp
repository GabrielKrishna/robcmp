#include "FunctionCall.h"
#include "Cast.h"
#include "FunctionDecl.h"
#include "FunctionImpl.h"
#include "HeaderGlobals.h"
#include "Coercion.h"
#include "Load.h"
#include "Visitor.h"
#include "BackLLVM.h"
#include "UserType.h"

DataType FunctionCall::getDataType() {
	
	if (dt == BuildTypes::undefinedType) {
		// is a cast or constructor?
		dt = buildTypes->getType(ident.getFullName());
		if (parameters->getNumParams() <= 1 && dt != BuildTypes::undefinedType) {
			return dt;
		}

		if (!symbol)
			symbol = ident.getSymbol(getScope());

		if (symbol)
			dt = symbol->getDataType();
	}
	return dt;
}

Value *FunctionCall::generate(FunctionImpl *func, BasicBlock *block, BasicBlock *allocblock) {

	RobDbgInfo.emitLocation(this);
	string name = ident.getFullName();

	// check if it is a cast call
	DataType adt = buildTypes->getType(name);
	if (adt != BuildTypes::undefinedType) {
		dt = adt;
		unsigned p = parameters->getNumParams();
		
		// call with only one parameter is a cast
		if (p == 1) {
			Cast ca(dt, parameters->getParamElement(0));
			ca.setScope(this);
			ca.children()[0]->setScope(&ca);
			return ca.generate(func, block, allocblock);

		} else if (p == 0) { // it's a constructor			
			// alloc
			Value *var = leftValue->getLLVMValue(func);
			if (var == NULL) {
				Builder->SetInsertPoint(allocblock);
				var = Builder->CreateAlloca(buildTypes->llvmType(dt), dataAddrSpace, 0, 
					leftValue->getName());
				leftValue->setAlloca(var);
				leftValue->setDataType(dt);
				if (debug_info)
					RobDbgInfo.declareVar(this, var, allocblock);
			}
			vector<Value*> args;
			args.push_back(var);

			// call #init
			Node *type = findSymbol(name);
			if (!type)
				return NULL;
			Node *fsymbol = type->findMember("init");
			if (!fsymbol)
				return NULL;
			FunctionBase *initfunc = dynamic_cast<FunctionBase*>(fsymbol);
			
			if (initfunc->needsParent()) {
				args.push_back(initfunc->findMember("#this")->getLLVMValue(NULL));
			}

			Builder->SetInsertPoint(block);
			Builder->CreateCall(initfunc->getLLVMFunction(), ArrayRef<Value*>(args));
			return NULL;
		}
	}

	if (!symbol)
		symbol = ident.getSymbol(getScope(), false);

	if (symbol == NULL) {
		yyerrorcpp("Function " + name + " not defined.", this);
		return NULL;
	}

	FunctionBase *fsymbol = dynamic_cast<FunctionBase*>(symbol);
	if (fsymbol == NULL) {
		yyerrorcpp("Symbol " + name + " is not a function.", this);
		return NULL;
	}

	dt = fsymbol->getDataType();

	int additionalParams = 0;
	Value *parent = NULL;
	DataType parentdt = BuildTypes::undefinedType;
	Value *stem = func->getThisArg();
	DataType stemdt = func->getThisArgDt();
	if (stem)
		additionalParams = 1;

	Builder->SetInsertPoint(block);
	if (ident.isComplex()) {
		Identifier istem = ident.getStem();
		Node *n = istem.getSymbol(getScope());
		
		if (buildTypes->isInternal(n->getDataType())) {
			parent = func->getThisArg();
			parentdt = func->getThisArgDt();
			stem = n->getLLVMValue(func);
			stemdt = n->getDataType();
			additionalParams = 2;
		}
		else {
			stem = n->getLLVMValue(func);
			stemdt = n->getDataType();
			additionalParams = 1;
		}
		
		// TODO: When accessing a.x.func(), need to load a and gep x
		//Load loadstem(ident.getStem());
		//loadstem.setParent(this->parent);
		//stem = loadstem.generate(func, block, allocblock);
	}

	if (fsymbol->getNumCodedParams() != parameters->getNumParams()) {
		yyerrorcpp(string_format("Function %s has %d argument(s) but was called with %d.",
			name.c_str(), fsymbol->getNumCodedParams(), 
			parameters->getNumParams()), this);
		yywarncpp("The function declaration is here.", symbol);
		return NULL;
	}

	vector<Value*> args;
	for (int i = 0; i < parameters->getNumParams(); i++){
		Value *valor = parameters->getParamElement(i)->generate(func, block, allocblock);
		if (!valor)
			return NULL;
			
		DataType pdt = fsymbol->getParameters().getParamType(i);
		if (buildTypes->isInterface(pdt)) {
			valor = Builder->CreateLoad(valor->getType()->getPointerTo(), valor, "defer");
		} else if (!buildTypes->isComplex(pdt)) {
			//TODO: we don't support cohercion between user types yet
			Type *pty = buildTypes->llvmType(pdt);
			valor = Coercion::Convert(valor, pty, block, this);
		}
		args.push_back(valor);
	}

	if (stem) {
		if (buildTypes->isInterface(stemdt))
			stem = Builder->CreateLoad(stem->getType()->getPointerTo(), stem, "defer");
		args.push_back(stem);
	}
	if (parent) {
		if (buildTypes->isInterface(parentdt))
			parent = Builder->CreateLoad(parent->getType()->getPointerTo(), parent, "defer");
		args.push_back(parent);
	}

	ArrayRef<Value*> argsRef(args);

	Builder->SetInsertPoint(allocblock);
	Value *vfunc = symbol->getLLVMValue(func);
	Function *cfunc = dyn_cast<Function>(vfunc);

	Builder->SetInsertPoint(block);
	return Builder->CreateCall(cfunc, argsRef);
}

void FunctionCall::accept(Visitor& v) {
	v.visit(*this);
}
