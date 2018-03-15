
#include "json.h"

struct {
	struct JsonEntry **entries;
} JsonObject ;

struct {
	const char* name;
	struct JsonObject *data;
} JsonEntry;

struct {

} JsonArray;
