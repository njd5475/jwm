/*
 * Token.cpp
 *
 *  Created on: Feb 18, 2020
 *      Author: nick
 */

#include <sys/stat.h>
#include <algorithm>

#include "debug.h"
#include "string.h"
#include "logger.h"
#include "misc.h"
#include "Token.h"

Token::Token(const char *start) :
    _count(0), _type(UNKNOWN), _start(start) {
}

Token::Token(const Token &cpy) {
  _count = cpy._count;
  _type = cpy._type;
  _start = cpy._start;
}

Token::~Token() {
  // TODO Auto-generated destructor stub
}

unsigned Token::getCount() {
  return _count;
}

Token* Token::next(const char *position) {
  Token *cur = this;
  char ch = position[0];
  if (_count == 0 && _count < 256) {
    _type = getType(ch);
    ++_count;
  } else if (getType(ch) == _type && _type != NEW_LINE && _count < 256) {
    ++_count;
  } else {
    return NULL;
  }
  return cur;
}

Token::TYPE Token::getType(char ch) {
  if (ch == '\'') {
    return SINGLE_QUOTE;
  } else if (ch == '\"') {
    return DOUBLE_QUOTE;
  } else if (ch == '=') {
    return EQUAL;
  } else if (ch == '\n') {
    return NEW_LINE;
  } else if (ch == '\t') {
    return TAB;
  } else if (ch == '\\') {
    return ESCAPE_CHAR;
  } else if (ch == ' ') {
    return SPACE;
  } else if (ch == '#') {
    return POUND;
  } else if (ch == '+') {
    return PLUS;
  }
  return UNKNOWN;
}

const char* Token::copy() {
  char *dest = new char[_count + 1];
  memset(dest, 0, sizeof(char) * (_count + 1));
  return strncpy(dest, _start, _count);
}

bool Token::compare(const char *str) {
  const char *token = copy();

  bool comp = strcmp(token, str) == 0;

  delete token;

  return comp;
}

Token::TYPE Token::getType() {
  return _type;
}

//################## Tokenizer ##########################
#ifdef USE_INOTIFYTOOLS
#include <inotifytools/inotify.h>
#include <inotifytools/inotifytools.h>
#endif
Tokenizer::Tokenizer(FILE *file) :
    _file(file), _eof(false), _position(0), _lastReadCount(0), _theToken(
        Token(_charBuffer)), _nextToken(false) {
  memset((void*) _charBuffer, 0, BUFFER_SIZE * sizeof(char));
}

Tokenizer::Tokenizer(const Tokenizer &cpy) :
    _theToken(_charBuffer) {
  _file = cpy._file;
  _eof = cpy._eof;
  _position = cpy._position;
  _lastReadCount = cpy._lastReadCount;
  memset((void*) _charBuffer, 0, BUFFER_SIZE * sizeof(char));
  memcpy((void*) _charBuffer, cpy._charBuffer, BUFFER_SIZE * sizeof(char));
  _theToken = Token(cpy._theToken);
  _nextToken = cpy._nextToken;
}

Tokenizer::~Tokenizer() {

}

void Tokenizer::fillBuffer() {
  memset((void*) _charBuffer, 0, sizeof(char) * BUFFER_SIZE);
  _lastReadCount = fread((void*) _charBuffer, 1, BUFFER_SIZE, _file);
  if (_lastReadCount == 0) {
    _eof = true;
  }
}

Tokenizer Tokenizer::create(const char *wildcardname) {
  struct stat tmpStat;

  char *path;

  path = CopyString(wildcardname);
  ExpandPath(&path);
  const char *filename = path;
  int statError = stat(filename, &tmpStat);
  if (statError) {
    fprintf(stderr, "Could not stat file '%s'\n", filename);
    return NULL;
  }

#ifdef USE_INOTIFYTOOLS
      int res = inotifytools_initialize();
      if(!res) {
        Log("Could not start inotifytools");
      }
      //setup inotify watch
      res = inotifytools_watch_file(filename, IN_MODIFY | IN_CREATE );
      if(!res) {
        vLog("Could not watch file %s\n", filename);
      }else{
        vLog("WARNING: watching %s for changes\n", filename);
      }
#endif

  size_t bufSize = std::min(BUFFER_SIZEL, tmpStat.st_size);

  printf("Parsing file with buffer size %d\n", bufSize);
  FILE *file = fopen(filename, "r");

  return Tokenizer(file);
}

