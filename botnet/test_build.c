#include <stdio.h>
#include <json-c/json.h>

int main()
{
    // Create json object
    json_object *jasonObject = json_object_new_object();

    json_object *objectType = json_object_new_string("outputs");


    json_object *propertiesObject = json_object_new_object();

    json_object *hostName = json_object_new_string("Bobo");
    json_object *cmdName = json_object_new_string("ls");
    json_object *time = json_object_new_string("12:12 PM");
    json_object *date = json_object_new_string("22/02/2020");
    json_object *ipAddr = json_object_new_string("127.0.0.1");

    json_object_object_add(propertiesObject, "hostname", hostName);
    json_object_object_add(propertiesObject, "input command", cmdName);
    json_object_object_add(propertiesObject, "time", time);
    json_object_object_add(propertiesObject, "date", date);
    json_object_object_add(propertiesObject, "ip address", ipAddr);


    json_object *returnedObject = json_object_new_object();

    json_object *results = json_object_new_string("client.c server.c ...");

    json_object_object_add(returnedObject, "results", results);

    json_object_object_add(jasonObject,"type", objectType);
    json_object_object_add(jasonObject,"properties", propertiesObject);
    json_object_object_add(jasonObject,"returned", returnedObject);

    // Printing the json object
    printf ("The json object created: %s\n",json_object_to_json_string(jasonObject));
}