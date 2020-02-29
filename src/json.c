#include "json.h"

#include <libio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_HASH_SIZE   15
#define DEFAULT_INC_AMOUNT  100

JObject *stringCache = NULL;

int jsonGetEntryIndex(const JObject *obj, const char* keys);
void jsonPrintObjectTabs(const FILE *io, const JObject* obj, unsigned int tabs, unsigned int tabInc);

char *nextKey(const char *keys, int *last) {
  if (!keys || !last) {
    return 0;
  }

  int start = *last;
  int begin = start;
  for (; keys[start] != '.' && keys[start]; ++start) {
    ;
  }

  int size = start - begin;
  if (size <= 0) {
    *last = -1;
    return 0;
  }

  char *newkey = malloc(size + 1);
  memset(newkey, 0, size + 1);

  int another = begin;
  for (; begin <= start; ++begin) {
    newkey[start - begin] = keys[another + (start - begin)];
  }

  newkey[size] = '\0';
  *last = start + 1;

  if (keys[start] == '\0') {
    *last = -1;
  }

  return newkey;
}

JEntry* insertInto(JEntry *entry, JObject *obj) {
  int index = (entry->hash + (entry->probes - 1)) % obj->_arraySize;

  while (obj->entries[index] != 0) {
    // steal the slot if the probed entrys' probes is less than mine
    if (obj->entries[index]->probes < entry->probes) {
      JEntry *tmp = obj->entries[index];
      obj->entries[index] = entry;
      entry = tmp; //swap
    }

    ++entry->probes;
    // adjust hash key and keep going
    index = (entry->hash + (entry->probes - 1)) % obj->_arraySize;

    if (entry->probes > obj->_maxProbes) {
      obj->_maxProbes = entry->probes;
    }
  }
  obj->entries[index] = entry;
  ++obj->size;

  return 0;
}

JObject* jsonDeleteKey(JObject *obj, const char *key) {
  int index = jsonGetEntryIndex(obj, key);
  if(index > -1) {
    JEntry *toDel = obj->entries[index];
    obj->entries[index] = 0;
    free(toDel->name);
    jsonFree(toDel->value, toDel->value_type);
    free(toDel);
    return obj;
  }
  obj->size--;
  return 0;
}

void signalHandler() {
  if(!stringCache) {
    return;
  }
  JObject *obj = stringCache;
  JEntry *toDel = NULL;
  JItemValue valToDel = { 0 };
  for (int i = 0; i < obj->_arraySize; ++i) {
    if (obj->entries[i] != NULL) {
      toDel = obj->entries[i];
      valToDel = toDel->value;
      //free(valToDel.ptr_val);
      free(toDel->name);
      free(toDel);
    }
  }
  free(obj->entries);
  free(obj);
  stringCache = 0;
}

JObject *jsonAddVal(JObject *obj, const char *name, JItemValue value, short type) {
  if (!obj) {
    obj = jsonNewObject();
  }

  if(type == 0) {
    fprintf(stderr, "WARNING: Adding entry with invalid type to object for key %s\n", name);
  }

  if(stringCache == NULL && stringCache != obj) {
    stringCache = jsonNewObject();
    if(atexit(signalHandler) != 0) {
      fprintf(stderr, "WARNING: Could not register cleanup function!");
    }
  }
  char *cached = NULL;
  if(stringCache != NULL && stringCache != obj) {
    cached = jsonString(stringCache, name);
    if(cached == NULL) {
      cached = strdup(name);
      jsonAddString(stringCache, cached, cached);
    }
  }
  JEntry *entry = malloc(sizeof(JEntry));
  if(cached) {
    entry->name = cached;
  }else{
    entry->name = strdup(name);
  }
  entry->value_type = type;
  entry->value = value;
  entry->probes = 1;
  entry->hash = fnvstr(entry->name);
  entry = insertInto(entry, obj);

  if (entry || obj->size == obj->_arraySize) {
    //now determine to expand the entry array
    JEntry **oldEntries = obj->entries;
    int oldCount = obj->_arraySize;
    obj->_arraySize += obj->_incrementSize;
    obj->_incrementSize += (int)(obj->_arraySize * 0.1f);
    obj->size = 0;
    obj->entries = malloc(sizeof(JEntry*) * obj->_arraySize);
    memset(obj->entries, 0, sizeof(JEntry*) * obj->_arraySize);
    for (int i = 0; i < oldCount; ++i) {
      if (oldEntries[i]) {
        oldEntries[i]->probes = 1;
        if (insertInto(oldEntries[i], obj) != 0) {
          printf("Our new size did not spread the keys enough\n");
        }
      }
    }
    if (entry) {
      entry->probes = 0;
      insertInto(entry, obj);
    }
    free(oldEntries);
  }

  return obj;
}