Token* Tokenizer::current() {
  return &_theToken;
}

bool Tokenizer::isValid() {
  return _file != NULL;
}

const char* Tokenizer::bufferPosition() {
  const char *ptr = (const char*) &_charBuffer[_position];
  return ptr;
}

Token* Tokenizer::next() {
  Token *toRet = current();
  if (_eof) {
    return NULL;
  }

  if (_position >= _lastReadCount) {
    _position = 0;
    fillBuffer();
    //just set next token to true but the following if must come after a fill
    _nextToken = true;
  }

  if (_nextToken) {
    _theToken = Token(bufferPosition());
    _nextToken = false;
  }

  for (; _position < _lastReadCount; ++_position) {
    Token *next = toRet->next(bufferPosition());
    // we are moving on to the next token
    if (next == NULL) {
      _nextToken = true;
      break;
    }
  }

  return toRet;
}

//############################ Parser ###########################
#include <string>

NicksConfigParser::NicksConfigParser(Tokenizer tokenizer) :
    _tokenizer(tokenizer), _lastToken(NULL), _indentLevel(0), _escapeSequenceCount(
        0) {

}

NicksConfigParser::~NicksConfigParser() {

}

JItemValue NicksConfigParser::parse(const char *wildcardname) {

  Tokenizer tokenizer = Tokenizer::create(wildcardname);
  if (!tokenizer.isValid()) {
    return JItemValue { 0 };
  }
  NicksConfigParser parser(tokenizer);

  return parser.rootObject();
}

JItemValue NicksConfigParser::rootObject() {
  JArray *arr = jsonNewArray();
  JObject *obj = NULL;
  try {
    while (true) {
      obj = jsonNewObject();
      whitespace(true);
      JObject *child = object(obj);
      if (child != NULL) {
        jsonAddArrayItemObject(arr, obj);
        obj = NULL;
      } else {
        Log("Object was null\n");
        jsonFree(JItemValue { obj }, VAL_OBJ);
      }
    }
  } catch (Tokenizer &t) {
    jsonAddArrayItemObject(arr, obj);
  }
  return JItemValue { arr };
}

JObject* NicksConfigParser::object(JObject *parent, unsigned tabs) {
  if (_indentLevel != tabs) {
    return NULL;
  }
  const char *ident = identifier();
  if (ident == NULL) {
    return NULL;
  }

  JObject *obj = jsonNewObject();
  if (parent != NULL) {
    jsonAddObj(parent, ident, obj);
  } else {
    jsonAddString(obj, "name", ident);
  }
  whitespace();
  do {
    if (isToken( { Token::PLUS })) {
      consume();
      whitespace();
    }
    while (_lastToken->getType() != Token::NEW_LINE) {
      //has more properties
      const char *key = identifier();
      if (_lastToken->getType() == Token::EQUAL) {
        consume(true);
        const char *value = identifier();
        jsonAddString(obj, key, value);
      } else {
        //set the key value to true
        JItemValue val;
        val.char_val = 1;
        jsonAddVal(obj, key, val, VAL_BOOL);
      }
      whitespace();
    }

    // setting the indent level here because we need to expect children objects
    // to be at tabs + 1
    whitespace(true);
  } while (isToken( { Token::PLUS }));

  children(obj, tabs + 1);

  return obj;
}

