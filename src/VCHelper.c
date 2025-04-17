/*
CIS*2750 A1
Submission By: Anson Truong (1102843)
Due: Feb 10th 2025
File name: VCHelper.c
*/

#include "VCHelper.h"
#include <stdio.h>
#include <ctype.h>


// Function stores all special characters and their index as a pair inside of the List myList
void specialCharIndexFinder(List *myList, char * s, int sLen) {
    int sIndex = 0;
    char myChar = ' ';
    Pair *tempPair;
    while (sIndex < sLen) {
        myChar = s[sIndex];
        if (myChar == ';' || myChar == ':' || myChar == '\n' || myChar == '\r') {
            tempPair = malloc(sizeof(Pair));
            tempPair->index = sIndex;
            tempPair->special = myChar;

            insertBack(myList, tempPair);
        }
        ++sIndex;
    }
}

// Returns a dynamically allocated string representation of the Pair given as item
char * pairToString(void * item) {
    char * result = malloc(sizeof(char) * 30);
    sprintf(result, "Index = %d, char = %c\n", ((Pair *)item)->index, ((Pair *)item)->special);
    return result;
}

// Returns 0 if two Pairs, item1, and item2, are identical, and -1 otherwise
int pairCompare(const void *item1, const void *item2) {
    if (((Pair *)item1)->index == ((Pair *)item2)->index && ((Pair *)item1)->special == ((Pair *)item2)->special) {
        return 0;
    }
    return -1;
}

// Returns a dynamically allocated substring of string s from index start to index end exclusive
//         Caller must call free
char * stringSplice(char *s, int start, int end) {
    if (start == end) {
        return emptyString();
    }
    int len = end - start + 1;
    char * newString = malloc(sizeof(char) * len);
    if (newString == NULL) {
        return NULL;
    }
    for (int i = 0; i < len - 1; ++i) {
        newString[i] = s[i + start];
    }
    newString[len - 1] = '\0';
    return newString;
}


// Returns the index of the character (=) between the parameter name and parameter value
int indexOfValue(char * s) {
    for (int i = 0; i < strlen(s); ++i) {
        if (s[i] == '=') {
            return i;
        }
    }
    return -1;              // -1 if = was not found
}

// Gets the index of the \r character by extracting it from the List myList
int getCRIndex(List *myList) {
    Node *temp = myList->tail;
    temp = temp->previous;
    Pair * tempPair = (Pair *) temp->data;
    if (tempPair->special == '\r') {
        return tempPair->index;
    }
    return -1;
}

// Checks if the string contains the character 'T', which denotes that the string contains a time value
bool containsTime(char * s) {
    for (int i = 0; i < strlen(s); ++i) {
        if (s[i] == 'T' || s[i] == 't') {
            return true;
        }
    }
    return false;
}

// Returna dynamically allocated empty string (Caller must free)
char * emptyString() {
    char * result = malloc(sizeof(char));
    result[0] = '\0';
    return result;
}

// Returns a dynamically allocated string that represents the parameter name when given s, 
//      Returns an empty string if the parameter format is invalid.
//      (Caller must free)
char * paramName (char * s) {
    if (indexOfValue(s) == -1) {
        return emptyString();
    }
    char * result = stringSplice(s, 0, indexOfValue(s));
    return result;
}

// Returns a dynamicalyl allocated string that represents the parameter value when given s.
//      Returns an empty string if the parameter format is invalid.
//      (Caller must free)
char * paramValue(char * s) {
    if (indexOfValue(s) == -1) {
        return emptyString();
    }
    char * result = stringSplice(s, indexOfValue(s) + 1, strlen(s));
    return result;
}

// Returns the pointer to the node of the next special character greater than index start, found by looking through the List myList that contains
//      index and character pairs
Node  * valueEndIndex (List *myList, int start) {
    Node * node = myList->head;
    while (node != NULL) {
        Pair * tempPair = (Pair *)node->data;
        if (tempPair->index >= start && (tempPair->special == ';' || tempPair->special == '\r')) {
            return node;
        }
        node = node->next;
    }
    return myList->tail->previous;
}

