#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "json_value.h"

typedef struct
{
    const char *cursor;
    int has_error;
}
json_parser;

static json_value *parse_value(json_parser *parser);

static json_value *alloc_value(int kind)
{
    json_value *value;

    value = calloc(1, sizeof(json_value));
    if (value == NULL)
    {
        return NULL;
    }
    value->kind = kind;
    return value;
}

static void skip_whitespace(json_parser *parser)
{
    while (*parser->cursor == ' ' || *parser->cursor == '\t'
           || *parser->cursor == '\n' || *parser->cursor == '\r')
    {
        ++parser->cursor;
    }
}

static int parse_hex4(const char *cursor, unsigned int *out)
{
    unsigned int value;
    int index;

    value = 0;
    index = 0;
    while (index < 4)
    {
        char c;

        c = cursor[index];
        value <<= 4;
        if (c >= '0' && c <= '9')
        {
            value |= (unsigned int)(c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            value |= (unsigned int)(c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            value |= (unsigned int)(c - 'A' + 10);
        }
        else
        {
            return 0;
        }
        ++index;
    }
    *out = value;
    return 1;
}

static int encode_utf8(unsigned int code, char *out)
{
    if (code < 0x80)
    {
        out[0] = (char)code;
        return 1;
    }
    if (code < 0x800)
    {
        out[0] = (char)(0xC0 | (code >> 6));
        out[1] = (char)(0x80 | (code & 0x3F));
        return 2;
    }
    out[0] = (char)(0xE0 | (code >> 12));
    out[1] = (char)(0x80 | ((code >> 6) & 0x3F));
    out[2] = (char)(0x80 | (code & 0x3F));
    return 3;
}

static int append_unicode(json_parser *parser, char *buffer, int *length)
{
    unsigned int code;

    if (!parse_hex4(parser->cursor, &code))
    {
        parser->has_error = 1;
        return 0;
    }
    parser->cursor += 4;
    *length += encode_utf8(code, buffer + *length);
    return 1;
}

static const char ESCAPE_FROM[] = "\"\\/bfnrt";
static const char ESCAPE_TO[] = "\"\\/\b\f\n\r\t";

static char escape_literal(char c)
{
    int index;

    index = 0;
    while (ESCAPE_FROM[index] != '\0')
    {
        if (ESCAPE_FROM[index] == c)
        {
            return ESCAPE_TO[index];
        }
        ++index;
    }
    return '\0';
}

static int parse_escape(json_parser *parser, char *buffer, int *length)
{
    char c;
    char literal;

    c = *parser->cursor;
    ++parser->cursor;
    if (c == 'u')
    {
        return append_unicode(parser, buffer, length);
    }
    literal = escape_literal(c);
    if (literal == '\0')
    {
        parser->has_error = 1;
        return 0;
    }
    buffer[(*length)++] = literal;
    return 1;
}

static int parse_string_char(json_parser *parser, char *buffer, int *length)
{
    char c;

    c = *parser->cursor;
    if (c == '\0')
    {
        parser->has_error = 1;
        return -1;
    }
    if (c == '\\')
    {
        ++parser->cursor;
        if (!parse_escape(parser, buffer, length))
        {
            return -1;
        }
        return 1;
    }
    buffer[(*length)++] = c;
    ++parser->cursor;
    return 1;
}

static char *parse_raw_string(json_parser *parser)
{
    char *buffer;
    int length;

    if (*parser->cursor != '"')
    {
        parser->has_error = 1;
        return NULL;
    }
    ++parser->cursor;
    buffer = malloc(strlen(parser->cursor) + 1);
    if (buffer == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }
    length = 0;
    while (*parser->cursor != '"')
    {
        if (parse_string_char(parser, buffer, &length) < 0)
        {
            free(buffer);
            return NULL;
        }
    }
    ++parser->cursor;
    buffer[length] = '\0';
    return buffer;
}

static json_value *parse_string(json_parser *parser)
{
    json_value *value;
    char *raw;

    raw = parse_raw_string(parser);
    if (raw == NULL)
    {
        return NULL;
    }
    value = alloc_value(JSON_STRING);
    if (value == NULL)
    {
        parser->has_error = 1;
        free(raw);
        return NULL;
    }
    value->string_value = raw;
    return value;
}

static int is_number_char(char c)
{
    return (c >= '0' && c <= '9') || c == '-' || c == '+'
           || c == '.' || c == 'e' || c == 'E';
}

static json_value *parse_number(json_parser *parser)
{
    json_value *value;
    char *end;
    double number;

    end = NULL;
    number = strtod(parser->cursor, &end);
    if (end == parser->cursor)
    {
        parser->has_error = 1;
        return NULL;
    }
    parser->cursor = end;
    value = alloc_value(JSON_NUMBER);
    if (value == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }
    value->number_value = number;
    return value;
}

static json_value *parse_literal(json_parser *parser, const char *word, int kind, int bool_value)
{
    json_value *value;
    size_t length;

    length = strlen(word);
    if (strncmp(parser->cursor, word, length) != 0)
    {
        parser->has_error = 1;
        return NULL;
    }
    parser->cursor += length;
    value = alloc_value(kind);
    if (value == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }
    value->bool_value = bool_value;
    return value;
}

static int append_item(json_value *array, json_value *item)
{
    json_value **grown;
    int new_count;

    new_count = array->item_count + 1;
    grown = realloc(array->items, (size_t)new_count * sizeof(json_value *));
    if (grown == NULL)
    {
        return 0;
    }
    array->items = grown;
    array->items[array->item_count] = item;
    array->item_count = new_count;
    return 1;
}

static int parse_array_item(json_parser *parser, json_value *array)
{
    json_value *item;

    skip_whitespace(parser);
    item = parse_value(parser);
    if (item == NULL || !append_item(array, item))
    {
        json_free(item);
        parser->has_error = 1;
        return -1;
    }
    skip_whitespace(parser);
    if (*parser->cursor == ',')
    {
        ++parser->cursor;
        return 1;
    }
    if (*parser->cursor == ']')
    {
        ++parser->cursor;
        return 0;
    }
    parser->has_error = 1;
    return -1;
}

static json_value *parse_array(json_parser *parser)
{
    json_value *array;
    int status;

    array = alloc_value(JSON_ARRAY);
    if (array == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }
    ++parser->cursor;
    skip_whitespace(parser);
    if (*parser->cursor == ']')
    {
        ++parser->cursor;
        return array;
    }
    status = 1;
    while (status == 1)
    {
        status = parse_array_item(parser, array);
    }
    if (status < 0)
    {
        json_free(array);
        return NULL;
    }
    return array;
}

static int append_member(json_value *object, char *key, json_value *value)
{
    json_member *grown;
    int new_count;

    new_count = object->member_count + 1;
    grown = realloc(object->members, (size_t)new_count * sizeof(json_member));
    if (grown == NULL)
    {
        return 0;
    }
    object->members = grown;
    object->members[object->member_count].key = key;
    object->members[object->member_count].value = value;
    object->member_count = new_count;
    return 1;
}

static int parse_member(json_parser *parser, json_value *object)
{
    char *key;
    json_value *value;

    skip_whitespace(parser);
    key = parse_raw_string(parser);
    if (key == NULL)
    {
        return 0;
    }
    skip_whitespace(parser);
    if (*parser->cursor != ':')
    {
        parser->has_error = 1;
        free(key);
        return 0;
    }
    ++parser->cursor;
    skip_whitespace(parser);
    value = parse_value(parser);
    if (value == NULL || !append_member(object, key, value))
    {
        parser->has_error = 1;
        free(key);
        json_free(value);
        return 0;
    }
    return 1;
}

static json_value *parse_object(json_parser *parser)
{
    json_value *object;

    object = alloc_value(JSON_OBJECT);
    if (object == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }
    ++parser->cursor;
    skip_whitespace(parser);
    if (*parser->cursor == '}')
    {
        ++parser->cursor;
        return object;
    }
    while (1)
    {
        if (!parse_member(parser, object))
        {
            json_free(object);
            return NULL;
        }
        skip_whitespace(parser);
        if (*parser->cursor == ',')
        {
            ++parser->cursor;
            continue;
        }
        if (*parser->cursor == '}')
        {
            ++parser->cursor;
            return object;
        }
        parser->has_error = 1;
        json_free(object);
        return NULL;
    }
}

static json_value *parse_value(json_parser *parser)
{
    char c;

    skip_whitespace(parser);
    c = *parser->cursor;
    if (c == '{')
    {
        return parse_object(parser);
    }
    if (c == '[')
    {
        return parse_array(parser);
    }
    if (c == '"')
    {
        return parse_string(parser);
    }
    if (c == 't')
    {
        return parse_literal(parser, "true", JSON_BOOL, 1);
    }
    if (c == 'f')
    {
        return parse_literal(parser, "false", JSON_BOOL, 0);
    }
    if (c == 'n')
    {
        return parse_literal(parser, "null", JSON_NULL, 0);
    }
    if (is_number_char(c))
    {
        return parse_number(parser);
    }
    parser->has_error = 1;
    return NULL;
}

json_value *json_parse(const char *text)
{
    json_parser parser;
    json_value *root;

    if (text == NULL)
    {
        return NULL;
    }
    parser.cursor = text;
    parser.has_error = 0;
    root = parse_value(&parser);
    if (root == NULL || parser.has_error)
    {
        json_free(root);
        return NULL;
    }
    skip_whitespace(&parser);
    if (*parser.cursor != '\0')
    {
        json_free(root);
        return NULL;
    }
    return root;
}

static void free_members(json_value *value)
{
    int index;

    index = 0;
    while (index < value->member_count)
    {
        free(value->members[index].key);
        json_free(value->members[index].value);
        ++index;
    }
    free(value->members);
}

static void free_items(json_value *value)
{
    int index;

    index = 0;
    while (index < value->item_count)
    {
        json_free(value->items[index]);
        ++index;
    }
    free(value->items);
}

void json_free(json_value *value)
{
    if (value == NULL)
    {
        return;
    }
    if (value->kind == JSON_OBJECT)
    {
        free_members(value);
    }
    else if (value->kind == JSON_ARRAY)
    {
        free_items(value);
    }
    else if (value->kind == JSON_STRING)
    {
        free(value->string_value);
    }
    free(value);
}

const json_value *json_object_get(const json_value *object, const char *key)
{
    int index;

    if (object == NULL || object->kind != JSON_OBJECT)
    {
        return NULL;
    }
    index = 0;
    while (index < object->member_count)
    {
        if (strcmp(object->members[index].key, key) == 0)
        {
            return object->members[index].value;
        }
        ++index;
    }
    return NULL;
}

int json_array_size(const json_value *array)
{
    if (array == NULL || array->kind != JSON_ARRAY)
    {
        return 0;
    }
    return array->item_count;
}

const json_value *json_array_get(const json_value *array, int index)
{
    if (array == NULL || array->kind != JSON_ARRAY)
    {
        return NULL;
    }
    if (index < 0 || index >= array->item_count)
    {
        return NULL;
    }
    return array->items[index];
}

int json_is_object(const json_value *value)
{
    return value != NULL && value->kind == JSON_OBJECT;
}

int json_is_array(const json_value *value)
{
    return value != NULL && value->kind == JSON_ARRAY;
}

int json_is_number(const json_value *value)
{
    return value != NULL && value->kind == JSON_NUMBER;
}

int json_is_string(const json_value *value)
{
    return value != NULL && value->kind == JSON_STRING;
}

int json_is_bool(const json_value *value)
{
    return value != NULL && value->kind == JSON_BOOL;
}

double json_as_number(const json_value *value)
{
    if (value == NULL || value->kind != JSON_NUMBER)
    {
        return 0.0;
    }
    return value->number_value;
}

const char *json_as_string(const json_value *value)
{
    if (value == NULL || value->kind != JSON_STRING)
    {
        return NULL;
    }
    return value->string_value;
}

int json_as_bool(const json_value *value)
{
    if (value == NULL || value->kind != JSON_BOOL)
    {
        return 0;
    }
    return value->bool_value;
}