JArray* NicksConfigParser::children(JObject *parent, unsigned tabs) {
  JArray *arry = NULL;
  JObject *item = NULL;
  JObject *child = NULL;
  try {
    do {
      whitespace(true);
      child = jsonNewObject();
      item = object(child, tabs);
      if (item != NULL) {
        if (arry == NULL) {
          arry = jsonNewArray();
        }
        jsonAddArrayItemObject(arry, child);
      } else {
        jsonFree(JItemValue { item }, VAL_OBJ);
      }
    } while (item != NULL);
  } catch (Tokenizer &t) {
    if(child != NULL) {
      if(arry == NULL) {
        arry = jsonNewArray();
      }
      jsonAddArrayItemObject(arry, child);
    }
    if (arry != NULL) {
      jsonAddVal(parent, "children", JItemValue { arry }, VAL_MIXED_ARRAY);
    }
    throw t;
  }
  if (arry != NULL) {
    jsonAddVal(parent, "children", JItemValue { arry }, VAL_MIXED_ARRAY);
  }
  return arry;
}

unsigned NicksConfigParser::setIndentLevel() {
  unsigned tabs = 0;
  while (_lastToken->getType() == Token::TAB) {
    tabs += _lastToken->getCount();
    consume();
  }
  _indentLevel = tabs;
  return _indentLevel;
}

void NicksConfigParser::getNextToken(bool allowPound) {
  if (_lastToken == NULL) {
    _lastToken = _tokenizer.next();

    if (_lastToken == NULL || _lastToken->getCount() == 0) {
      throw _tokenizer;
    }

    if (isToken( { Token::NEW_LINE })) {
      _indentLevel = 0;
    }

    if (isToken( { Token::ESCAPE_CHAR }) || _escapeSequenceCount > 0) {
      ++_escapeSequenceCount;
    }

    if (!_escapeSequenceCount && isToken( { Token::POUND }) && !allowPound) {
      //consume all tokens to eliminate the comment line
      Token *tmp = NULL;
      do {
        tmp = _tokenizer.next();
        if (tmp == NULL || tmp->getCount() == 0) {
          throw _tokenizer;
        }
      } while (tmp->getType() != Token::NEW_LINE);
      _lastToken = tmp;
      _indentLevel = 0;
    }

    if (_escapeSequenceCount > 1) {
      _escapeSequenceCount = 0; //only allow a single escape character for the time being
    }
  }
}

const char* NicksConfigParser::identifier() {
  std::string id;
  const char *tok;
  while (!isToken( { Token::SPACE, Token::SINGLE_QUOTE, Token::DOUBLE_QUOTE,
      Token::TAB, Token::EQUAL, Token::NEW_LINE })) {
    id.append(tok = _lastToken->copy());
    delete tok;
    consume(true);
  }

  if (isToken( { Token::SINGLE_QUOTE, Token::DOUBLE_QUOTE })) {
    return quotedIdentifier();
  }

  if (id.size() == 0) {
    return NULL;
  }

  return strdup(id.c_str());
}

const char* NicksConfigParser::quotedIdentifier() {
  unsigned quoteType = _lastToken->getType();
  std::string id;
  const char *tok;
  consume(true);
  while (!isToken( { quoteType })) {
    id.append(tok = _lastToken->copy());
    delete tok;
    consume(true);
  }
  consume(true);

  return strdup(id.c_str());
}

void NicksConfigParser::whitespace(bool includeNewLines, bool consumeCommentLines) {
  getNextToken();

  while (_lastToken != NULL
      && (isToken( { Token::SPACE }) || (includeNewLines && isToken( {
          Token::NEW_LINE })))) {
    if (includeNewLines && _lastToken->getType() == Token::NEW_LINE
        && _indentLevel > 0) {
      _indentLevel = 0;
    }
    consume(consumeCommentLines);
  }

}

void NicksConfigParser::consume(bool allowPound) {
  bool resetIndentLevel = false;
  if (_lastToken && _lastToken->getType() == Token::NEW_LINE) {
    resetIndentLevel = true;
  }
  _lastToken = NULL;
  getNextToken(allowPound);
  if (resetIndentLevel) {
    setIndentLevel();
  }
}

bool NicksConfigParser::isToken(std::initializer_list<unsigned> types) {
  bool retVal = false;

  for (auto t : types) {
    if (_lastToken->getType() == t) {
      retVal = true;
      break;
    }
  }

  return retVal;
}
