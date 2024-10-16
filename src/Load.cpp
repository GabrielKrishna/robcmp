#include "Header.h"

Type* Load::getLLVMResultType(BasicBlock *block, BasicBlock *allocblock) {
	auto rsym = search_symbol(ident, allocblock, block);
	if (rsym) {
		if (auto *lvalue = dyn_cast<AllocaInst>(rsym->value))
			return lvalue->getAllocatedType();
		else if (auto *lvalue = dyn_cast<GlobalVariable>(rsym->value))
			return lvalue->getValueType();
		else
			return rsym->pointerType;
	}
	return NULL;
}

Value* Load::generate(Function *func, BasicBlock *block, BasicBlock *allocblock) {
	auto rsym = search_symbol(ident, allocblock, block);
	if (rsym == NULL) {
		yyerrorcpp("Variable " + ident + " not defined.", this);
		return NULL;
	}

	auto sym = rsym->value;
	if (rsym->qualifier == qconst)
		return sym;

	if (block == NULL && (allocblock == NULL || allocblock == global_alloc)) {
		// trying to load a variable to initialize a global one.
		// permitted only for const globals
		if (rsym->qualifier != qconst) {
			yyerrorcpp("Can't not use '" + ident + "' to define another var/const in global context.", this);
			return NULL;
		}
	}

	LoadInst *ret = NULL;
	bool vol = rsym->qualifier == qvolatile;
	if (auto *lvalue = dyn_cast<AllocaInst>(sym))
		ret = new LoadInst(lvalue->getAllocatedType(), sym, ident, vol, block);
	else if (auto *lvalue = dyn_cast<GlobalVariable>(sym))
		ret = new LoadInst(lvalue->getValueType(), sym, ident, vol, block);
	else if (rsym->pointerType != NULL)
		ret = new LoadInst(rsym->pointerType, sym, ident, vol, block);
	else {
		printf("ERR: Going to return NULL!\n");
	}
		
	return ret;
}

void Load::accept(Visitor &v) {
	v.visit(*this); 
}

bool Load::isConstExpr(BasicBlock *block, BasicBlock *allocblock) {
	RobSymbol *rsym = search_symbol(ident, block, allocblock);
	return rsym && (dyn_cast<Constant>(rsym->value) != NULL);
}
