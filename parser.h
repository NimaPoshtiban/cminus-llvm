#pragma once
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#define PARSER_H
#include "ast.h"
#include "lexer.h"
#include <format>
#include <functional>
using prefixParseFn = std::function<std::unique_ptr<Expression>()>;
using infixParseFn =
std::function<std::unique_ptr<Expression>(std::unique_ptr<Expression>)>;

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
	Parser(const std::string& input) {
		lexer = make_unique<Lexer>(input);
		errors = vector<string>();

		infixParseFns = std::map<TokenType, infixParseFn>();
		registerInfix(PLUS, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(MINUS, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(MODULO, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(ASTERISK, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(RSHIFT, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(LSHIFT, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(EQ, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(NOT_EQ, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(LOGICAL_AND, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(LOGICAL_OR, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(LT, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(LT_EQ, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(GT, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(GT_EQ, std::bind(&Parser::parseInfixExpression, this,
			std::placeholders::_1));
		registerInfix(LPAREN, std::bind(&Parser::parseCallExpression, this,
			std::placeholders::_1));
		registerInfix(LBRACKET, std::bind(&Parser::parseIndexExpression, this,
			std::placeholders::_1));

		prefixParseFns = std::map<TokenType, prefixParseFn>();
		registerPrefix(LPAREN, std::bind(&Parser::parseGroupedExpression, this));
		registerPrefix(IDENT, std::bind(&Parser::parseIdentifier, this));
		registerPrefix(INT, std::bind(&Parser::parseIntegerLiteral, this));
		registerPrefix(BANG, std::bind(&Parser::parsePrefixExpression, this));
		registerPrefix(MINUS, std::bind(&Parser::parsePrefixExpression, this));
		registerPrefix(TRUE, std::bind(&Parser::parseBoolean, this));
		registerPrefix(FALSE, std::bind(&Parser::parseBoolean, this));
		registerPrefix(IF, std::bind(&Parser::parseIfExpression, this));
		registerPrefix(WHILE, std::bind(&Parser::parseWhileLoop, this));
		registerPrefix(STRING, std::bind(&Parser::parseStringLiteral, this));
		registerPrefix(LBRACKET, std::bind(&Parser::parseArrayLiteral, this));
		registerPrefix(LBRACE, std::bind(&Parser::parseHashLiteral, this));

		nextToken();
		nextToken();

	}
	std::shared_ptr<Program> ParserProgram() {
		auto program = std::make_shared<Program>();
		while (curToken.Type.compare(EOF_TOKEN) != 0) {
			auto statement = parseStatement();
			if (statement != nullptr) {
				program->Statements.push_back(std::move(statement));
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
		if (LookupType(curToken.Type).compare(IDENT)!=0)
		{
			return parseFunctionLiteral();
		}
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
		return std::move(ident);
	}

	std::unique_ptr<LetStatement> parseLetStatement() {
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
		return std::move(statement);
	}
	std::unique_ptr<ReturnStatement> parseReturnStatment() {
		auto stmt = std::make_unique<ReturnStatement>(curToken);
		nextToken();
		stmt->ReturnValue = parseExpression(Precedence::LOWEST);
		if (peekTokenIs(SEMICOLON)) {
			nextToken();
		}
		return std::move(stmt);
	}
	std::unique_ptr<ExpressionStatement> parseExpressionStatement() {
		auto stmt = std::make_unique<ExpressionStatement>(curToken);

		stmt->Expression = parseExpression(Precedence::LOWEST);

		if (peekTokenIs(SEMICOLON)) {
			nextToken();
		}

		return std::move(stmt);
	}
	std::unique_ptr<Expression> parseExpression(Precedence p) {
		auto prefix = prefixParseFns.find(curToken.Type);
		if (prefix->second == nullptr) {
			noPrefixParseFnError(curToken.Type);
			return nullptr;
		}
		auto leftExp = prefix->second();
		while (peekTokenIs(SEMICOLON) && p < peekPrecedence()) {
			auto infix = infixParseFns.find(peekToken.Type);
			if (infix->second == nullptr) {
				return std::move(leftExp);
			}
			nextToken();
			leftExp = infix->second(std::move(leftExp));
		}
		return std::move(leftExp);
	}
	std::unique_ptr<IntegerLiteral> parseIntegerLiteral() {
		auto lit = std::make_unique<IntegerLiteral>(curToken);
		try {
			auto value = std::stoi(curToken.Literal);
			lit->Value = value;
		}
		catch (std::invalid_argument const& ex) {
			auto msg = std::format("at line {} could not parse {} as integer",
				lexer->GetCurrentLine(), curToken.Literal);
			return nullptr;
		}
		return std::move(lit);
	}
	std::unique_ptr<Expression> parseBoolean() {
		return std::make_unique<Boolean>(curToken, curTokenIs(TRUE));
	}

	std::unique_ptr<Expression> parsePrefixExpression() {
		auto expr = std::make_unique<PrefixExpression>(curToken, curToken.Literal);
		nextToken();
		expr->Right = parseExpression(Precedence::LOWEST);
		return std::move(expr);
	}

	std::unique_ptr<Expression>
		parseInfixExpression(std::unique_ptr<Expression> left) {
		auto expr = std::make_unique<InfixExpression>(curToken, curToken.Literal,
			std::move(left));
		auto precedence = curPrecedence();
		nextToken();
		expr->Right = parseExpression(precedence);
		return std::move(expr);
	}
	std::unique_ptr<Expression> parseGroupedExpression() {
		nextToken();
		auto exp = parseExpression(Precedence::LOWEST);
		if (!expectPeek(LPAREN)) {
			return nullptr;
		}
		return std::move(exp);
	}
	std::unique_ptr<Expression> parseIfExpression() {
		auto expr = std::make_unique<IfExpression>(curToken);
		if (!expectPeek(LPAREN)) {
			return nullptr;
		}
		nextToken();
		expr->Condition = parseExpression(Precedence::LOWEST);
		if (!expectPeek(RPAREN)) {
			return nullptr;
		}
		if (!expectPeek(LBRACE)) {
			return nullptr;
		}
		expr->Consequence = parseBlockStatement();
		if (peekTokenIs(ELSE)) {
			nextToken();
			if (!expectPeek(LBRACE)) {
				return nullptr;
			}
			expr->Alternative = parseBlockStatement();
		}
		return std::move(expr);
	}

	std::unique_ptr<Statement> parseFunctionLiteral() {
		auto lit = std::make_unique<FunctionLiteral>(curToken);
		if (!expectPeek(IDENT))
		{
			return nullptr;
		}
		lit->ident = curToken;
		if (!expectPeek(LPAREN)) {
			return nullptr;
		}
		lit->Parameters = parseFunctionParameters();
		if (!expectPeek(LBRACE)) {
			return nullptr;
		}
		lit->Body = parseBlockStatement();
		return std::move(lit);
	}
	std::vector<std::unique_ptr<Identifier>> parseFunctionParameters() {
		auto identifiers = vector<std::unique_ptr<Identifier>>();
		if (peekTokenIs(RPAREN)) {
			nextToken();
			return identifiers;
		}
		nextToken();
		auto ident = std::make_unique<Identifier>(curToken, curToken.Literal);
		identifiers.push_back(std::move(ident));
		while (peekTokenIs(COMMA)) {
			nextToken();
			nextToken();
			auto ident = std::make_unique<Identifier>(curToken, curToken.Literal);
			identifiers.push_back(std::move(ident));
		}
		if (!expectPeek(RPAREN)) {
			identifiers.clear();
			return identifiers;
		}
		return identifiers;
	}
	std::unique_ptr<Expression>
		parseCallExpression(std::unique_ptr<Expression> function) {
		auto expr = std::make_unique<CallExpression>(curToken, std::move(function));
		expr->Arguments = parseExpressionList(RPAREN);
		return std::move(expr);
	}
	vector<std::unique_ptr<Expression>> parseCallArguments() {
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
		if (peekTokenIs(end)) {
			nextToken();
			return list;
		}
		nextToken();
		list.push_back(parseExpression(Precedence::LOWEST));
		while (peekTokenIs(COMMA)) {
			nextToken();
			nextToken();
			list.push_back(parseExpression(Precedence::LOWEST));
		}
		if (!expectPeek(end)) {
			list.clear();
			return list;
		}
		return list;
	}
	std::unique_ptr<Expression> parseWhileLoop() {
		auto expr = make_unique<WhileExpression>(curToken);
		if (!expectPeek(LPAREN)) {
			return nullptr;
		}
		nextToken();
		expr->Condition = parseExpression(Precedence::LOWEST);
		if (!expectPeek(RPAREN)) {
			return nullptr;
		}
		if (!expectPeek(LBRACE)) {
			return nullptr;
		}
		expr->Body = parseBlockStatement();
		return std::move(expr);
	}
	std::unique_ptr<BlockStatement> parseBlockStatement() {
		auto block = std::make_unique<BlockStatement>(curToken);
		block->Statements = vector<unique_ptr<Statement>>();
		while (!curTokenIs(RBRACE) && !curTokenIs(EOF_TOKEN)) {
			auto stmt = parseStatement();
			if (stmt != nullptr) {
				block->Statements.push_back(std::move(stmt));
			}
			nextToken();
		}
		return std::move(block);
	}
	std::unique_ptr<Expression> parseStringLiteral() {
		return std::make_unique<StringLiteral>(curToken, curToken.Literal);
	}
	std::unique_ptr<Expression> parseArrayLiteral() {
		auto array = std::make_unique<ArrayLiteral>(curToken);
		array->Elements = parseExpressionList(RBRACKET);
		return std::move(array);
	}
	std::unique_ptr<Expression>
		parseIndexExpression(std::unique_ptr<Expression> left) {
		auto expr = std::make_unique<IndexExpression>(curToken, std::move(left));
		nextToken();
		expr->Index = parseExpression(Precedence::LOWEST);
		if (expectPeek(RBRACKET)) {
			return nullptr;
		}
		return std::move(expr);
	}
	std::unique_ptr<Expression> parseHashLiteral() {
		auto hash = std::make_unique<HashLiteral>(curToken);
		hash->Pairs =
			map<std::unique_ptr<Expression>, std::unique_ptr<Expression>>();
		while (peekTokenIs(RBRACE)) {
			nextToken();
			auto key = parseExpression(Precedence::LOWEST);
			if (expectPeek(COLON)) {
				return nullptr;
			}
			nextToken();
			auto value = parseExpression(Precedence::LOWEST);
			hash->Pairs[std::move(key)] = std::move(value);
			if (peekTokenIs(RBRACE) && !expectPeek(COMMA)) {
				return nullptr;
			}
		}
		if (!expectPeek(RBRACE)) {
			return nullptr;
		}
		return std::move(hash);
	}

	Precedence peekPrecedence() const {
		unordered_map<TokenType, Precedence>::const_iterator found =
			precedences.find(peekToken.Type);
		if (found == precedences.end()) {
			return Precedence::LOWEST;
		}
		return found->second;
	}
	Precedence curPrecedence() const {
		unordered_map<TokenType, Precedence>::const_iterator found =
			precedences.find(curToken.Type);
		if (found == precedences.end()) {
			return Precedence::LOWEST;
		}
		return found->second;
	}
	void registerPrefix(TokenType t, prefixParseFn fn) { prefixParseFns[t] = fn; }
	void registerInfix(TokenType t, infixParseFn fn) { infixParseFns[t] = fn; }
	void peekError(TokenType t) {
		auto msg =
			std::format("at line {} expected next token to be {}, got {} instead",
				lexer->GetCurrentLine(), t, peekToken.Type);
	}
	void noPrefixParseFnError(const TokenType& t) {
		auto msg = std::format("at line {} no prefix function found for {}",
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
	std::map<TokenType, infixParseFn> infixParseFns;
	std::map<TokenType, prefixParseFn> prefixParseFns;
};