JObject* jsonAddObj(JObject *obj, const char *name, JObject *value) {
  return jsonAddVal(obj, name, (JItemValue) { value }, VAL_OBJ);
}

JObject* jsonAddInt(JObject *obj, const char *name, const int value) {
  return jsonAddVal(obj, name, (JItemValue) { value }, VAL_INT);
}

JObject* jsonAddUInt(JObject *obj, const char *name, const unsigned int value) {
  return jsonAddVal(obj, name, (JItemValue) { value }, VAL_UINT);
}

JObject *jsonAddString(JObject *obj, const char *name, const char *value) {
  return jsonAddVal(obj, name, (JItemValue) { value }, VAL_STRING);
}

JObject *jsonAddBool(JObject *obj, const char *name, const char value) {
  return jsonAddVal(obj, name, (JItemValue) { value }, VAL_BOOL);
}

JItemValue _jsonGetObjVal(const JObject *obj, const char* keys, short *type) {
  if(!keys) {
    return (JItemValue){ 0 };
  }
  int keyhash = fnvstr(keys);
  int index = keyhash % obj->_arraySize;

  int probes = -1;
  do {
    // not found, keep probing
    ++probes;
    index = (keyhash + probes) % obj->_arraySize;
    if (obj->entries[index] != 0 && obj->entries[index]->hash == keyhash) {
      *type = obj->entries[index]->value_type;
      return obj->entries[index]->value;
    }
  } while (probes < obj->_maxProbes);

  return (JItemValue){ 0 };
}

int jsonGetEntryIndex(const JObject *obj, const char* keys) {
  int keyhash = fnvstr(keys);
  int index = keyhash % obj->_arraySize;

  int probes = -1;
  do {
    // not found, keep probing
    ++probes;
    index = (keyhash + probes) % obj->_arraySize;
    if (obj->entries[index] != 0 && obj->entries[index]->hash == keyhash) {
      return index;
    }
  } while (probes < obj->_maxProbes);

  return -1;
}

JItemValue jsonGet(const JObject *obj, const char* keys, short *type) {
  if (obj == 0 || keys == NULL) {
    return (JItemValue) { 0 };
  }

  JItemValue jval = { 0 };
  const JObject *found = obj;
  char *key = NULL;
  int index = 0;
  do {
    key = nextKey(keys, &index);
    jval = _jsonGetObjVal(found, key, type);
    if (jval.ptr_val && *type == VAL_OBJ) {
      found = jval.object_val;
    } else if (key && index > -1) {
      if (jval.ptr_val) {
        printf("Err: Key %s was the wrong type.\n", keys);
      } else {
        printf("Err: Object did not contain a next object %s\n", keys);
      }
      if (key) {
        free(key);
      }
      return (JItemValue) { 0 };
    }

    if (key != NULL) {
      free(key);
      key = NULL;
    }
  } while (index > -1);

  return jval;
}

