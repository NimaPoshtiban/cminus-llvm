#pragma once
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>
#define PARSER_H
#include "ast.h"
#include "lexer.h"
#include <format>
#include <functional>
typedef  function<std::unique_ptr<Expression>()> prefixParseFn;
typedef  function<std::unique_ptr<Expression>(std::unique_ptr<Expression>)> infixParseFn;

enum class Precedence {
	LOWEST = 0,
	EQUALS = 1,
	LESSGREATER = 2,
	SUM = 3,
	PRODUCT = 4,
	PREFIX = 5,
	CALL = 6,
	INDEX = 7,
};

unordered_map<TokenType, Precedence> precedences = {
	{EQ, Precedence::EQUALS},          {NOT_EQ, Precedence::EQUALS},
	{LOGICAL_AND, Precedence::EQUALS}, {LOGICAL_OR, Precedence::EQUALS},
	{LT, Precedence::LESSGREATER},     {GT, Precedence::LESSGREATER},
	{GT_EQ, Precedence::LESSGREATER},  {LT_EQ, Precedence::LESSGREATER},
	{PLUS, Precedence::SUM},           {MINUS, Precedence::SUM},
	{SLASH, Precedence::PRODUCT},      {ASTERISK, Precedence::PRODUCT},
	{MODULO, Precedence::PRODUCT},     {RSHIFT, Precedence::PRODUCT},
	{LSHIFT, Precedence::PRODUCT},     {LPAREN, Precedence::CALL},
	{LBRACKET, Precedence::INDEX} };

class Parser {
public:
	Parser() {
		lexer = make_unique<Lexer>();
		errors = vector<string>();
		nextToken();
		nextToken();
		infixParseFns = unordered_map<TokenType, function<std::unique_ptr<Expression>(std::unique_ptr<Expression>)>>();
		//TODO: register prefix and infix expressions


	}
	std::unique_ptr<Program> ParserProgram() {
		auto program = std::make_unique<Program>();
		while (curToken.Type.compare(EOF_TOKEN) != 0) {
			auto statement = parseStatement();
			if (statement != nullptr) {
				program->Statements.push_back(statement);
			}
			nextToken();
		}
		return program;
	}

private:
	void nextToken(void) {
		curToken = peekToken;
		peekToken = lexer->NextToken();
	}
	std::unique_ptr<Statement> parseStatement() {
		if (curToken.Type.compare(LET) == 0) {
			return parseLetStatement();
		}
		else if (curToken.Type.compare(RETURN) == 0) {
			return parseReturnStatment();
		}
		return parseExpressionStatement();
	}
	std::unique_ptr<Identifier> parseIdentifier() {
		auto ident = std::make_unique<Identifier>(curToken, curToken.Literal);
		return ident;
	}

	std::unique_ptr<LetStatement>parseLetStatement() {
		auto statement = std::make_unique<LetStatement>(curToken);
		if (!expectPeek(IDENT)) {
			return nullptr;
		}
		statement->Name = make_unique<Identifier>(curToken, curToken.Literal);
		if (!expectPeek(ASSIGN)) {
			return nullptr;
		}
		nextToken();
		statement->Value = parseExpression(Precedence::LOWEST);
		if (peekTokenIs(SEMICOLON)) {
			nextToken();
		}
		return statement;
	}
	std::unique_ptr<ReturnStatement> parseReturnStatment() {
		auto stmt = std::make_unique<ReturnStatement>(curToken.Type);
		nextToken();
		stmt->ReturnValue = parseExpression(Precedence::LOWEST);
		if (peekTokenIs(SEMICOLON)) {
			nextToken();
		}
		return stmt;
	}
	std::unique_ptr<ExpressionStatement> parseExpressionStatement() {
		auto stmt = std::make_unique<ExpressionStatement>(curToken);

		stmt->Expression = parseExpression(Precedence::LOWEST);

		if (peekTokenIs(SEMICOLON)) {
			nextToken();
		}

		return stmt;
	}
	std::unique_ptr<Expression>parseExpression(Precedence p) {
		auto& prefix = prefixParseFns.at(curToken.Type);
		if (prefix == nullptr) {
			noPrefixParseFnError(curToken.Type);
			return nullptr;
		}
		auto leftExp = prefix();
		while (peekTokenIs(SEMICOLON) && p < peekPrecedence()) {
			auto infix = infixParseFns.at(peekToken.Type);
			if (infix == nullptr) {
				return leftExp;
			}
			nextToken();
			leftExp = infix(std::move(leftExp));
		}
	}
	std::unique_ptr<IntegerLiteral> parseIntegerLiteral() {
		auto lit = std::make_unique<IntegerLiteral>(curToken);
		try {
			auto value = std::stoi(curToken.Literal);
			lit->Value = value;
		}
		catch (std::invalid_argument const& ex) {
			auto msg =
				std::format("at line {} could not parse {} as integer",
					lexer->GetCurrentLine(), curToken.Literal);
			return nullptr;
		}
		return lit;
	}
	std::unique_ptr<Expression> parseBoolean() {
		return std::make_unique<Boolean>(curToken, curTokenIs(TRUE));
	}

