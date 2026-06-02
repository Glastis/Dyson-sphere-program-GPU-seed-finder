#ifndef DSP_JSON_VALUE_H
#define DSP_JSON_VALUE_H

enum json_kind
{
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
};

typedef struct json_value json_value;

typedef struct
{
    char *key;
    json_value *value;
}
json_member;

struct json_value
{
    int kind;
    int bool_value;
    double number_value;
    char *string_value;
    json_value **items;
    int item_count;
    json_member *members;
    int member_count;
};

json_value *json_parse(const char *text);
void json_free(json_value *value);

const json_value *json_object_get(const json_value *object, const char *key);
int json_array_size(const json_value *array);
const json_value *json_array_get(const json_value *array, int index);

int json_is_object(const json_value *value);
int json_is_array(const json_value *value);
int json_is_number(const json_value *value);
int json_is_string(const json_value *value);
int json_is_bool(const json_value *value);

double json_as_number(const json_value *value);
const char *json_as_string(const json_value *value);
int json_as_bool(const json_value *value);

#endif
