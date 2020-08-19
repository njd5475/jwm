/*
 * HashMap.h
 *
 *  Created on: Aug 11, 2020
 *      Author: nick
 */

#ifndef SRC_HASHMAP_H_
#define SRC_HASHMAP_H_

#include <vector>
#include "json.h"

template<class T> class HashMap {
public:

  class MapEntry {
  public:
    virtual ~MapEntry() {}

    void operator=(T value) {
      this->_value = value;
      jsonAddVal(_map, _key, {this}, VAL_STRING);
    }

    T value() {
      return _value;
    }

    const char* key() {
      return _key;
    }

    static MapEntry* createEntry(JObject* map, const char* key, T value) {
      return new MapEntry(map, key, value);
    }
  protected:
    MapEntry(JObject* map, const char* key, T value): _map(map), _key(key), _value(value) {
    }

  private:
    const char* _key;
    T _value;
    JObject* _map;
  };
private:
  MapEntry* _get(const char* key) {
    short type;
    Value entry = jsonGet(_map, key, &type);
    return (MapEntry*)entry.ptr_val;
  }

  void _put(const char* key, MapEntry* value) {
    jsonAddVal(_map, key, {value});
  }

public:
  HashMap() {
    this->_map = jsonNewObject();
  }
  virtual ~HashMap() {
    jsonFree(Value {this->_map}, VAL_OBJ);
  }

  bool has(const char* key) {
    MapEntry *entry = _get(key);
    return entry != NULL;
  }

  T get(const char* key) {
    auto entry = _get(key);
    return entry.value();
  }

  void put(const char* key, T value) {
    auto entry = HashMap<T>::MapEntry::createEntry(_map, key, value);
    entry = value;
  }

  std::vector<MapEntry*> entries() {
    std::vector<MapEntry*> entries;
    for(unsigned int i = 0; i < _map->_arraySize; ++i) {
      if(_map->entries[i]) {
        entries.push_back((MapEntry*)_map->entries[i]->value.ptr_val);
      }
    }
    return entries;
  }

  MapEntry& operator [](const char* key) {
    MapEntry *entry = _get(key);
    if(entry == NULL) {
      entry = HashMap<T>::MapEntry::createEntry(_map, key, NULL);
    }
    return *entry;
  }

private:

  JObject *_map;

};

#endif /* SRC_HASHMAP_H_ */
