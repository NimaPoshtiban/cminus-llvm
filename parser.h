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
// TODO: Implement this
unordered_map<TokenType, int> precedences = {
    {EQ, Precedence::EQUALS},          {NOT_EQ, Precedence::EQUALS},
    {LOGICAL_AND, Precedence::EQUALS}, {LOGICAL_OR, Precedence::EQUALS},
    {LT, Precedence::LESSGREATER},     {GT, Precedence::LESSGREATER},
    {GT_EQ, Precedence::LESSGREATER},  {LT_EQ, Precedence::LESSGREATER},
    {PLUS, Precedence::SUM},           {MINUS, Precedence::SUM},
    {SLASH, Precedence::PRODUCT},      {ASTERISK, Precedence::PRODUCT},
    {MODULO, Precedence::PRODUCT},     {RSHIFT, Precedence::PRODUCT},
    {LSHIFT, Precedence::PRODUCT},     {LPAREN, Precedence::CALL},
    {LBRACKET, Precedence::INDEX}};

class Parser {
public:
  Parser() {
    lexer = make_unique<Lexer>();
    errors = vector<string>();
    nextToken();
    nextToken();
  }
  // TODO: Implement this
  Program *ParserProgram() {
    auto program = new Program();
    while (curToken.Type.compare(EOF_TOKEN) != 0) {
      auto statement = parseStatement();
      if (statement != nullptr) {
        program->Statements.push_back(statement);
      }
      nextToken();
    }
  }

private:
  void nextToken(void) {
    curToken = peekToken;
    peekToken = lexer->NextToken();
  }
  Statement *parseStatement() {
    if (curToken.Type.compare(LET) == 0) {
      return parseLetStatement();
    } else if (curToken.Type.compare(RETURN) == 0) {
      return parseReturnStatment();
    }
    return parseExpressionStatement();
  }
  Identifier *parseIdentifier() {
    auto ident = new Identifier(curToken, curToken.Literal);
    return ident;
  }

  LetStatement *parseLetStatement() {
    auto statement = new LetStatement(curToken);
    if (!expectPeek(IDENT)) {
      return nullptr;
    }
    statement->Name = new Identifier(curToken, curToken.Literal);
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
  ReturnStatement *parseReturnStatment() {
    auto stmt = new ReturnStatement(curToken);
    nextToken();
    stmt->ReturnValue = parseExpression(Precedence::LOWEST);
    if (peekTokenIs(SEMICOLON)) {
      nextToken();
    }
    return stmt;
  }
  ExpressionStatement *parseExpressionStatement() {
    auto stmt = new ExpressionStatement(curToken);

    stmt->Expression = parseExpression(Precedence::LOWEST);

    if (peekTokenIs(SEMICOLON)) {
      nextToken();
    }

    return stmt;
  }
  // TODO: Implement this
  Expression *parseExpression(Precedence p) {
    auto prefix = prefixParseFns.at(curToken.Type);
    if (prefix == nullptr) {
      // TODO: Implement this
      return nullptr;
    }
    auto leftExp = prefix();
    while (peekTokenIs(SEMICOLON) && p < peekPrecedence()) {
      auto infix = infixParseFns.at(peekToken.Type);
      if (infix == nullptr) {
        return leftExp;
      }
      nextToken();
      leftExp = infix(leftExp);
    }
  }

  void peekError(TokenType t) {
    auto msg =
        std::format("at line {} expected next token to be {}, got {} instead",
                    lexer->GetCurrentLine(), t, peekToken.Type);
  }
  bool curTokenIs(TokenType t) { return !(curToken.Type.compare(t)); }
  bool peekTokenIs(TokenType t) { return !(peekToken.Type.compare(t)); }
  bool expectPeek(TokenType t) {
    if (peekTokenIs(t)) {
      nextToken();
      return true;
    } else {
      peekError(t);
      return false;
    }
  }
  unique_ptr<Lexer> lexer;
  Token curToken;
  Token peekToken;
  vector<string> errors;
  unordered_map<TokenType, function<Expression *()>> prefixParseFns;
  unordered_map<TokenType, function<Expression *(Expression *)>> infixParseFns;
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
