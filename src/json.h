#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include "fnv.h"

#define VAL_STRING        1
#define VAL_INT           2
#define VAL_UINT          3
#define VAL_FLOAT         4
#define VAL_DOUBLE        5
#define VAL_BOOL          6
#define VAL_OBJ           7
#define VAL_STRING_ARRAY  8
#define VAL_INT_ARRAY     9
#define VAL_FLOAT_ARRAY   10
#define VAL_DOUBLE_ARRAY  11
#define VAL_BOOL_ARRAY    12
#define VAL_OBJ_ARRAY     13
#define VAL_MIXED_ARRAY   14 /* JValue array */
#define VAL_NULL          15

typedef union Value {
  void *ptr_val;
  float float_val;
  double double_val;
  int int_val;
  unsigned char char_val;
  char *string_val;
  struct JArray *array_val;
  struct JObject *object_val;
} JItemValue;

typedef struct Item {
  unsigned char type :4;
  JItemValue value;
} JArrayItem;

typedef union Items {
  void *items;
  JArrayItem **vItems;
} JArrayItems;

typedef struct JArray {
  unsigned int count;
  unsigned char type :4;
  JArrayItems _internal;
} JArray;

typedef struct JEntry {
  char *name;
  JItemValue value;
  int hash;
  unsigned short probes;
  unsigned char value_type :4;
} JEntry;

typedef struct JObject {
  JEntry **entries; // hashed arrangement by key.
  unsigned int size;
  unsigned int _incrementSize;
  unsigned int _arraySize;
  unsigned short _maxProbes;
  unsigned char value_type :4;
} JObject;

extern JObject *stringCache;

// Building functions
JObject* jsonAddVal(JObject *obj, const char *name, JItemValue value,
    short type);
/* Convenience methods */
JObject* jsonAddObj(JObject *obj, const char *name, JObject *value);
JObject* jsonAddInt(JObject *obj, const char *name, const int value);
JObject* jsonAddUInt(JObject *obj, const char *name, const unsigned int value);
JObject* jsonAddString(JObject *obj, const char *name, const char *value);
JObject* jsonDeleteKey(JObject *obj, const char *key);

JArray* jsonAddArrayItem(JArray *arr, JArrayItem *item);
JArray* jsonAddArrayItemObject(JArray *arr, JObject *obj);


#define NO_DUP 0
#define DUP 1

JItemValue jsonParse(const char *filename, short *type);
JItemValue jsonParseF(FILE *file, short *type);
const char* jsonParserError();
void jsonPrintError();
JObject* jsonNewObject();
JArray* jsonNewArray();
JItemValue jsonGet(const JObject *obj, const char *keys, short *type);
int jsonInt(const JObject *obj, const char *keys);
unsigned int jsonUInt(const JObject *obj, const char *keys);
float jsonFloat(const JObject *obj, const char *keys);
double jsonDouble(const JObject *obj, const char *keys);
char* jsonString(const JObject *obj, const char *keys);
char jsonBool(const JObject *obj, const char *keys);
JArray* jsonArray(const JObject *obj, const char *keys);
char* jsonBoolArray(const JObject *obj, const char *keys);
int** jsonIntArray(const JObject *obj, const char *keys);
float** jsonFoatArray(const JObject *obj, const char *keys);
double** jsonDoubleArray(const JObject *obj, const char *keys);
char** jsonStringArray(const JObject *obj, const char *keys);
JObject* jsonObject(const JObject *obj, const char *keys);

JArrayItem **jsonArrayItemList(JArray *array);
JObject **jsonArrayKeyFilter(JArray* array, const char* key, unsigned *size);

void jsonPrintObject(const FILE *io, const JObject *obj);
void jsonPrintEntry(const FILE *io, unsigned char type, JItemValue *value,
    unsigned int tabs, unsigned int tabInc);
void jsonPrintEntry(const FILE *io, const unsigned short type,
    const JItemValue *value);

void jsonFree(JItemValue val, const short vtype);

// miscellaneous
char* getOrCacheString(const char *value);

#endif
