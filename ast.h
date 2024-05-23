#pragma once
#include <cstdint>
#include <map>
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
  Program() { Statements = vector<Statement *>(); }
  Program(vector<Statement *> statements) : Statements(statements) {}
  vector<Statement *> Statements;

  string TokenLiteral() {
    if (Statements.size() > 0) {
      return Statements[0]->TokenLiteral();
    } else {
      return "";
    }
  }

  string String() {
    string out = "";

    for (auto s : Statements) {
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
  Identifier *Name;
  Expression *Value;

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
  ReturnStatement(Token token) : Token(token) {}
  Token Token; // the 'return' token
  Expression *ReturnValue;
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
  ExpressionStatement(Token token) : Token(token) {}
  Token Token; // the first token of the expression
  Expression *Expression;

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
  Expression *Right;

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
  InfixExpression(Token token, string operator_, Expression *left)
      : Token(token), Operator(operator_), Left(left) {}
  Token Token; // the operator such as + , * ,....
  Expression *Left;
  string Operator;
  Expression *Right;

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
  IndexExpression(Token token, Expression *left) : Token(token), Left(left) {}
  Token Token; // The [ token
  Expression *Left;
  Expression *Index;

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
  vector<Statement *> Statements;

  void statementNode() {}

  string TokenLiteral() { return Token.Literal; }

  string String() {
    string out = "";
    for (auto s : Statements) {
      out += s->String();
    }
    return out;
  }
};

struct IfExpression : Expression {
  IfExpression(Token token) : Token(token) {}
  Token Token;
  Expression *Condition;
  BlockStatement *Consequence;
  BlockStatement *Alternative;

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
  Expression *Condition;
  BlockStatement *Body;

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

struct FunctionLiteral : Expression {
  FunctionLiteral(Token token) : Token(token) {}
  Token Token; // the 'func' token
  vector<Identifier *> Parameters;
  BlockStatement *Body;

  void expressionNode() {}

  string TokenLiteral() { return Token.Literal; }

  string String() {
    string out = "";

    vector<string> params;
    for (auto p : Parameters) {
      params.push_back(p->String());
    }

    out += TokenLiteral();
    out += "(";
    for (auto &p : params) {
      out += p;
      out += ", ";
    }
    out += ") ";
    out += Body->String();
    return out;
  }
};

struct CallExpression : Expression {
  CallExpression(Token token, Expression *function)
      : Token(token), Function(function) {}
  Token Token;          // The '(' token
  Expression *Function; // Identifier or FunctionLiteral
  vector<Expression *> Arguments;

  void expressionNode() {}

  string TokenLiteral() { return Token.Literal; }

  string String() {
    string out = "";
    vector<string> args;
    for (auto a : Arguments) {
      args.push_back(a->String());
    }
    out += Function->String();
    out += "(";
    for (auto &arg : args) {
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
  vector<Expression *> Elements;

  void expressionNode() {}

  string TokenLiteral() { return Token.Literal; }

  string String() {
    string out = "";
    vector<string> elements;
    for (auto el : Elements) {
      elements.push_back(el->String());
    }
    out += "[";
    for (auto &el : elements) {
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
  map<Expression *, Expression *> Pairs;

  void expressionNode() {}

  string TokenLiteral() { return Token.Literal; }

  string String() {
    string out = "";
    vector<string> pairs;
    for (auto pair : Pairs) {
      pairs.push_back(pair.first->String() + ":" + pair.second->String());
    }

    out += "{";
    for (auto &p : pairs) {
      out += p;
      out += ", ";
    }
    out += "}";
    return out;
  }
};