char *last(const char *keys) {
  if (!keys) {
    return 0;
  }

  int len = strlen(keys);
  const char *lastDot = keys + len;
  const char* end = lastDot;
  while (*lastDot != '.' && lastDot > keys) {
    --lastDot;
  }
  if (keys == lastDot) {
    return strdup(keys);
  }

  ++lastDot;

  if ((end - lastDot) <= 0) {
    return 0;
  }

  char *newkeys = malloc((end - lastDot) + 1);
  memset(newkeys, 0, (end - lastDot) + 1);
  strncpy(newkeys, lastDot, end - lastDot);
  return newkeys;
}

char *allButLast(const char *keys) {
  if (!keys) {
    return 0;
  }

  int len = strlen(keys);
  const char *last = keys + len;
  while (*last != '.' && last > keys) {
    --last;
  }

  if (keys == last) {
    return 0;
  }

  char *newkeys = malloc((last - keys) + 1);
  memset(newkeys, 0, (last - keys) + 1);
  strncpy(newkeys, keys, last - keys);
  return newkeys;
}

char *getOrCacheString(const char* value) {
  if(stringCache == NULL) {
    stringCache = jsonNewObject();
    if(atexit(signalHandler) != 0) {
      fprintf(stderr, "WARNING: Could not register cleanup function!");
    }
  }
  short type;
  if(stringCache != NULL) {
    char* cached = NULL;
    cached = _jsonGetObjVal(stringCache, value, &type).string_val;
    if(cached == NULL) {
      const char* duped = strdup(value);
      cached = duped;
      jsonAddVal(stringCache, cached, (JItemValue) { cached }, VAL_STRING);
      return cached;
    }else{
      return cached;
    }
  }
  return strdup(value);
}

JObject *jsonNewObject() {
  JObject *obj = malloc(sizeof(JObject));
  obj->_incrementSize = DEFAULT_INC_AMOUNT;
  obj->_arraySize = DEFAULT_HASH_SIZE;
  obj->_maxProbes = obj->_arraySize / 5;
  obj->size = 0;
  obj->entries = malloc(sizeof(JEntry*) * obj->_arraySize);
  memset(obj->entries, 0, sizeof(JEntry*) * obj->_arraySize);
  return obj;
}

JArray* jsonNewArray() {
  JArray *arr = malloc(sizeof(JArray));
  arr->type = VAL_MIXED_ARRAY;
  arr->count = 0;
  arr->_internal.vItems = NULL;
  return arr;
}

JArray* jsonAddArrayItem(JArray *arr, JArrayItem *item) {
  JArrayItem **items = malloc(sizeof(JArrayItem*)*(arr->count+1));
  JArrayItem **fromArray = arr->_internal.vItems;
  unsigned i = 0;
  for(; i < arr->count; ++i) {
    items[i] = fromArray[i];
  }
  items[i] = item;
  if(arr->_internal.vItems) {
    free(arr->_internal.vItems);
  }
  arr->_internal.vItems = items;
  ++arr->count;
  return arr;
}

JArray* jsonAddArrayItemObject(JArray *arr, JObject *obj) {
  JArrayItem *item = malloc(sizeof(JArrayItem));
  item->type = VAL_OBJ;
  item->value = (JItemValue) { obj };
  jsonAddArrayItem(arr, item);
  return arr;
}

int jsonInt(const JObject *obj, const char* keys) {
  short type = 0;
  JItemValue val = jsonGet(obj, keys, &type);

  if (type == VAL_INT) {
    return val.int_val;
  }
  return -1;
}

unsigned int jsonUInt(const JObject *obj, const char* keys) {
  short type = 0;
  JItemValue val = jsonGet(obj, keys, &type);
  if (val.ptr_val && type == VAL_UINT) {
    return val.int_val;
  }
  return -1;
}

float jsonFloat(const JObject *obj, const char* keys) {
  short type = 0;
  JItemValue val = jsonGet(obj, keys, &type);
  if (val.ptr_val && type == VAL_FLOAT) {
    return val.float_val;
  }
  return -1.0f;
}

