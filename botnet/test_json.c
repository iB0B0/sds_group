#include <stdio.h>
#include <json-c/json.h>

int main()
{
    // Create json object
    json_object *jobj = json_object_new_object();
    json_object *properties = json_object_new_object();

    // Create json string
    json_object *jstring = json_object_new_string("We love to JSON");

    // Create json int
    json_object *jint = json_object_new_int(10);

    // Create json bool
    json_object *jbool = json_object_new_boolean(1);

    // Create json double
    json_object *jdouble = json_object_new_double(2.14);

    // Create json array
    json_object *jarray = json_object_new_array();
    json_object *jarray2 = json_object_new_array();

    json_object *jarray3 = json_object_new_array();
    json_object *jarray4 = json_object_new_array();

    // Create json strings
    json_object *jstring1 = json_object_new_string("A");
    json_object *jstring2 = json_object_new_string("B");
    //json_object *jstring3 = json_object_new_string("C");

    // Add items to array
    json_object_array_add(jarray, jstring1);
    json_object_array_add(jarray, jstring2);
    json_object_array_add(jarray, jarray2);

    // Add items to array
    json_object_array_add(jarray3, jstring1);
    json_object_array_add(jarray4, jstring2);
    json_object_array_add(jarray3, jarray4);

    json_object_object_add(properties,"properties test array", jarray3);

    // Form json object
    // Each is key value pair
    json_object_object_add(jobj,"type", jstring);
    json_object_object_add(jobj,"test boolean", jbool);
    json_object_object_add(jobj,"test double", jdouble);
    json_object_object_add(jobj,"test int", jint);
    json_object_object_add(jobj,"test array", jarray);

    json_object_object_add(jobj,"test array", properties);

    // Printing the json object
    printf ("The json object created: %sn",json_object_to_json_string(jobj));
}