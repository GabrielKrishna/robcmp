#include "Coercion.h"
#include "HeaderGlobals.h"
#include "BuildTypes.h"

unsigned Coercion::GetFloatingPointBitwidth(Type *ty) {
	switch (ty->getTypeID()) {
		case Type::FloatTyID:
			return 32;
		case Type::DoubleTyID:
			return 64;
		case Type::X86_FP80TyID:
			return 80;
		case Type::PPC_FP128TyID:
		case Type::FP128TyID:
			return 128;
		default:
			return 0;
	}
}

Value *Coercion::Convert(Value *v, Type *destty, BasicBlock *block, SourceLocation *loc, bool isCast){

	Value *r = v;
	Type *ty = v->getType();

	RobDbgInfo.emitLocation(loc);
	Builder->SetInsertPoint(block);
	
	if (ty != destty){
		//Float to Integer
		if (ty->isFloatingPointTy() && destty->isIntegerTy()){
			r = Builder->CreateFPToSI(v, destty, "fptosi");
			if (!isCast)
				yywarncpp("Float point converted to integer.", loc);
		}
		//Integer to Float
		else if (destty->isFloatingPointTy() && ty->isIntegerTy()){
			r = Builder->CreateSIToFP(v, destty, "sitofp");
		}
		//Floating point to Floating point
		else if (ty->isFloatingPointTy() && destty->isFloatingPointTy()) {
			unsigned tybw = GetFloatingPointBitwidth(ty);
			unsigned dtybw = GetFloatingPointBitwidth(destty);
			if (dtybw > tybw)
				r = Builder->CreateFPExt(v, destty, "fpext");
			else if (dtybw < tybw) {
				r = Builder->CreateFPTrunc(v, destty, "fptrunc");
				if (!isCast)
					yywarncpp("Float point value truncated.", loc);
			}
		}
		//Generic ExtInt to Int
		else if (destty->isIntegerTy() && ty->isIntegerTy()){
			unsigned wty = ty->getIntegerBitWidth();
			unsigned wdestty = destty->getIntegerBitWidth();
			if (wty > wdestty){
				r = Builder->CreateTrunc(v, destty, "trunc");
				if (!isCast) {
					yywarncpp(string_format("Integer value truncated from int%d to int%d.", 
						wty, wdestty), loc);
				}
			}
			else if (wty < wdestty) {
				r = Builder->CreateSExt(v, destty, "sext");
			}
		}
		else {
			yyerrorcpp("No coercion between " + getTypeName(ty) + " and " + 
				getTypeName(destty) + " implemented.", loc);
		}
	} else {
		// check composite types
		bool distinct = ty->getNumContainedTypes() != destty->getNumContainedTypes();
		if (!distinct && ty->getNumContainedTypes() > 0) {
			for(int i = 0; i < ty->getNumContainedTypes(); i++) {
				auto ity = ty->getContainedType(i);
				auto dity = destty->getContainedType(i);
				if (ity != dity) {
					distinct = true;
				} else {
					if (ity->isIntegerTy() && dity->isIntegerTy()) {
						unsigned wity = ity->getIntegerBitWidth();
						unsigned wdity = dity->getIntegerBitWidth();
						if (wity != wdity)
							distinct = true;
					} else if (ity->isFloatingPointTy() && dity->isFloatingPointTy()) {
						unsigned wity = GetFloatingPointBitwidth(ity);
						unsigned wdity = GetFloatingPointBitwidth(dity);
						if (wity != wdity)
							distinct = true;
					}
				}
				if (distinct)
					break;
			}
			if (distinct)
				yyerrorcpp("No coercion between " + getTypeName(ty) + " and " + 
					getTypeName(destty) + " implemented.", loc);
		}
	}
	return r;
}