	std::unique_ptr<Expression>parsePrefixExpression() {
		auto expr = std::make_unique<PrefixExpression>(curToken, curToken.Literal);
		nextToken();
		expr->Right = parseExpression(Precedence::LOWEST);
		return expr;
	}

	std::unique_ptr<Expression>parseInfixExpression(std::unique_ptr<Expression> left) {
		auto expr = std::make_unique<InfixExpression>(curToken, curToken.Literal, left);
		auto precedence = curPrecedence();
		nextToken();
		expr->Right = parseExpression(precedence);
		return expr;
	}
	std::unique_ptr<Expression> parseGroupedExpression() {
		nextToken();
		auto exp = parseExpression(Precedence::LOWEST);
		if (!expectPeek(LPAREN))
		{
			return nullptr;
		}
		return exp;
	}
	std::unique_ptr<Expression>parseIfExpression() {
		auto expr = std::make_unique<IfExpression>(curToken);
		if (!expectPeek(LPAREN))
		{
			return nullptr;
		}
		nextToken();
		expr->Condition = parseExpression(Precedence::LOWEST);
		if (!expectPeek(RPAREN))
		{
			return nullptr;
		}
		if (!expectPeek(LBRACE)) {
			return nullptr;
		}
		expr->Consequence = parseBlockStatement();
		if (peekTokenIs(ELSE))
		{
			nextToken();
			if (!expectPeek(LBRACE)) {
				return nullptr;
			}
			expr->Alternative = parseBlockStatement();
		}
		return expr;
	}

	std::unique_ptr<Expression> parseFunctionLiteral() {
		auto lit = std::make_unique<FunctionLiteral>(curToken);
		if (!expectPeek(LPAREN))
		{
			return nullptr;
		}
		lit->Parameters = parseFunctionParameters();
		if (!expectPeek(LBRACE)) {
			return nullptr;
		}
		lit->Body = parseBlockStatement();
		return lit;

	}
	std::vector<std::unique_ptr<Identifier>>parseFunctionParameters() {
		auto identifiers = vector<std::unique_ptr<Identifier>>();
		if (peekTokenIs(RPAREN)) {
			nextToken();
			return identifiers;
		}
		nextToken();
		auto ident = std::make_unique<Identifier>(curToken, curToken.Literal);
		identifiers.push_back(ident);
		while (peekTokenIs(COMMA))
		{
			nextToken();
			nextToken();
			auto ident = std::make_unique<Identifier>(curToken, curToken.Literal);
			identifiers.push_back(ident);
		}
		if (!expectPeek(RPAREN))
		{
			identifiers.clear();
			return identifiers;
		}
		return identifiers;
	}
	std::unique_ptr<Expression> parseCallExpression(Expression* function) {
		auto expr = std::make_unique<CallExpression>(curToken, function);
		expr->Arguments = parseExpressionList(RPAREN);
		return expr;
	}
	vector<std::unique_ptr<Expression>>parseCallArguments() {
		auto args = vector<std::unique_ptr<Expression>>();
		if (peekTokenIs(LPAREN)) {
			nextToken();
			return args;
		}

		nextToken();
		args.push_back(parseExpression(Precedence::LOWEST));

		while (peekTokenIs(COMMA)) {
			nextToken();
			nextToken();
			args.push_back(parseExpression(Precedence::LOWEST));
		}

		if (!expectPeek(RPAREN)) {
			args.clear();
			return args;
		}

		return args;
	}

	vector<std::unique_ptr<Expression>> parseExpressionList(TokenType end) {
		auto list = vector<std::unique_ptr<Expression>>();
		if (peekTokenIs(end))
		{
			nextToken();
			return list;
		}
		nextToken();
		list.push_back(parseExpression(Precedence::LOWEST));
		while (peekTokenIs(COMMA))
		{
			nextToken();
			nextToken();
			list.push_back(parseExpression(Precedence::LOWEST));
		}
		if (!expectPeek(end))
		{
			list.clear();
			return list;
		}
		return list;
	}
	std::unique_ptr<Expression>parseWhileLoop() {
		auto expr = make_unique<WhileExpression>(curToken);
		if (!expectPeek(LPAREN))
		{
			return nullptr;
		}
		nextToken();
		expr->Condition = parseExpression(Precedence::LOWEST);
		if (!expectPeek(RPAREN))
		{
			return nullptr;
		}
		if (!expectPeek(LBRACE))
		{
			return nullptr;
		}
		expr->Body = parseBlockStatement();
		return expr;
	}
	std::unique_ptr<BlockStatement> parseBlockStatement() {
		auto block = std::make_unique<BlockStatement>(curToken);
		block->Statements = vector<unique_ptr<Statement>>();
		while (!curTokenIs(RBRACE) && !curTokenIs(EOF_TOKEN))
		{
			auto stmt = parseStatement();
			if (stmt != nullptr)
			{
				block->Statements.push_back(stmt);
			}
			nextToken();
		}
		return block;
	}
	std::unique_ptr<Expression>parseStringLiteral() {
		return std::make_unique<StringLiteral>(curToken, curToken.Literal);
	}
	std::unique_ptr<Expression>parseArrayLiteral() {
		auto array = std::make_unique<ArrayLiteral>(curToken);
		array->Elements = parseExpressionList(RBRACKET);
		return array;
	}
	std::unique_ptr<Expression>parseIndexExpression(Expression* left) {
		auto expr = std::make_unique<IndexExpression>(curToken, left);
		nextToken();
		expr->Index = parseExpression(Precedence::LOWEST);
		if (expectPeek(RBRACKET)) {
			return nullptr;
		}
		return expr;
	}
	std::unique_ptr<Expression>parseHashLiteral() {
		auto hash = std::make_unique<HashLiteral>(curToken);
		hash->Pairs = map<std::unique_ptr<Expression>, std::unique_ptr<Expression>>();
		while(peekTokenIs(RBRACE) ){
		nextToken();
		auto key = parseExpression(Precedence::LOWEST);
				if(expectPeek(COLON)) {
					return nullptr;
				}
				nextToken();
				auto value = parseExpression(Precedence::LOWEST);
				hash->Pairs[key] = std::move(value);
				if (peekTokenIs(RBRACE) && !expectPeek(COMMA)) {
					return nullptr;
				}
		}
		if (!expectPeek(RBRACE)) {
			return nullptr;
		}
		return hash;
	}