// Returns the index of where the equal sign(=) character is found in string s
int groupEndIndex (char * s) {
    for (int i = 0; i < strlen(s); ++i) {
        if (s[i] == '.') {
            return i;
        }
    }
    return -1;
}


// Returns true if a vCard line is valid, ie. ends with /r/n
bool validEnding (List *myList, char * s) {
    if (!myList && !s) {
        return false;
    }
    Node *tempNode = myList->tail;
    Pair * tempPair = (Pair *)tempNode->data;
    if (tempPair->special != '\n' && tempPair->index != strlen(s) - 1) {
        return false;
    }
    tempNode = tempNode->previous;
    tempPair = (Pair *)tempNode->data;
    if (tempPair->special != '\r' && tempPair->index != strlen(s) - 2) {
        return false;
    }
    return true;
}

// Returns true if the string s has a colon in it and false otherwise
bool validColon(char * s) {
    for (int i = 0; i < strlen(s); ++i) {
        if (s[i] == ':') {
            return true;
        }
    }
    return false;
}

// To be used for check if a List *values has a length of 0, return true is so, and false otherwise.
bool zeroValues(List * myList) {
    if (myList == NULL) {
        return true;
    }
    // char * valueInQuestion = (char *)myList->head->data;
    // // Returns true if a list contains only 1 value and it is the empty string "\0"
    // if (getLength(myList) == 1 && strcmp(valueInQuestion, "\0") == 0) {
    //     return true;
    // }
    void * elem;
    ListIterator iter = createIterator(myList);
    while ((elem = nextElement(&iter)) != NULL) {
        char * tempS = (char *)elem;
        // Returns false if List has more than one value or a single non-empty string value
        if (strlen(tempS) > 0) return false;
    }
    // Return true if List of values only contains empty strings
    return true;
}

// Function writes the given Property out to a file using the FILE pointer given as argument
void writeOptionalProperties(FILE *fptr, Property * prop) {
    if (strlen(prop->group) != 0) {
        fprintf(fptr, "%s.", prop->group);
    }
    fprintf(fptr, "%s", prop->name);
    void * elem;
    ListIterator paramIter = createIterator(prop->parameters);
    while ((elem = nextElement(&paramIter)) != NULL) {
        Parameter * tempParam = (Parameter *)elem;
        fprintf(fptr, ";%s=%s", tempParam->name, tempParam->value);
    }
    fprintf(fptr, ":");
    bool first = true;
    ListIterator valuesIter = createIterator(prop->values);
    while((elem = nextElement(&valuesIter)) != NULL) {
        char * value = (char *)elem;
        if (first) {
            fprintf(fptr, value);
            first = false;
        }
        else {
            fprintf(fptr, ";%s", value);
        }
    }
    fprintf(fptr, "\r\n");
}

// Function writes the give DateTime structure out to a file using the FILE pointer given as argument
void writeOutDates(FILE *fptr, int mode, DateTime * aDate) {
    if (mode == 0) {
        fprintf(fptr, "BDAY");
    }
    else {
        fprintf(fptr, "ANNIVERSARY");
    }
    if (aDate->isText) {
        fprintf(fptr, ";VALUE=TEXT:%s\r\n", aDate->text);
    }
    else {
        fprintf(fptr, ":");
        if (strlen(aDate->date) != 0) {
            fprintf(fptr, "%s", aDate->date);
        }
        if (strlen(aDate->time) != 0) {
            fprintf(fptr, "T%s", aDate->time);
        }
        if (aDate->UTC) {
            fprintf(fptr,"Z\r\n");
        }
        else {
            fprintf(fptr, "\r\n");
        }
    }
}

// Returns the length of the list values
int valuesLength(List * values) {
    void * elem;
    int total = 0;
    ListIterator iter = createIterator(values);
    while ((elem = nextElement(&iter)) != NULL) {
        ++total;
    }
    return total;
}

