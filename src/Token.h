/*
 * Token.h
 *
 *  Created on: Feb 18, 2020
 *      Author: nick
 */

#ifndef SRC_TOKEN_H_
#define SRC_TOKEN_H_

#include <map>
#include <stdio.h>
#include "json.h"

class Token {
public:
  Token(const char*);
  Token(const Token&);
  virtual ~Token();

  enum TYPE {UNKNOWN, EQUAL, ESCAPE_CHAR, SINGLE_QUOTE, DOUBLE_QUOTE, NEW_LINE, TAB, SPACE, POUND, PLUS};

  Token* next(const char*);

  bool compare(const char* str);
  unsigned getCount();
  TYPE getType();
  const char* copy();

private:
  TYPE getType(char ch);

  unsigned _count;
  const char* _start;
  TYPE _type;
};

#define BUFFER_SIZE 2048
#define BUFFER_SIZEL 2048L
class Tokenizer {
public:
  Tokenizer(const Tokenizer&);
  Tokenizer(FILE *file);
  virtual ~Tokenizer();

  Token *next();
  bool isValid();
  static Tokenizer create(const char* wildcardname);

private:
  Token* current();
  const char* bufferPosition();
  void fillBuffer();

  FILE *_file;

  bool _eof;
  bool _nextToken;
  Token _theToken;
  const char _charBuffer[BUFFER_SIZE];
  size_t _lastReadCount;
  size_t _position;

};

class NicksConfigParser {
private:
  NicksConfigParser(Tokenizer tokenizer);
  virtual ~NicksConfigParser();

  JItemValue rootObject();
  JObject *object(JObject *parent, unsigned tabCount = 0);
  JArray *children(JObject *parent, unsigned tabCount = 0);
  void whitespace(bool includeNewLine = false, bool consumeCommentLines = false);
  void consume(bool allowPound = false);
  void getNextToken(bool allowPound = false);
  const char* identifier();
  const char* quotedIdentifier();
  unsigned setIndentLevel();
  bool isToken(std::initializer_list<unsigned> types);

  Tokenizer _tokenizer;
  Token *_lastToken;
  unsigned _indentLevel;
  unsigned _escapeSequenceCount;
public:
  static JItemValue parse(const char *file);
private:
  static std::map<const char*, NicksConfigParser> parsers;

};

#endif /* SRC_TOKEN_H_ */
