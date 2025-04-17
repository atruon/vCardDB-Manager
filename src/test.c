#include "VCParser.h"
#include "VCHelper.h"

int main() {
    Card * test;
    char * fName = "./bin/cards/testCardNoVersion.vcf";
    VCardErrorCode errCode = createCard(fName, &test);
    if (errCode != OK) {
        char * errString = errorToString(errCode);
        printf("%s\n", errString);
        free(errString);
        exit(0);
    }
 
    char * dataString = cardToString(test);
    printf("%s\n", dataString);
 
    errCode = validateCard(test);
    if (errCode != OK) {
        char * errString = errorToString(errCode);
        printf("%s\n", errString);
        free(errString);
    }
    free(dataString);
    errCode = writeCard("testing.vcf", test);
    if (errCode != OK) {
        char * errString = errorToString(errCode);
        printf("%s\n", errString);
        free(errString);
        deleteCard(test);
        exit(0);
    }
   
    deleteCard(test);
    printf("---------------------------------------------------------------\n");

    test = malloc(sizeof(Card));
    test->fn = malloc(sizeof(Property));
    void * tempVar = initializeList(propertyToString, deleteProperty, compareProperties);
    test->optionalProperties = (List *)tempVar;  
    test->anniversary = NULL;        
    test->birthday = NULL;
    test->fn->name = malloc(3);
    strcpy(test->fn->name, "FN");
    test->fn->group = emptyString();
    test->fn->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    test->fn->values = initializeList(valueToString, deleteValue, compareValues);
    dataString = malloc(50);
    strcpy(dataString, "Alberto Alfonzo");
    insertFront(test->fn->values, dataString);

    Property * tempProp = malloc(sizeof(Property));
    tempProp->name = malloc(3);
    strcpy(tempProp->name, "FN");
    tempProp->group = emptyString();
    tempProp->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    tempProp->values = initializeList(valueToString, deleteValue, compareValues);
    dataString = malloc(50);
    strcpy(dataString, "Alberto Alfonzo");
    insertFront(tempProp->values, dataString);
    insertFront(test->optionalProperties, tempProp);

    tempProp = malloc(sizeof(Property));
    tempProp->name = malloc(50);
    strcpy(tempProp->name, "REV");
    tempProp->group = emptyString();
    tempProp->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    tempProp->values = initializeList(valueToString, deleteValue, compareValues);
    dataString = malloc(50);
    strcpy(dataString, "TESTINGSTRING");
    insertFront(tempProp->values, dataString);
    insertFront(test->optionalProperties, tempProp);
    
    // tempProp = malloc(sizeof(Property));
    // tempProp->name = malloc(50);
    // strcpy(tempProp->name, "REV");
    // tempProp->group = emptyString();
    // tempProp->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    // tempProp->values = initializeList(valueToString, deleteValue, compareValues);
    // dataString = malloc(50);
    // strcpy(dataString, "TESTINGSTRING");
    // insertFront(tempProp->values, dataString);
    // insertFront(test->optionalProperties, tempProp);

    
    errCode = validateCard(test);
    if (errCode != OK) {
        char * errString = errorToString(errCode);
        printf("%s\n", errString);
        free(errString);
        deleteCard(test);
        exit(0);
    }

    dataString = cardToString(test);
    printf("%s\n", dataString);
    deleteCard(test);

}