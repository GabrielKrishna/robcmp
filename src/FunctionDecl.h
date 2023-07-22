#ifndef __FUNCTIONDECLEXTERN_H__
#define __FUNCTIONDECLEXTERN_H__

#include "Node.h"

class FunctionDecl: public NamedNode {
private:
	BasicDataType tipo;
	FunctionParams *parameters;
public:
	FunctionDecl(BasicDataType tipo, string name, FunctionParams *fp) : NamedNode(name) {
		this->tipo = tipo;
		this->parameters = fp;
	}
	
	virtual bool isFunctionDecl() override {
		return true;
	}

	virtual Value *generate(Function *func, BasicBlock *block, BasicBlock *allocblock) override;

	FunctionParams const& getParameters() {
		return *parameters;
	}

	virtual BasicDataType getResultType(BasicBlock *block, BasicBlock *allocblock) override {
		return tipo;
	}
};

#endif

