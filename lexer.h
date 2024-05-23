#pragma once
#define LEXER_H
#include <cctype>
#include <string>
#include <unordered_map>

using namespace std;

typedef string TokenType;

struct Token {
  TokenType Type;
  string Literal;
};

const string ILLEGAL = "ILLEGAL";
const string EOF_TOKEN = "EOF";

// identifiers + literals
const string IDENT = "IDENT"; // add , foobar ,x ,y ,...
const string INT = "INT";     // integers

// Operators
const string ASSIGN = "=";
const string PLUS = "+";
const string MINUS = "-";
const string BANG = "!";
const string ASTERISK = "*";
const string SLASH = "/";
const string MODULO = "%";
const string LSHIFT = "<<";
const string RSHIFT = ">>";

const string LT = "<";
const string GT = ">";

const string EQ = "==";
const string NOT_EQ = "!=";
const string LT_EQ = "<=";
const string GT_EQ = ">=";

const string LOGICAL_AND = "and";
const string LOGICAL_OR = "or";

// Delimiters
const string COMMA = ",";
const string SEMICOLON = ";";
const string STRING = "STRING";

const string LPAREN = "(";
const string RPAREN = ")";
const string LBRACE = "{";
const string RBRACE = "}";

const string LBRACKET = "[";
const string RBRACKET = "]";

const string COLON = ":";

// keywords
const string MACRO = "macro";
const string FUNCTION = "func";
const string LET = "let";
const string TRUE = "true";
const string FALSE = "false";
const string IF = "if";
const string ELSE = "else";
const string RETURN = "return";
const string WHILE = "while";

// language keywords
inline unordered_map<string, TokenType> keywords = {
    {"func", FUNCTION}, {"macro", MACRO},   {"let", LET},
    {"true", TRUE},     {"false", FALSE},   {"if", IF},
    {"else", ELSE},     {"return", RETURN}, {"and", LOGICAL_AND},
    {"or", LOGICAL_OR}, {"while", WHILE},
};

// check to see if the given identifier is a keyword
inline TokenType LookupIdent(const string &ident) {
  if (keywords.find(ident) != keywords.end()) {
    return keywords[ident];
  }
  return IDENT;
}

class Lexer {
public:
  string input;
  Lexer(const string &input = "")
      : input(input), position(0), readPosition(0), ch(0), line(1) {
    readChar();
  }

  Token NextToken() {
    Token tok;

    skipWhitespace();

    switch (ch) {
    case '%':
      tok = newToken(MODULO, ch);
      break;
    case '=':
      if (peekChar() == '=') {
        char ch = this->ch;
        readChar();
        tok = Token{EQ, string(1, ch) + string(1, this->ch)};
      } else {
        tok = newToken(ASSIGN, ch);
      }
      break;
    case '-':
      tok = newToken(MINUS, ch);
      break;
    case '!':
      if (peekChar() == '=') {
        char ch = this->ch;
        readChar();
        tok = Token{NOT_EQ, string(1, ch) + string(1, this->ch)};
      } else {
        tok = newToken(BANG, ch);
      }
      break;
    case '*':
      tok = newToken(ASTERISK, ch);
      break;
    case '/':
      tok = newToken(SLASH, ch);
      break;
    case '<':
      if (peekChar() == '=') {
        char ch = this->ch;
        readChar();
        tok = Token{LT_EQ, string(1, ch) + string(1, this->ch)};
      } else if (peekChar() == '<') {
        char ch = this->ch;
        readChar();
        tok = Token{LSHIFT, string(1, ch) + string(1, this->ch)};
      } else {
        tok = newToken(LT, ch);
      }
      break;
    case '>':
      if (peekChar() == '=') {
        char ch = this->ch;
        readChar();
        tok = Token{GT_EQ, string(1, ch) + string(1, this->ch)};
      } else if (peekChar() == '>') {
        char ch = this->ch;
        readChar();
        tok = Token{RSHIFT, string(1, ch) + string(1, this->ch)};
      } else {
        tok = newToken(GT, ch);
      }
      break;
    case ';':
      tok = newToken(SEMICOLON, ch);
      break;
    case '(':
      tok = newToken(LPAREN, ch);
      break;
    case ')':
      tok = newToken(RPAREN, ch);
      break;
    case ',':
      tok = newToken(COMMA, ch);
      break;
    case '+':
      tok = newToken(PLUS, ch);
      break;
    case '{':
      tok = newToken(LBRACE, ch);
      break;
    case '}':
      tok = newToken(RBRACE, ch);
      break;
    case '[':
      tok = newToken(LBRACKET, ch);
      break;
    case ']':
      tok = newToken(RBRACKET, ch);
      break;
    case '"':
      tok.Type = STRING;
      tok.Literal = readString();
      break;
    case ':':
      tok = newToken(COLON, ch);
      break;
    case 0:
      tok.Literal = "";
      tok.Type = EOF_TOKEN;
      break;
    default:
      if (isLetter(ch)) {
        tok.Literal = readIdentifier();
        tok.Type = LookupIdent(tok.Literal);
        return tok;
      } else if (isdigit(ch)) {
        tok.Type = INT;
        tok.Literal = readNumber();
        return tok;
      } else {
        tok = newToken(ILLEGAL, ch);
      }
      break;
    }

    readChar();
    return tok;
  }

  int GetCurrentLine() const { return line; }

private:
  int position;     // current position in input (points to current char)
  int readPosition; // current reading position in input (after current char)
  char ch;          // current char under examination
  int line;         // the current scanning line

  void readChar() {
    if (readPosition >= input.size()) {
      ch = 0;
    } else {
      ch = input[readPosition];
    }
    position = readPosition;
    readPosition += 1;
  }

  char peekChar() const {
    if (readPosition >= input.size()) {
      return 0;
    } else {
      return input[readPosition];
    }
  }

  void skipWhitespace() {
    while (isspace(ch)) {
      if (ch == '\n') {
        line++;
      }
      readChar();
    }
  }

  string readIdentifier() {
    int startPos = position;
    while (isLetter(ch)) {
      readChar();
    }
    return input.substr(startPos, position - startPos);
  }

  string readNumber() {
    int startPos = position;
    while (isdigit(ch)) {
      readChar();
    }
    return input.substr(startPos, position - startPos);
  }

  string readString() {
    int startPos = position + 1;
    while (true) {
      readChar();
      if (ch == '"' || ch == 0) {
        break;
      }
    }
    return input.substr(startPos, position - startPos);
  }

  Token newToken(TokenType tokenType, char ch) {
    return Token{tokenType, string(1, ch)};
  }

  static bool isLetter(char ch) {
    return isalpha(ch) || ch == '_' || ch == '$';
  }
};
