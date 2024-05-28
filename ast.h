#pragma once
#include <cstdint>
#include <map>
#include <memory>
#define AST_H
#include "lexer.h"

struct Node {
	virtual string TokenLiteral() = 0;
	virtual string String() = 0;
};

struct Statement : Node {
	virtual void statementNode() = 0;
};

struct Expression : Node {
	virtual void expressionNode() = 0;
};

struct Program : Node {
	Program() { Statements = vector<shared_ptr<Statement>>(); }
	vector<std::shared_ptr<Statement>> Statements;

	string TokenLiteral() {
		if (Statements.size() > 0) {
			return Statements[0]->TokenLiteral();
		}
		else {
			return "";
		}
	}

	string String() {
		string out = "";

		for (auto& s : Statements) {
			out += s->String();
		}

		return out;
	}
};

struct Identifier : Expression {
	Identifier(Token token, string value) : Token(token), Value(value) {}
	Token Token; // the token.IDENT token
	string Value;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() { return Value; }
};

struct LetStatement : Statement {
	LetStatement(Token token) : Token(token) {}
	Token Token; // the token.LET token
	std::unique_ptr<Identifier> Name;
	std::unique_ptr<Expression>Value;

	void statementNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";

		out += TokenLiteral() + " ";
		out += Name->String();
		out += " = ";

		if (Value != nullptr) {
			out += Value->String();
		}
		out += ";";

		return out;
	}
};

struct ReturnStatement : Statement {
	ReturnStatement(Token token, std::unique_ptr<Expression>returnValue = nullptr) : Token(token) {
		ReturnValue = std::move(returnValue);
	}
	Token Token; // the 'return' token
	std::unique_ptr<Expression>ReturnValue;
	void statementNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";

		out += TokenLiteral() + " ";

		if (ReturnValue != nullptr) {
			out += ReturnValue->String();
		}

		out += ";";

		return out;
	}
};

struct ExpressionStatement : Statement {
	ExpressionStatement(Token token) : Token(token), Expression(nullptr) {}
	Token Token; // the first token of the expression
	std::unique_ptr<Expression> Expression;

	void statementNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		if (Expression != nullptr) {
			return Expression->String();
		}
		return "";
	}
};

struct IntegerLiteral : Expression {
	IntegerLiteral(Token token) : Token(token) {}
	IntegerLiteral(Token token, int64_t value) : Token(token), Value(value) {}
	Token Token;
	int64_t Value;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() { return Token.Literal; }
};

struct PrefixExpression : Expression {
	PrefixExpression(Token token, string operator_)
		: Token(token), Operator(operator_) {}
	Token Token;
	string Operator;
	std::unique_ptr<Expression> Right;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";

		out += "(";
		out += Operator;
		out += Right->String();
		out += ")";

		return out;
	}
};

struct InfixExpression : Expression {
	InfixExpression(Token token, string operator_, std::unique_ptr<Expression>left)
		: Token(token), Left(std::move(left)) ,Operator(operator_){}
	Token Token; // the operator such as + , * ,....
	std::unique_ptr<Expression> Left;
	string Operator;
	std::unique_ptr<Expression>Right;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";

		out += "(";
		out += Left->String();
		out += " " + Operator + " ";
		out += Right->String();
		out += ")";

		return out;
	}
};

struct IndexExpression : Expression {
	IndexExpression(Token token, std::unique_ptr<Expression>left = nullptr) : Token(token), Left(std::move(left)) {}
	Token Token; // The [ token
	std::unique_ptr<Expression>Left;
	std::unique_ptr<Expression>Index;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";
		out += "(";
		out += Left->String();
		out += "[";
		out += Index->String();
		out += "])";
		return out;
	}
};

struct Boolean : Expression {
	Boolean(Token token, bool value) : Token(token), Value(value) {}
	Token Token;
	bool Value;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() { return Token.Literal; }
};

struct BlockStatement : Statement {
	BlockStatement(Token token) : Token(token) {}
	Token Token; // the '{' token
	vector<std::unique_ptr<Statement>> Statements;

	void statementNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";
		for (auto& s : Statements) {
			out += s->String();
		}
		return out;
	}
};

struct IfExpression : Expression {
	IfExpression(Token token) : Token(token) {}
	Token Token;
	std::unique_ptr< Expression >Condition;
	std::unique_ptr<BlockStatement> Consequence;
	std::unique_ptr<BlockStatement> Alternative;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";

		out += "if";
		out += Condition->String();
		out += " ";
		out += Consequence->String();

		if (Alternative != nullptr) {
			out += "else ";
			out += Alternative->String();
		}

		return out;
	}
};

struct WhileExpression : Expression {
	WhileExpression(Token token) : Token(token) {}
	Token Token; // the while token
	std::unique_ptr<Expression> Condition;
	std::unique_ptr<BlockStatement> Body;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";
		out += TokenLiteral();
		out += "(";
		out += Condition->String();
		out += ") ";
		out += Body->String();
		return out;
	}
};
// i32 ident() {
//   // something
// }
struct FunctionLiteral : Statement {
	FunctionLiteral(Token token) : Type(token) {}
	Token Type; // the 'type' token function type
	Token ident; // function name;
	vector<std::unique_ptr<Identifier>> Parameters;
	std::unique_ptr<BlockStatement> Body;

	void statementNode() {};

	string TokenLiteral() { return Type.Literal; }

	string String() {
		string out = "";

		vector<string> params;
		for (auto& p : Parameters) {
			params.push_back(p->String());
		}

		out += TokenLiteral();
		out += "(";
		for (auto& p : params) {
			out += p;
			out += ", ";
		}
		out += ") ";
		out += "-> " + ident.Literal;
		out += Body->String();
		return out;
	}
};

struct CallExpression : Expression {
	CallExpression(Token token, std::unique_ptr<Expression> function = nullptr)
		: Token(token) {

		Function = std::move(function);
	}
	Token Token;          // The '(' token
	std::unique_ptr<Expression> Function; // Identifier or FunctionLiteral
	vector<std::unique_ptr<Expression>> Arguments;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";
		vector<string> args;
		for (auto& a : Arguments) {
			args.push_back(a->String());
		}
		out += Function->String();
		out += "(";
		for (auto& arg : args) {
			out += arg;
			out += ", ";
		}
		out += ")";
		return out;
	}
};

struct StringLiteral : Expression {
	StringLiteral(Token token, string value) : Token(token), Value(value) {}
	Token Token;
	string Value;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() { return Token.Literal; }
};

struct ArrayLiteral : Expression {
	ArrayLiteral(Token token) : Token(token) {}
	Token Token; // the '[' token
	vector<std::unique_ptr<Expression>> Elements;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";
		vector<string> elements;
		for (auto& el : Elements) {
			elements.push_back(el->String());
		}
		out += "[";
		for (auto& el : elements) {
			out += el;
			out += ", ";
		}
		out += "]";
		return out;
	}
};

struct HashLiteral : Expression {
	HashLiteral(Token token) : Token(token) {}
	Token Token; //? the '{' token
	map<std::unique_ptr<Expression>, std::unique_ptr<Expression>> Pairs;

	void expressionNode() {}

	string TokenLiteral() { return Token.Literal; }

	string String() {
		string out = "";
		vector<string> pairs;
		for (auto& pair : Pairs) {
			pairs.push_back(pair.first->String() + ":" + pair.second->String());
		}

		out += "{";
		for (auto& p : pairs) {
			out += p;
			out += ", ";
		}
		out += "}";
		return out;
	}
};