// Return the number of Property in the list optionalProperties with the name propName
int numPropOccurence(List * optionalProperties, char * propName) {
    int count = 0;
    void * elem;
    ListIterator iter = createIterator(optionalProperties);
    while ((elem = nextElement(&iter)) != NULL) {
        Property * readProp = (Property *)elem;
        char * readName = readProp->name;
        if (strcmp(propName, readName) == 0) {
            ++count;
        }
    }
    return count;
}

VCardErrorCode validateCardShape(const Card *obj) {
    if (!obj) return INV_CARD;
    if (!obj->fn) return INV_CARD;
    if (!obj->fn->group) return INV_CARD;
    if (valuesLength(obj->fn->values) != 1) return INV_CARD;
    if (!obj->optionalProperties) return INV_CARD;
    return OK;
}

VCardErrorCode validProperty (Property * tempProp, int propOccurence) {
    // Check for null pointers
    if (!tempProp) return INV_PROP;
    if (!tempProp->name) return INV_PROP;
    if (!tempProp->group) return INV_PROP;
    if (!tempProp->parameters) return INV_PROP;
    if (!tempProp->values) return INV_PROP;
    char * propName = tempProp->name;
    int numValues = valuesLength(tempProp->values);

    // Checks if all parameters are valid
    void * elem;
    ListIterator iter = createIterator(tempProp->parameters);
    while ((elem = nextElement(&iter)) != NULL) {
        Parameter * tempParam = (Parameter *)elem;
        if (!tempParam->name || !tempParam->value) return INV_PROP;
        if (strlen(tempParam->name) == 0 || strlen(tempParam->value) == 0) return INV_PROP;
    }
    if (zeroValues(tempProp->values)) return INV_PROP;

    // Check if the property name is valid and if the cardinality of values and property is correct
    if (strcmp("KIND", propName) == 0) {
        if (propOccurence > 1) return INV_PROP;
        if (numValues != 1) return INV_PROP;
        return OK;
    }
    else if (strcmp("FN", propName) == 0) {
        if (propOccurence != 1) return INV_CARD;
        if (numValues != 1) return INV_PROP;
        return OK;
    }
    else if (strcmp("N", propName) == 0) {
        if (propOccurence > 1) return INV_PROP;
        if (numValues != 5) return INV_PROP;
        return OK;
    }
    else if (strcmp("NICKNAME", propName) == 0) {
        if (numValues < 1) return INV_PROP;
        return OK;
    }
    else if (strcmp("GENDER", propName) == 0) {
        if (propOccurence > 1) return INV_PROP;
        if (numValues != 1 && numValues != 2) return INV_PROP;
        return OK;
    }
    else if (strcmp("TEL", propName) == 0) {
        if (numValues != 1 && numValues != 2) return INV_PROP;
        return OK;
    }
    else if (strcmp("ADR", propName) == 0) {
        if (numValues != 7) return INV_PROP;
        return OK;
    }
    else if (strcmp("CATEGORIES", propName) == 0) {
        return OK;
    }
    else if (strcmp("PRODID", propName) == 0 || strcmp("REV", propName) == 0 || strcmp("UID", propName) == 0) {
        if (numValues != 1) return INV_PROP;
        if (propOccurence > 1) return INV_PROP;
        return OK;
    }
    else if (strcmp("CLIENTPIDMAP", propName) == 0) {
        if (numValues != 2) return INV_PROP;
        return OK;
    }
    else if (strcmp("SOURCE", propName) == 0 || strcmp("XML", propName) == 0 || strcmp("PHOTO", propName) == 0 ||
            strcmp("EMAIL", propName) == 0 || strcmp("IMP", propName) == 0 ||
            strcmp("LANG", propName) == 0 || strcmp("TZ", propName) == 0 || strcmp("GEO", propName) == 0 ||
            strcmp("TITLE", propName) == 0 || strcmp("ROLE", propName) == 0 || strcmp("LOGO", propName) == 0 ||
            strcmp("ORG", propName) == 0 || strcmp("MEMBER", propName) == 0 || strcmp("RELATED", propName) == 0 ||
            strcmp("NOTE", propName) == 0 || strcmp("SOUND", propName) == 0 || strcmp("URL", propName) == 0 ||
            strcmp("KEY", propName) == 0 || strcmp("FBIURL", propName) == 0 || strcmp("CALADURI", propName) == 0 ||
            strcmp("CALURI", propName) == 0) {
                if (numValues != 1) return INV_PROP;
                return OK;
            }
    else if (strcmp("ANNIVERSARY", propName) == 0 || strcmp("BDAY", propName) == 0) return INV_DT;
    else if (strcmp("VERSION", propName) == 0)  {
        return INV_CARD;
    }
    else return INV_PROP;
}