double jsonDouble(const JObject *obj, const char* keys) {
  short type = 0;
  JItemValue val = jsonGet(obj, keys, &type);
  if (val.ptr_val && type == VAL_DOUBLE) {
    return val.double_val;
  }
  return -1.0;
}

char* jsonString(const JObject *obj, const char* keys) {
  if (keys == NULL || obj == NULL) {
    return NULL;
  }
  char *firstKeys = allButLast(keys);
  const JObject *parentObj = obj;

  if (firstKeys != NULL) {
    parentObj = jsonObject(obj, firstKeys);
    free(firstKeys);
  }

  if (parentObj != NULL) {
    char *lastKey = last(keys);
    if (lastKey != NULL) {
      short type = 0;
      JItemValue val = jsonGet(parentObj, lastKey, &type);
      free(lastKey);
      if (val.string_val && type == VAL_STRING) {
        return val.string_val;
      }
    }
  }

  return NULL;
}

char jsonBool(const JObject* obj, const char* keys) {
  if (keys == NULL) {
    return -1;
  }
  char *firstKeys = allButLast(keys);
  const JObject *parentObj = obj;

  if (firstKeys != NULL) {
    parentObj = jsonObject(obj, firstKeys);
    free(firstKeys);
  }

  if (parentObj != NULL) {
    char *lastKey = last(keys);
    if (lastKey != NULL) {
      short type = 0;
      JItemValue val = jsonGet(parentObj, lastKey, &type);
      free(lastKey);
      if (type == VAL_BOOL) {
        return val.char_val;
      }
    }
  }

  return -1;
}

char* jsonBoolArray(const JObject *obj, const char* keys) {
  short type = 0;
  JArray *val = jsonGet(obj, keys, &type).array_val;
  if (val && type == VAL_BOOL_ARRAY && val->count > 0) {
    JItemValue *items = val->_internal.items;
    char *ret = malloc(val->count);
    for(int i = 0; i < val->count; ++i) {
      ret[i] = (&items[i])->char_val;
    }
    return ret;
  }
  return NULL;
}

JArray* jsonArray(const JObject* obj, const char* keys) {
  short type;
  JItemValue value = jsonGet(obj, keys, &type);
  if(value.array_val && (type == VAL_MIXED_ARRAY || type == VAL_OBJ_ARRAY || type == VAL_INT_ARRAY ||
      type == VAL_DOUBLE_ARRAY || type == VAL_FLOAT_ARRAY || type == VAL_BOOL_ARRAY)) {
    return value.array_val;
  }
  return NULL;
}

int** jsonIntArray(const JObject* obj, const char* keys) {
  return NULL;
}

float** jsonFoatArray(const JObject* obj, const char* keys) {
  return NULL;
}

double** jsonDoubleArray(const JObject* obj, const char* keys) {
  return NULL;
}

char** jsonStringArray(const JObject* obj, const char* keys) {
  return NULL;
}

JArrayItem **jsonArrayItemList(JArray *array) {
  return array->_internal.vItems;
}

JObject **jsonArrayKeyFilter(JArray* array, const char *key, unsigned *size) {
  JArrayItem **items = jsonArrayItemList(array);
  JObject** toRet = NULL;
  JObject* found[256];
  memset(&found, 0, sizeof(JObject*)*256);
  int count = 0;
  unsigned totalFound = 0;
  for (int i = 0; i < array->count; ++i) {
    JArrayItem *item = items[i];
    if (item->type == VAL_OBJ) {
      JObject *toEval = item->value.object_val;
      if(jsonObject(toEval, key)) {
        found[count] = toEval;
        ++count;
        if(count >= 256) {
          if(!toRet) {
            delete[] toRet;
          }
          toRet = (JObject**)malloc(sizeof(JObject*)*count);
          memcpy(toRet, &found, sizeof(JObject*));
          memset(&found, 0, sizeof(JObject*)*count);
          totalFound += count;
          count = 0;
        }
      }
    }
  }
  if(!toRet) {
    delete[] toRet;
  }
  toRet = (JObject**)malloc(sizeof(JObject*)*count);
  memcpy(toRet, &found, sizeof(JObject*));
  memset(&found, 0, sizeof(JObject*)*count);
  totalFound += count;
  *size = count;
  return toRet;
}