	Precedence peekPrecedence() {
		unordered_map<TokenType, Precedence>::const_iterator found = precedences.find(peekToken.Type);
		if (found == precedences.end())
		{
			return Precedence::LOWEST;
		}
		return found->second;
	}
	Precedence curPrecedence() {
		unordered_map<TokenType, Precedence>::const_iterator found = precedences.find(curToken.Type);
		if (found == precedences.end())
		{
			return Precedence::LOWEST;
		}
		return found->second;
	}
	void registerPrefix(TokenType t, prefixParseFn fn) {
		prefixParseFns[t] = fn;
	}
	void registerInfix(TokenType t, infixParseFn fn) {
		infixParseFns[t] = fn;
	}
	void peekError(TokenType t) {
		auto msg =
			std::format("at line {} expected next token to be {}, got {} instead",
				lexer->GetCurrentLine(), t, peekToken.Type);
	}
	void noPrefixParseFnError(const TokenType& t) {
		auto msg =
			std::format("at line {} no prefix function found for {}",
				lexer->GetCurrentLine(), t);
		errors.push_back(msg);
	}
	bool curTokenIs(const TokenType& t) { return !(curToken.Type.compare(t)); }
	bool peekTokenIs(const TokenType& t) { return !(peekToken.Type.compare(t)); }
	bool expectPeek(const TokenType& t) {
		if (peekTokenIs(t)) {
			nextToken();
			return true;
		}
		else {
			peekError(t);
			return false;
		}
	}
	unique_ptr<Lexer> lexer;
	Token curToken;
	Token peekToken;
	vector<string> errors;
	unordered_map<TokenType, function<std::unique_ptr<Expression>()>> prefixParseFns;
	unordered_map<TokenType, function<std::unique_ptr<Expression>(std::unique_ptr<Expression>)>> infixParseFns;
};

// unordered_map<TokenType, function<Expression *()>> prefixParseFns = {
//{IDENT, [&]() { return parseIdentifier(); }},
//{INT, [&]() { return parseIntegerLiteral(); }},
//{BANG, [&]() { return parsePrefixExpression(); }},
//{MINUS, [&]() { return parsePrefixExpression(); }},
//{TRUE, [&]() { return parseBooleanLiteral(); }},
//{FALSE, [&]() { return parseBooleanLiteral(); }},
//{IF, [&]() { return parseIfExpression(); }},
//{FUNCTION, [&]() { return parseFunctionLiteral(); }},
//{STRING, [&]() { return parseStringLiteral(); }},
//{LBRACKET, [&]() { return parseArrayLiteral(); }},
//{LBRACE, [&]() { return parseHashLiteral(); }},
//};
// unordered_map<TokenType, function<Expression *(Expression *)>> infixParseFns
// = { {PLUS, [&](Expression *left) { return parseInfixExpression(left); }},
//{MINUS, [&](Expression *left) { return parseInfixExpression(left); }},
//{ASTERISK, [&](Expression *left) { return parseInfixExpression(left); }},
//{SLASH, [&](Expression *left) { return parseInfixExpression(left); }},
//{LT, [&](Expression *left) { return parseInfixExpression(left); }},
//{GT, [&](Expression *left) { return parseInfixExpression(left); }},
//{EQ, [&](Expression *left) { return parseInfixExpression(left); }},
//{NOT_EQ, [&](Expression *left) { return parseInfixExpression(left); }},
//{LOGICAL_AND, [&](Expression *left) { return parseInfixExpression(left); }},
//{LOGICAL_OR, [&](Expression *left) { return parseInfixExpression(left); }},
//{LPAREN, [&](Expression *left) { return parseCallExpression(left); }},
//{LBRACKET, [&](Expression *left) { return parseIndexExpression(left); }}};
