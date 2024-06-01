#pragma once
#define CMINUS_H
#include "parser.h"
#include "ast.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include <variant>
#include "src/Environment.h"
class Cminus {
public:
	Cminus(const std::string& input) :parser(std::make_unique<Parser>(input)) {
		moduleInit();
		setupExternalFunctions();
		setupGlobalEnvironment();
	}
	void exec() {
		auto ast = parser->ParserProgram();
		compile(ast);
		module->print(llvm::outs(), nullptr);
		saveModuleToFile("./out.ll");
	}
private:
	void moduleInit(void) {
		ctx = std::make_unique<llvm::LLVMContext>();
		module = std::make_unique<llvm::Module>("cminus", *ctx);
		builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
		variableBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
	}
	void setupExternalFunctions() {
		module->getOrInsertFunction("printf", llvm::FunctionType::get(
			/* return type*/builder->getInt32Ty(),
			/* format arg char*/builder->getInt8Ty()->getPointerTo(),
			/* var args*/true));
	}
	void compile(std::shared_ptr<Program> ast) {
		// 1. create main function
		fn = createFunction("main", llvm::FunctionType::get(/* return type*/ builder->getInt64Ty(),/* varargs*/false), GlobalEnv);
		// 2. compile main body
		//eval(ast,GlobalEnv);
		for (size_t i = 0; i < ast->Statements.size(); i++)
		{
			eval(ast->Statements[i], GlobalEnv);
		}
	}
	//TODO: implement this
	llvm::Value* eval(std::shared_ptr<Node> node, std::shared_ptr<Environment> env) {
		if (dynamic_cast<ExpressionStatement*>(node.get()) != nullptr) {
			auto expr = dynamic_cast<ExpressionStatement*>(node.get());
			return eval(std::move(expr->Expression), env);
		}
		if (dynamic_cast<WhileExpression*>(node.get()) != nullptr)
		{
			auto expr = dynamic_cast<WhileExpression*>(node.get());
			auto conditionBlcok = createBB("condition", fn);
			builder->CreateBr(conditionBlcok);

			auto bodyBlock = createBB("body", fn);
			auto loopendBlock = createBB("end", fn);

			builder->SetInsertPoint(conditionBlcok);
			auto cond = eval(std::move(expr->Condition), env);
			if (cond == nullptr)
			{
				return nullptr;
			}
			builder->CreateCondBr(cond, bodyBlock, loopendBlock);
			fn->insert(fn->end(), bodyBlock);
			builder->SetInsertPoint(bodyBlock);
			eval(std::move(expr->Body), env);
			builder->CreateBr(conditionBlcok);

			fn->insert(fn->end(), loopendBlock);
			builder->SetInsertPoint(loopendBlock);

			return builder->getInt32(0);
		}
		if (dynamic_cast<BlockStatement*>(node.get()) != nullptr)
		{
			auto block = dynamic_cast<BlockStatement*>(node.get());
			auto blockEnv = std::make_shared<Environment>(std::map<std::string, llvm::Value*>{}, env);
			llvm::Value* blockRes = nullptr;
			for (auto i = 0; i < block->Statements.size(); i++)
			{
				auto stmt = std::move(block->Statements[i]);
				if (dynamic_cast<ReturnStatement*>(stmt.get()) != nullptr)
				{
					blockRes = eval(std::move(stmt), blockEnv);
					return blockRes;
				}
				blockRes = eval(std::move(stmt), blockEnv);
			}
			// return the last block result
			return blockRes;

		}
		if (dynamic_cast<IfExpression*>(node.get()) != nullptr)
		{
			auto ifexpr = dynamic_cast<IfExpression*>(node.get());
			auto cond = eval(std::move(ifexpr->Condition), env);

			// consequence block
			auto consequenceBlock = createBB("consequence", fn);
			auto elseBlock = createBB("else", fn);
			auto ifEndBlock = createBB("end", fn);
			builder->CreateCondBr(cond, consequenceBlock, elseBlock);

			builder->SetInsertPoint(consequenceBlock);
			auto conseqResult = eval(std::move(ifexpr->Consequence), env);
			if (conseqResult == nullptr)
			{
				return nullptr;
			}
			builder->CreateBr(ifEndBlock);

			consequenceBlock = builder->GetInsertBlock();
			// else branch

			fn->insert(fn->end(), elseBlock);
			builder->SetInsertPoint(elseBlock);
			auto alternativeResult = eval(std::move(ifexpr->Alternative), env);
			if (alternativeResult == nullptr)
			{
				return nullptr;
			}
			builder->CreateBr(ifEndBlock);
			elseBlock = builder->GetInsertBlock();

			fn->insert(fn->end(), ifEndBlock);

			builder->SetInsertPoint(ifEndBlock);

			auto phi = builder->CreatePHI(cond->getType(), 2, "tmpif");
			phi->addIncoming(conseqResult, consequenceBlock);
			phi->addIncoming(alternativeResult, elseBlock);
			return phi;

		}
		if (dynamic_cast<StringLiteral*>(node.get()) != nullptr)
		{
			auto str = dynamic_cast<StringLiteral*>(node.get());
			return builder->CreateGlobalString(str->Value);
		}
		if (dynamic_cast<ReturnStatement*>(node.get()) != nullptr)
		{
			auto rt = dynamic_cast<ReturnStatement*>(node.get());
			auto val = eval(std::move(rt->ReturnValue), env);
			builder->CreateRet(val);
		}

		if (dynamic_cast<LetStatement*>(node.get()) != nullptr) {
			auto stmt = dynamic_cast<LetStatement*>(node.get());
			auto val = eval(std::move(stmt->Value), env);
			if (val == nullptr)
			{
				return val;
			}
			if (stmt->Token.Type.compare(MUT) == 0)
			{
				auto MutBinding = env->lookup(stmt->Name->Value);
				return builder->CreateStore(val, MutBinding);

			}

			auto letBinding = allocateVariable(stmt->Name->Value, val->getType(), env);
			builder->CreateStore(val, letBinding);
			return val;
		}
		if (dynamic_cast<FunctionLiteral*>(node.get()) != nullptr)
		{
			auto fnLiteral = (dynamic_cast<FunctionLiteral*>(node.get()));
			auto params = std::move(fnLiteral->Parameters);
			auto v = vector<llvm::Type*>(); // parameters types
			auto names = vector<std::string>(); // parameters names
			for (auto& p : params) {
				v.push_back(getTypeFromIdentifier(p->type));
			}
			for (auto& p : params) {
				names.push_back(p->Token.Literal);
			}
			auto body = std::move(fnLiteral->Body);
			llvm::FunctionType* fnType = nullptr;
			if (fnLiteral->Type.Literal == VOID)
			{
				fnType = llvm::FunctionType::get(builder->getVoidTy(), v, true);
			}
			if (fnLiteral->Type.Literal == BOOLEAN)
			{
				fnType = llvm::FunctionType::get(builder->getInt1Ty(), v, true);
			}
			if (fnLiteral->Type.Literal == I8)
			{
				fnType = llvm::FunctionType::get(builder->getInt8Ty(), v, true);
			}
			if (fnLiteral->Type.Literal == I16)
			{
				fnType = llvm::FunctionType::get(builder->getInt16Ty(), v, true);
			}
			if (fnLiteral->Type.Literal == I32)
			{
				fnType = llvm::FunctionType::get(builder->getInt32Ty(), v, true);
			}
			if (fnLiteral->Type.Literal == I64)
			{
				fnType = llvm::FunctionType::get(builder->getInt64Ty(), v, true);
			}
			if (fnLiteral->Type.Literal == FLOAT)
			{
				fnType = llvm::FunctionType::get(builder->getFloatTy(), v, true);
			}
			if (fnLiteral->Type.Literal == DOUBLE)
			{
				fnType = llvm::FunctionType::get(builder->getDoubleTy(), v, true);
			}
			auto prevFn = fn;
			auto prevBlock = builder->GetInsertBlock();

			auto function = createFunction(fnLiteral->ident.Literal, fnType, env);
			auto fnEnv = setFunctionArgs(function, names, env); // function environment
			fn = function;

			// restore the previous fn location
			builder->CreateRet(eval(std::move(body), fnEnv));
			builder->SetInsertPoint(prevBlock);
			fn = prevFn;

			return function;
		}
		if (dynamic_cast<CallExpression*>(node.get()) != nullptr)
		{
			auto fn = dynamic_cast<CallExpression*>(node.get());
			auto function = eval(std::move(fn->Function), env);
			if (function == nullptr)
			{
				return function;
			}
			std::vector<llvm::Value*> args{};

			for (auto& a : fn->Arguments) {
				auto arg = eval(std::move(a), env);
				args.push_back(arg);
			}

			auto func = (llvm::Function*)function;
			return builder->CreateCall(func,args);

		}
		if (dynamic_cast<Identifier*>(node.get()) != nullptr)
		{
			llvm::Value* result = evalIdentifier(node, env);
			return result;
		}
		if (dynamic_cast<IntegerLiteral*>(node.get()) != nullptr) {
			auto number = dynamic_cast<IntegerLiteral*>(node.get());
			return builder->getInt32(number->Value);
		}
		if (dynamic_cast<FloatLiteral*>(node.get()) != nullptr) {
			auto number = dynamic_cast<FloatLiteral*>(node.get());
			return llvm::ConstantFP::get(builder->getDoubleTy(), number->Value);
		}
		if (dynamic_cast<Boolean*>(node.get()) != nullptr)
		{
			auto b = dynamic_cast<Boolean*>(node.get());
			return builder->getInt1(b->Value);
		}
		if (dynamic_cast<PrefixExpression*>(node.get()) != nullptr) {
			auto prefix = dynamic_cast<PrefixExpression*>(node.get());
			auto right = eval(std::move(prefix->Right), env);
			if (right == nullptr)
			{
				return right;
			}
			if (prefix->Operator.compare("!") == 0)
			{
				return builder->CreateNot(right);
			}
			if (prefix->Operator.compare("-") == 0)
			{
				return builder->CreateNeg(right);
			}
			return nullptr;
		}
		if (dynamic_cast<InfixExpression*>(node.get()) != nullptr)
		{
			auto infix = dynamic_cast<InfixExpression*>(node.get());
			auto left = eval(std::move(infix->Left), env);
			if (left == nullptr)
			{
				return left;
			}
			auto right = eval(std::move(infix->Right), env);
			if (right == nullptr)
			{
				return right;
			}
			return evalInfixExpression(infix->Operator, left, right);
		}
		return builder->getInt32(0);
	}