JObject* jsonObject(const JObject* obj, const char* keys) {
  if (obj == 0 || !keys) {
    return 0;
  }

  int index = 0;
  short type = 0;
  char *key = nextKey(keys, &index);
  JItemValue jval = jsonGet(obj, key, &type);
  if (!jval.ptr_val || type != VAL_OBJ) {
    free(key);
    return 0;
  }

  JObject *found = jval.object_val;
  while (index > -1 && found != 0) {
    free(key);
    key = nextKey(keys, &index);
    printf("Looking for key %s\n", key);
    jval = jsonGet(found, key, &type);
    if (jval.ptr_val && type == VAL_OBJ) {
      found = jval.object_val;
    } else if (key) {
      if (jval.ptr_val) {
        printf("Err: Key %s was the wrong type.\n", key);
      } else {
        printf("Err: Object did not contains a next object %s\n", key);
      }
      return 0;
    }
  }
  free(key);
  return found;
}

void jsonPrintEntry(const FILE *io, unsigned char type, JItemValue* value, unsigned int tabs, unsigned int tabInc) {
  if (type == VAL_INT) {
    fprintf(io, "%d", value->int_val);
  } else if (type == VAL_FLOAT) {
    fprintf(io, "%f", value->float_val);
  } else if (type == VAL_DOUBLE) {
    fprintf(io, "%e", value->double_val);
  } else if (type == VAL_STRING) {
    fprintf(io, "\"%s\"", value->string_val);
  } else if (type == VAL_BOOL) {
    if (value->char_val) {
      fprintf(io, "%s", "true");
    } else {
      fprintf(io, "%s", "false");
    }
  } else if (type == VAL_BOOL_ARRAY) {
    fprintf(io, "[");
    JItemValue *bools = value->array_val->_internal.items;
    for(int i = 0; i < value->array_val->count; ++i) {
      JItemValue *boolVal = &bools[i];
      jsonPrintEntry(io, VAL_BOOL, boolVal, tabs, tabInc);
      if(i != value->array_val->count-1) {
        fprintf(io, ",");
      }
    }
    fprintf(io, "]");
  } else if (type == VAL_INT_ARRAY) {
    fprintf(io, "[");
    JItemValue *ints = value->array_val->_internal.items;
    for(int i = 0; i < value->array_val->count; ++i) {
      JItemValue *intVal = &ints[i];
      jsonPrintEntry(io, VAL_INT, intVal, tabs, tabInc);
      if(i != value->array_val->count-1) {
        fprintf(io, ",");
      }
    }
    fprintf(io, "]");
  } else if (type == VAL_FLOAT_ARRAY) {
    fprintf(io, "[");
    JItemValue *floats = value->array_val->_internal.items;
    for(int i = 0; i < value->array_val->count; ++i) {
      JItemValue *floatVal = &floats[i];
      jsonPrintEntry(io, VAL_FLOAT, floatVal, tabs, tabInc);
      if(i != value->array_val->count-1) {
        fprintf(io, ",");
      }
    }
    fprintf(io, "]");
  } else if (type == VAL_DOUBLE_ARRAY) {
    fprintf(io, "[");
    JItemValue *doubles = value->array_val->_internal.items;
    for(int i = 0; i < value->array_val->count; ++i) {
      JItemValue *doubleVal = &doubles[i];
      jsonPrintEntry(io, VAL_DOUBLE, doubleVal, tabs, tabInc);
      if(i != value->array_val->count-1) {
        fprintf(io, ",");
      }
    }
    fprintf(io, "]");
  } else if (type == VAL_MIXED_ARRAY) {
    fprintf(io, "[");
    JArrayItem **values = value->array_val->_internal.vItems;
    for(int i = 0; i < value->array_val->count; ++i) {
      JArrayItem *val = values[i];
      jsonPrintEntry(io, val->type, &val->value, tabs, tabInc);
      if(i != value->array_val->count-1) {
        fprintf(io, ",");
      }
    }
    fprintf(io, "]");
  } else if (type == VAL_OBJ) {
    jsonPrintObjectTabs(io, value->object_val, tabs, tabInc);
  }
}

