
#pragma once

#include "Node.h"
#include "ParamsCall.h"
#include "Identifier.h"
#include "Variable.h"

class FunctionCall: public Node {
private:
	Identifier ident;
	ParamsCall *parameters;
	Node *symbol = NULL;
	Variable *leftValue = NULL;

public:
	FunctionCall(const string& name, ParamsCall *pc): ident(name) {
		parameters = pc;
		node_children.reserve(pc->getNumParams());
		node_children.insert(end(node_children), pc->getParameters().begin(),
			pc->getParameters().end());
	}
	
	virtual Value *generate(FunctionImpl *func, BasicBlock *block, BasicBlock *allocblock) override;

	virtual DataType getDataType() override;

	virtual void setLeftValue(Variable *symbol) override {
		leftValue = symbol;
	}

	std::vector<Node *> const& getParameters() {
		return parameters->getParameters();
	}

	void accept(Visitor& v) override;
};
