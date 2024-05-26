#pragma once
#define CMINUS_H
#include "parser.h"
#include "ast.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
class Cminus {
public:
	Cminus(const std::string& input) :parser(std::make_unique<Parser>(input)) {
		moduleInit();
		setupExternalFunctions();
	}
	void exec() {
		auto ast = parser->ParserProgram();
		compile(std::move(ast));
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
	void compile(std::unique_ptr<Program> ast) {
		// 1. create main function
		fn = createFunction("main", llvm::FunctionType::get(/* return type*/ builder->getInt64Ty(),/* varargs*/false));
		// 2. compile main body
		gen(std::move(ast));

		builder->CreateRet(builder->getInt64(0));
	}
	//TODO: implement this
	llvm::Value* gen(std::unique_ptr<Node> exp) {
		if (auto program = dynamic_cast<Program*>(exp.get()))
		{
			llvm::Value* result=nullptr;
			for (size_t i = 0; i < program->Statements.size(); i++)
			{
				result = gen(std::move(program->Statements[i]));
			}
			return result;
		}
		else if (auto f = dynamic_cast<FunctionLiteral*>(exp.get()))
		{
			llvm::Value* result = nullptr;
			if (f->Token.Literal=="printf")
			{
				auto printfFn = module->getFunction("printf");
				std::vector<llvm::Value*> args{};
				for (size_t i = 0; i < f->Parameters.size(); i++)
				{
					args.push_back((llvm::Value*)(f->Parameters[i].get()->Value.at(i)));
				}
				return builder->CreateCall(printfFn, args);
			}
		}
		else if (auto let = dynamic_cast<LetStatement*>(exp.get())) {

		}
		else {
			return builder->getInt32(0);
		}
	}

	void saveModuleToFile(const std::string& filename) {
		std::error_code error_code;
		llvm::raw_fd_ostream outLL(filename, error_code);
		module->print(outLL, nullptr);
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
};