	void saveModuleToFile(const std::string& filename) {
		std::error_code error_code;
		llvm::raw_fd_ostream outLL(filename, error_code);
		module->print(outLL, nullptr);
	}

	void setupGlobalEnvironment() {
		auto record = map<std::string, llvm::Value*>();
		GlobalEnv = std::make_shared<Environment>(record, nullptr);
		GlobalEnv->define("version", createGlobal("version", (llvm::Constant*)builder->getInt32(1)));
	}

	/**
	* creates a function
	*/
	llvm::Function* createFunction(const std::string& fnName, llvm::FunctionType* fnType, std::shared_ptr<Environment> env) {
		// function prototype may already be defined
		auto fn = module->getFunction(fnName);
		// if not, allocate the function
		if (fn == nullptr)
		{
			fn = createFunctionProto(fnName, fnType, env);
		}
		createFunctionBlock(fn);
		return fn;
	}
	/*
	* set the names of the function arguments
	*/
	std::shared_ptr<Environment> setFunctionArgs(llvm::Function* fn, std::vector<std::string> fnArgs, std::shared_ptr<Environment> env) {
		auto fnEnv = std::make_shared<Environment>(std::map<std::string, llvm::Value*>{}, env);


		unsigned Idx = 0;
		for (auto& arg : fn->args()) {
			auto argBinding = allocateVariable(fnArgs[Idx], arg.getType(), fnEnv);
			arg.setName(fnArgs[Idx++]);
			builder->CreateStore(&arg, argBinding);
		}
		return fnEnv;
	}
	/**
	* Creates function prototype (defines the function, but not the body)
	*/
	llvm::Function* createFunctionProto(const std::string& fnName, llvm::FunctionType* fnType, std::shared_ptr<Environment> env) {
		auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);
		verifyFunction(*fn);
		env->define(fnName, fn);
		return fn;
	}
	void createFunctionBlock(llvm::Function* fn) {
		auto entry = createBB("entry", fn);
		builder->SetInsertPoint(entry);
	}
	/**
	* Creates a Basic Block, if fn is passed
	* the block is appended to the parent function, otherwise
	* the block should be later appended manually via
	* fn->getBasicBlockList().push_back(block);
	*/
	llvm::BasicBlock* createBB(const std::string& name, llvm::Function* fn = nullptr) {
		return llvm::BasicBlock::Create(*ctx, name, fn);
	}
	/*
	* Creates a Global Variable
	* Linkage is what determines if multiple declarations
	* of the same object refer to the same
	* object,or to separate ones
	* Linkage Types:
	* ExternalLinkage -> Externally visible function.
	* AvailableExternallyLinkage -> Available for inspection,not emission.
	* LinkOnceAnyLinkage -> Keep one copy of function when linking(inline)
	* LinkOnceODRLinkage -> Same, but only  eplaced by something equivalent.
	* WeakAnyLinkage -> Keep one copy of named function when linking(weak)
	* WeakODRLinkage  -> Same, but only replaced by something equivalent.
	* AppendingLinkage -> Special purpose, only applies to global arrays.
	* InternalLinkage -> Rename collisions when linking (static functions).
	* PrivateLinkage -> Like internal, but omit from symbol table.
	* ExternalWeakLinkage -> ExternalWeak linkage description.
	* CommonLinkage -> Tentative definitions
	*/
	llvm::GlobalVariable* createGlobal(const std::string& name, llvm::Constant* init) {
		module->getOrInsertGlobal(name, init->getType());
		llvm::GlobalVariable* gVar = module->getNamedGlobal(name);
		gVar->setInitializer(init);
		gVar->setConstant(false);
		gVar->setLinkage(llvm::GlobalVariable::CommonLinkage);
		return gVar;
	}

	/*
	* Allocates a variable on the stack
	*/
	llvm::Value* allocateVariable(const std::string& name, llvm::Type* type_, std::shared_ptr<Environment>env) {
		variableBuilder->SetInsertPoint(&fn->getEntryBlock());

		auto allocatedVariable = variableBuilder->CreateAlloca(type_, 0, name.c_str());
		env->define(name, allocatedVariable);
		return allocatedVariable;
	}

	llvm::Type* getTypeFromIdentifier(const std::string& type_) {
		if (type_.compare(BOOLEAN) == 0)
		{
			return builder->getInt1Ty();
		}
		if (type_.compare(I8) == 0)
		{
			return builder->getInt8Ty();
		}
		if (type_.compare(I16) == 0)
		{
			return builder->getInt16Ty();
		}
		if (type_.compare(I32) == 0)
		{
			return builder->getInt32Ty();
		}
		if (type_.compare(I64) == 0)
		{
			return builder->getInt64Ty();
		}
		if (type_.compare(FLOAT) == 0)
		{
			return builder->getFloatTy();
		}
		if (type_.compare(DOUBLE) == 0)
		{
			return builder->getDoubleTy();
		}
		return nullptr;
	}


	llvm::Value* evalProgram(shared_ptr<Node> node, std::shared_ptr<Environment> env) {
		llvm::Value* result = nullptr;
		auto program = dynamic_cast<Program*>(node.get());
		for (size_t i = 0; i < program->Statements.size(); i++)
		{
			result = eval(program->Statements[i], env);
		}
		return result;
	}
	llvm::Value* evalIdentifier(shared_ptr<Node> node, std::shared_ptr<Environment> env) {
		auto ident = dynamic_cast<Identifier*>(node.get());
		auto value = env->lookup(ident->Value);

		// local variable
		if (auto localValue = dyn_cast<llvm::AllocaInst>(value))
		{
			return builder->CreateLoad(localValue->getAllocatedType(), localValue, ident->Value.c_str());
		}

		// global variable
		if (auto globalValue = dyn_cast<llvm::GlobalVariable>(value))
		{
			return builder->CreateLoad(globalValue->getInitializer()->getType(), globalValue, ident->Value.c_str());
		}
		return value;
	}

	llvm::Value* evalInfixExpression(const std::string& op, llvm::Value* left, llvm::Value* right) {
		if (left->getType()->isIntegerTy() == right->getType()->isIntegerTy())
		{

			if (op.compare("+") == 0)
			{
				return builder->CreateAdd(left, right);
			}
			if (op.compare("-") == 0)
			{
				return builder->CreateSub(left, right);
			}
			if (op.compare("*") == 0)
			{
				return builder->CreateMul(left, right);
			}
			if (op.compare("/") == 0)
			{
				return builder->CreateSDiv(left, right);
			}
			if (op.compare("%") == 0)
			{
				return builder->CreateSRem(left, right);
			}
			if (op.compare("<<") == 0)
			{
				return builder->CreateShl(left, right);
			}
			if (op.compare(">>") == 0)
			{
				return builder->CreateLShr(left, right);
			}
			if (op.compare("<") == 0)
			{
				return builder->CreateICmpSLT(left, right);
			}
			if (op.compare(">") == 0)
			{
				return builder->CreateICmpSGT(left, right);
			}
			if (op.compare("==") == 0)
			{
				return builder->CreateICmpEQ(left, right);
			}
			if (op.compare("!=") == 0)
			{
				return builder->CreateICmpNE(left, right);
			}
			if (op.compare(">=") == 0)
			{
				return builder->CreateICmpSGE(left, right);
			}
			if (op.compare("<=") == 0)
			{
				return builder->CreateICmpSLE(left, right);
			}
		}
		// float operations
		if (left->getType()->isFloatingPointTy() == right->getType()->isFloatingPointTy())
		{
			if (op.compare("+") == 0)
			{
				return builder->CreateFAdd(left, right);
			}
			if (op.compare("-") == 0)
			{
				return builder->CreateFSub(left, right);
			}
			if (op.compare("*") == 0)
			{
				return builder->CreateFMul(left, right);
			}
			if (op.compare("/") == 0)
			{
				return builder->CreateFDiv(left, right);
			}
			if (op.compare("<") == 0)
			{
				return builder->CreateFCmpOLT(left, right);
			}
			if (op.compare(">") == 0)
			{
				return builder->CreateFCmpOGT(left, right);
			}
			if (op.compare("==") == 0)
			{
				return builder->CreateFCmpOEQ(left, right);
			}
			if (op.compare("!=") == 0)
			{
				return builder->CreateFCmpONE(left, right);
			}
			if (op.compare(">=") == 0)
			{
				return builder->CreateFCmpOGE(left, right);
			}
			if (op.compare("<=") == 0)
			{
				return builder->CreateFCmpOLE(left, right);
			}
		}


		// string concatination
		if (left->getType()->isPointerTy())
		{
			// not implemented
		}

		if (op.compare("or") == 0)
		{
			return builder->CreateOr(left, right);
		}
		if (op.compare("and") == 0)
		{
			return builder->CreateAnd(left, right);
		}
		if (op.compare("==") == 0)
		{
			return builder->getInt1(left == right);
		}
		if (op.compare("!=") == 0)
		{
			return builder->getInt1(left != right);
		}
		return nullptr;
	}


	/*llvm::Value* evalExpression(shared_ptr<Node> node, std::shared_ptr<Environment> env) {

	}*/

	/*
	* The pratt parser
	*/
	std::unique_ptr<Parser>parser;
	/*
	* Global LLVM Context
	* It owns and managaes the core "global" data of llvm's core
	* infrastructure, including the type and constant unique tables
	*/
	std::unique_ptr<llvm::LLVMContext> ctx;
	/**
	 * A Module instance is used to store all the information related to an
	 * LLVM module. Modules are the top level container of all other LLVM
	 * Intermediate Representation (IR) objects. Each module directly contains a
	 * list of globals variables, a list of functions, a list of libraries (or
	 * other modules) this module depends on, a symbol table, and various data
	 * about the target's characteristics.
	 *
	 * A module maintains a GlobalList object that is used to hold all
	 * constant references to global variables in the module.  When a global
	 * variable is destroyed, it should have no entries in the GlobalList.
	 * The main container class for the LLVM Intermediate Representation.
	 */
	std::unique_ptr<llvm::Module> module;

	/**
	 * Extra builder for variables declaration.
	 * This builder always prepends to the beginning of the
	 * function entry block.
	 */
	std::unique_ptr<llvm::IRBuilder<>> variableBuilder;


	/**
	* IR Builder.
	*
	* This provides a uniform API for creating instructions and inserting
	* them into a basic block: either at the end of a BasicBlock, or at a
	* specific iterator location in a block.
	*/
	std::unique_ptr<llvm::IRBuilder<>> builder;
	llvm::Function* fn;

	/**
	* Global Environment (symbol table).
	*/
	std::shared_ptr<Environment> GlobalEnv;
};