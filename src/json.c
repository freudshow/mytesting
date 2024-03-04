#include <stdio.h>
#include <json-c/json.h>

void analyze_json_object(struct json_object *json_obj, const char *key, int depth)
{
    char deep[100] = { '\0' };
    int i = 0;
    for (i = 0; i < depth; i++)
    {
        strcat(deep, "\t");
    }

    enum json_type type = json_object_get_type(json_obj);

    printf("%sKey: %s\n", deep, key);

    switch (type)
    {
        case json_type_null:
            printf("%sType: Null\n", deep);
            break;
        case json_type_boolean:
            printf("%sType: Boolean\n", deep);
            printf("%sValue: %s\n", deep, json_object_get_boolean(json_obj) ? "true" : "false");
            break;
        case json_type_double:
            printf("%sType: Double\n", deep);
            printf("%sValue: %f\n", deep, json_object_get_double(json_obj));
            break;
        case json_type_int:
            printf("%sType: Int\n", deep);
            printf("%sValue: %d\n", deep, json_object_get_int(json_obj));
            break;
        case json_type_object:
            printf("Type: Object\n");
            json_object_object_foreach(json_obj, key, val)
            {
                analyze_json_object(val, key);
            }
            break;
        case json_type_array:
            printf("Type: Array\n");
            int array_len = json_object_array_length(json_obj);
            for (int i = 0; i < array_len; i++)
            {
                struct json_object *array_val = json_object_array_get_idx(json_obj, i);
                analyze_json_object(array_val, NULL);
            }
            break;
        case json_type_string:
            printf("Type: String\n");
            printf("Value: %s\n", json_object_get_string(json_obj));
            break;
        default:
            printf("Type: Unknown\n");
            break;
    }
}

void testjson(void)
{
    struct json_object *json_obj = json_object_from_file("/home/floyd/repo/mytesting/Debug/data.json");
    if(json_obj == NULL)
    {
        perror("file not exists or json format error");
        return;
    }

    json_object_object_foreach(json_obj, key, val)
    {
        analyze_json_object(val, key);
    }

    json_object_put(json_obj); // Free the json object
}