// Checks if DateTime object follow proper convention
//      time must not be null, ie. should be checked for null prior to calling the function
VCardErrorCode validDT (DateTime *aTime) {
    if (!aTime) return INV_DT;
    if (aTime->UTC && aTime->isText) return INV_DT;
    if (aTime->isText && (strlen(aTime->date) || strlen(aTime->time))) return INV_DT;
    if (!aTime->isText && strlen(aTime->text)) return INV_DT;
    if (strlen(aTime->time) == 0 && strlen(aTime->date) == 0 && strlen(aTime->text) == 0) return INV_DT;
    if ((strlen(aTime->time) || strlen(aTime->date)) && strlen(aTime->text)) return INV_DT;
    if (!aTime->isText && strlen(aTime->date) == 0 && strlen(aTime->time) == 0) return INV_DT;
    // Checks if DT struct field date and time contains any alphabetical characters, returns error if so
    if (!aTime->isText && strlen(aTime->date)) {
        char * tempS = aTime->date;
        int len = strlen(tempS);
        int x = 0;
        while (x < len) {
            if (isalpha(tempS[x])) return INV_DT;
            ++x;
        }
    }
    if (!aTime->isText && strlen(aTime->time)) {
        char * tempS = aTime->time;
        int len = strlen(tempS);
        int x = 0;
        while (x < len) {
            if (isalpha(tempS[x])) return INV_DT;
            ++x;
        }
    }
    
    return OK;
}

VCardErrorCode createValidate (char * filename, Card **obj) {
    VCardErrorCode result = createCard(filename, obj);
    if (result != OK) return result;
    return validateCard(*obj);
}

VCardErrorCode editFN (char *newName, Card *obj) {
    if (newName && strlen(newName) > 0) {
        deleteDataFromList(obj->fn->values, getFromFront(obj->fn->values));
        insertFront(obj->fn->values, newName);
        return validateCard(obj);
    }
    return INV_PROP;
    
}

bool hasDT(DateTime *aTime) {
    if (strlen(aTime->date) && strlen(aTime->time)) return true;
    return false;
}

bool validFileExt(char * fileName) {
    if (strcmp(strrchr(fileName, '.'), ".vcf") != 0 && strcmp(strrchr(fileName, '.'), ".vcard") != 0) return false;
    return true;
}

VCardErrorCode initCard(char *firstName, char *fileName, Card **obj) {
    *obj = malloc(sizeof(Card));
    (*obj)->fn = malloc(sizeof(Property));
    void * tempVar = initializeList(propertyToString, deleteProperty, compareProperties);
    (*obj)->optionalProperties = (List *)tempVar;  
    (*obj)->anniversary = NULL;        
    (*obj)->birthday = NULL;
    (*obj)->fn->name = malloc(3);
    strcpy((*obj)->fn->name, "FN");
    (*obj)->fn->group = emptyString();
    (*obj)->fn->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    (*obj)->fn->values = initializeList(valueToString, deleteValue, compareValues);
    insertFront((*obj)->fn->values, firstName);
    VCardErrorCode errorCode = validateCard(*obj);
    if (errorCode != OK) {
        deleteCard(*obj);
        return errorCode;
    }
    errorCode = writeCard(fileName, *obj);
    if (errorCode != OK) {
        deleteCard(*obj);
        return errorCode;
    }
    return errorCode;
    
}