void jsonPrintEntry(const FILE *io, const unsigned short type, const JItemValue *value) {
  jsonPrintEntry(io, type, value, 0, 2);
}

void jsonPrintObjectTabs(const FILE *io, const JObject* obj, unsigned int tabs, unsigned int tabInc) {
  tabs+=tabInc;
  char strTabs[tabs+1];
  memset(strTabs, ' ', tabs+1);
  strTabs[tabs] = '\0';

  fprintf(io, "{\n");
  unsigned char type = -1;
  JEntry* entry;

  int count = 0;
  for(int i = 0; i < obj->_arraySize; ++i) {
    entry = obj->entries[i];
    if(entry) {
      ++count;
      type = entry->value_type;
      char *comma = ",";
      if(count == obj->size) {
        comma = ""; //last element
      }
      fprintf(io, "%s\"%s\": ", strTabs, entry->name);
      jsonPrintEntry(io, entry->value_type, &entry->value, tabs, tabInc);
      fprintf(io, "%s\n", comma);
    }
  }
  strTabs[tabs-tabInc] = '\0';
  fprintf(io, "%s}", strTabs);
}

void jsonPrintObject(const FILE *io, const JObject* obj) {
  jsonPrintObjectTabs(io, obj, 0, 2);
}

void jsonFree(JItemValue val, const short vtype) {
  if (!val.ptr_val) {
    return;
  }

  if (vtype == VAL_OBJ) {
    JObject *obj = val.object_val;
    if(obj == stringCache) {
      return;
    }
    JEntry *toDel = NULL;
    JItemValue valToDel = { 0 };
    for (int i = 0; i < obj->_arraySize; ++i) {
      if (obj->entries[i] != NULL) {
        toDel = obj->entries[i];
        valToDel = toDel->value;

        jsonFree(valToDel, toDel->value_type);
        //free(toDel->name);
        free(toDel);
      }
    }
    free(obj->entries);
  } else if (vtype == VAL_MIXED_ARRAY || vtype == VAL_OBJ_ARRAY) {
    struct JArray *arr = val.array_val;
    int count = arr->count;
     for (int i = 0; i < count; ++i) {
      JArrayItem *items = arr->_internal.vItems[i];

      jsonFree(items->value, items->type);
    }
  } else if (vtype == VAL_INT_ARRAY || vtype == VAL_FLOAT_ARRAY
      || vtype == VAL_BOOL_ARRAY || vtype == VAL_DOUBLE_ARRAY) {
    JArray* arr = val.array_val;
    free(arr->_internal.items);
    free(arr);
  } else if (vtype == VAL_STRING_ARRAY) {
    JArray *arr = val.array_val;
    free(arr->_internal.items);
  }

  if(vtype != VAL_INT && vtype != VAL_NULL && vtype != VAL_BOOL
      && vtype != VAL_DOUBLE && vtype != VAL_FLOAT && vtype != VAL_UINT &&
      vtype != VAL_STRING) {
    //printf("Could not free type\n");
    //free(val);
  }
  if(vtype != 0 && vtype != VAL_FLOAT && vtype != VAL_DOUBLE && vtype != VAL_NULL &&
      vtype != VAL_BOOL && vtype != VAL_INT && vtype != VAL_STRING &&
      vtype != VAL_BOOL_ARRAY && vtype != VAL_DOUBLE_ARRAY &&
      vtype != VAL_FLOAT_ARRAY && vtype != VAL_INT_ARRAY) {
    free(val.ptr_val);
  }
}

