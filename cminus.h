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
	}
	void setupExternalFunctions() {
		module->getOrInsertFunction("printf", llvm::FunctionType::get(
			/* return type*/builder->getInt32Ty(),
			/* format arg char*/builder->getInt8Ty()->getPointerTo(),
			/* var args*/true));
	}
	void compile(std::shared_ptr<Program> ast) {
		// 1. create main function
		fn = createFunction("main", llvm::FunctionType::get(/* return type*/ builder->getInt64Ty(),/* varargs*/false));
		// 2. compile main body
		//eval(ast,GlobalEnv);
		for (size_t i = 0; i < ast->Statements.size(); i++)
		{
			eval(ast->Statements[i], GlobalEnv);
		}
	}
	//TODO: implement this
	llvm::Value* eval(std::shared_ptr<Node> node, std::shared_ptr<Environment> env) {
		if (dynamic_cast<ExpressionStatement*>(node.get())!=nullptr) {
			auto expr = dynamic_cast<ExpressionStatement*>(node.get());
			return eval(std::move(expr->Expression), env);
		}
		// implement function body 
		if (dynamic_cast<FunctionLiteral*>(node.get())!=nullptr)
		{
			auto fnLiteral = (dynamic_cast<FunctionLiteral*>(node.get()));
			auto params = std::move(fnLiteral->Parameters);
			auto v = vector<std::string>();
			for (auto& p : params)
			{
				v.push_back(p->String());
			}
			auto body = std::move(fnLiteral->Body);
			llvm::FunctionType* fnType = nullptr;
			if (fnLiteral->Type.Literal == VOID)
			{
				fnType = llvm::FunctionType::get(builder->getVoidTy(), true);
			}
			if (fnLiteral->Type.Literal == BOOLEAN)
			{
				fnType = llvm::FunctionType::get(builder->getInt1Ty(), true);
			}
			if (fnLiteral->Type.Literal == I8)
			{
				fnType = llvm::FunctionType::get(builder->getInt8Ty(), true);
			}
			if (fnLiteral->Type.Literal == I16)
			{
				fnType = llvm::FunctionType::get(builder->getInt16Ty(), true);
			}
			if (fnLiteral->Type.Literal == I32)
			{
				fnType = llvm::FunctionType::get(builder->getInt32Ty(), true);
			}
			if (fnLiteral->Type.Literal == I64)
			{
				fnType = llvm::FunctionType::get(builder->getInt64Ty(), true);
			}
			if (fnLiteral->Type.Literal == FLOAT)
			{
				fnType = llvm::FunctionType::get(builder->getFloatTy(), true);
			}
			if (fnLiteral->Type.Literal == DOUBLE)
			{
				fnType = llvm::FunctionType::get(builder->getDoubleTy(), true);
			}
			auto function = createFunction(fnLiteral->ident.Literal, fnType);
			setFunctionArgs(function, v);
			GlobalEnv->define(fnLiteral->ident.Literal, function);
			return function;
		}
		// implement this
		if (dynamic_cast<CallExpression*>(node.get())!=nullptr)
		{
			auto fn = dynamic_cast<CallExpression*>(node.get());
			auto function = eval(std::move(fn->Function), env);
			if (function == nullptr)
			{
				return function;
			}

		}
		if (dynamic_cast<Identifier*>(node.get()) != nullptr)
		{
			llvm::Value* result = evalIdentifier(node, env);
			return builder->CreateRet(result);
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
		GlobalEnv->define("version", createGlobal("version", builder->getInt8Ty()->getPointerTo()));
	}

	/**
    * creates a function
    */
	llvm::Function* createFunction(const std::string& fnName, llvm::FunctionType* fnType) {
		// function prototype may already be defined
		auto fn = module->getFunction(fnName);
		// if not, allocate the function
		if (fn == nullptr)
		{
			fn = createFunctionProto(fnName, fnType);
		}
		createFunctionBlock(fn);
		return fn;
	}
	/*
	* set the names of the function arguments
	*/
	void setFunctionArgs(llvm::Function* fn, std::vector<std::string> fnArgs) {
		unsigned Idx = 0;
		llvm::Function::arg_iterator AI, AE;
		for (AI = fn->arg_begin(), AE = fn->arg_end(); AI != AE; ++AI, Idx++)
		{
			AI->setName(fnArgs[Idx]);
		}
	}
	/**
	* Creates function prototype (defines the function, but not the body)
	*/
	llvm::Function* createFunctionProto(const std::string& fnName, llvm::FunctionType* fnType) {
		auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);
		verifyFunction(*fn);
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
	llvm::GlobalVariable* createGlobal(const std::string& name,llvm::Type* type) {
		module->getOrInsertGlobal(name, type);
		llvm::GlobalVariable* gVar = module->getNamedGlobal(name);
		gVar->setLinkage(llvm::GlobalVariable::CommonLinkage);
		return gVar;
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
		return env->lookup(ident->Value);
	}


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
	std::unique_ptr<llvm::IRBuilder<>> builder;
	llvm::Function* fn;
	
	/**
    * Global Environment (symbol table).
    */
	std::shared_ptr<Environment> GlobalEnv;
};