/*
CIS*2750 A1
Submission By: Anson Truong (1102843)
Due: Feb 10th 2025
File name: VCParser.c
*/


#include "VCParser.h"
#include "VCHelper.h"
#include <strings.h>
#include <ctype.h>

// Function parses a vCard read from fileName and stores it into obj if successful and returns a VCardErrorCode. If invalid
//          obj is set to NULL and an appropriate error code is returned. obj is dynamically allocated
//           (Caller must call free on obj)
VCardErrorCode createCard(char* fileName, Card** obj) {
    // Setting up boolean values
    bool validFN = false;
    bool validBegin = false;
    bool validEnd = false;
    bool validVersion = false;
    bool readingParameters = false;
    bool readingValues = false;
    bool foldDetected = false;
    VCardErrorCode returnCode = OK;     // Variable to hold the first error code detected
    int lineCount = 0;
    char  buffer[256];

    List *myList = initializeList(pairToString, free, pairCompare);     // List to hold indices of : ; \r or \n found in the buffer
    Pair * tempPair;
    char * tempString;
    int start = 0;
    int end = 0;
    char symbol = ' ';
    Parameter * tempParam;
    Property * tempProp;
    DateTime * tempDate;
    void * tempVar;
    char * subS;        // Pointer to substring of strstr or strpbrk

    // Return error if incorrect file extension
    if (fileName == NULL) {
        freeList(myList);
        return INV_FILE;
    }
    if (strcmp(strrchr(fileName, '.'), ".vcf") != 0 && strcmp(strrchr(fileName, '.'), ".vcard") != 0) {      
        freeList(myList);
        return INV_FILE;
    }

    FILE * fptr = fopen(fileName, "r");
    int lenInput = 0;
    if (fptr == NULL) {
        obj = NULL;
        freeList(myList);
        return INV_FILE;
    }
    *obj = malloc(sizeof(Card));
    if (*obj == NULL) {
        fclose(fptr);
        return OTHER_ERROR;
    }
    tempVar = initializeList(propertyToString, deleteProperty, compareProperties);
    if (tempVar == NULL) {
        free(*obj);
        fclose(fptr);
        *obj = NULL;
        return OTHER_ERROR;
    }
    (*obj)->optionalProperties = (List *)tempVar;  
    (*obj)->anniversary = NULL;        
    (*obj)->birthday = NULL;        

    tempProp = malloc(sizeof(Property));       
    if (tempProp == NULL) {
        freeList(tempVar);
        free(*obj);
        fclose(fptr);
        *obj = NULL;
        return OTHER_ERROR;
    }
    (*obj)->fn = tempProp;
    tempProp->name = malloc(sizeof(char) * 3);
    if (tempProp->name == NULL) {
        freeList(tempVar);
        free(*obj);
        fclose(fptr);
        *obj = NULL;
        return OTHER_ERROR;
    }
    strcpy(tempProp->name, "FN");       // Partially initialize FN property
    tempProp->group = emptyString();
    tempProp->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    tempProp->values = initializeList(valueToString, deleteValue, compareValues);
    while (fgets(buffer, 256, fptr) != NULL) {
        start = 0;
        tempString = NULL;
        clearList(myList);
        lenInput = strlen(buffer);
        specialCharIndexFinder(myList, buffer, lenInput);       // Stores character and index of any :, ;, /r, or /n in the read line into a list
        if (!validEnding(myList, buffer)) {  // Checks if the line read is terminated with \r and \n
            freeList(myList);
            fclose(fptr);
            deleteCard(*obj);
            return INV_PROP;
        }
        if (strlen(buffer) == 0) {      // Skips empty lines
            continue;
        }
        readingParameters = false;      // Reset booleans and set ptrs to null
        readingValues = false;
        foldDetected = false;
        //tempProp = NULL;
        tempParam = NULL;

        tempString = stringSplice(buffer, start, getCRIndex(myList));
        if (strcasecmp("BEGIN:VCARD", tempString) == 0) {       // Case to parse BEGIN:VCARD
            if (lineCount != 0 && returnCode == OK) {       // If BEGIN is found somewhere other than first line, handle error
                returnCode = INV_CARD;
            }
            validBegin = true;      // Mark BEGIN as seen
            free(tempString);
            tempString = NULL;
            continue;
        }
        else if (strcasecmp("END:VCARD", tempString) == 0) {        
            validEnd = true;            // Mark END as seen
            free(tempString);
            tempString = NULL;
            break;
        }
        free(tempString);
        tempString = NULL;
        Node * tempNode = myList->head;
        // Splice buffer from start to index of first special character
        tempString = stringSplice(buffer, start, ((Pair *)tempNode->data)->index);      
        
        // Handler for DateTime fields of type anniversary or bday
        if ((strcasecmp("anniversary", tempString) == 0 && (*obj)->anniversary == NULL) || 
            (strcasecmp("BDAY", tempString) == 0 && (*obj)->birthday == NULL)) {
            tempDate = malloc(sizeof(DateTime));        // Temp DateTime struct
            if (tempDate == NULL) {
                deleteCard(*obj);
                free(tempString);
                freeList(myList);
                fclose(fptr);
                *obj = NULL;
                return OTHER_ERROR;
            }
            tempDate->date = emptyString();             // Initialize fields for tempDate of type Datetime
            tempDate->time = emptyString();
            tempDate->text = emptyString();
            tempDate->UTC = false;
            tempDate->isText = false;

            Pair * tempPairStart = (Pair *)myList->tail->previous->previous->data;
            Pair * tempPairEnd =  (Pair *)myList->tail->previous->data;
            start = tempPairStart->index + 1;
            end = tempPairEnd->index; // End is the index of \r
   
            if (tempString[0] == 'A' || tempString[0] == 'a') {     // Points obj->anniversary pointer to the tempDate being filled
                (*obj)->anniversary = tempDate;
            }
            else {
                (*obj)->birthday = tempDate;        // Points obj->birthday pointer to the tempDate being filled
            }

            free(tempString); 

            tempString = stringSplice(buffer, start, end);      // String contains value of the date
            if(tempString[strlen(tempString)-1] == 'Z' || tempString[strlen(tempString) -1] == 'z') {       // Check UTC time
                tempDate->UTC = true;
            } else {
                tempDate->UTC = false;
            }
            if (isalpha(tempString[0]) && tempString[0] != 'T' && tempString[0] != 't') {       // Check if date is given as text
                tempDate->isText = true;
                free(tempDate->text);
                tempDate->text = tempString;
                continue;
            }
            else {          // If date is not text, toggle boolean and point a pointer to an empty string
                tempDate->isText = false;
            }
            // Check if date is given in format "--mmdd" (year not specified)
            if ((subS = strstr(tempString, "--")) != NULL && !tempDate->isText && strlen(subS) == 6) {  
                free(tempDate->date);      
                tempDate->date = malloc(sizeof(char) * strlen(subS) + 1);
                if (tempDate->date == NULL) {
                    free(tempString);
                    deleteCard(*obj);
                    freeList(myList);
                    fclose(fptr);
                    *obj = NULL;
                    return OTHER_ERROR;
                }
                strcpy(tempDate->date, subS);
                free(tempString);       // Free tempString since value has been copied to a tempDate->time which was dynamically allocated
                continue;;
            }
            // If len == 8, it has format "YYYYMMDD" with no time specified
            else if (strlen(tempString) == 8 && !tempDate->isText && strpbrk(tempString, "tT") == NULL) {     
                free(tempDate->date);
                tempDate->date = tempString;
                continue;
            }
            else if ((subS = strpbrk(tempString, "Tt")) != NULL && !tempDate->isText) {      // When date&time is given
                if (strlen(tempString) > 8) {
                    free(tempDate->date);
                    tempDate->date = stringSplice(tempString, 0, 8);
                }
                if (tempDate->UTC) {        // If it is UTC time, need to change end so 'Z' is not spliced together with the time
                    end = strlen(subS) - 1;
                }
                else {
                    end = strlen(subS);
                }
                free(tempDate->time);
                tempDate->time = stringSplice(subS, 1, end);
                free(tempString);
                continue;
            } else {
                 // None of the above if cases apply, date format is incorrect
                free(tempString);
                freeList(myList);
                deleteCard(*obj);
                fclose(fptr);
                *obj = NULL;
                return INV_DT;
            }
            
        } else {
            free(tempString);
            tempString = NULL;
        }
        while (tempNode != NULL) {      // Loop through and parse each substring found between colons(:) and semi-colons(;)
            if (readingValues) {
                tempNode = valueEndIndex(myList, start);
            }
            
            tempPair = (Pair *)tempNode->data;
            end = tempPair->index;
            symbol = tempPair->special;
            
            if (start == 0) {   // Checks to ensure splice  includes the index 0
                tempString = stringSplice(buffer, start, end);  // tempString stores the first bit of text from the start to the first ; or :
                // Case for parsing the FN field and its value into the card
                if (strcasecmp("FN", tempString) == 0 && !validFN) {
                    /*
                    free(tempString);
                    start = end + 1;
                    end = getCRIndex(myList);
                    tempString = stringSplice(buffer, start, end);      // Get value of fn property
                    if (strlen(tempString) == 0) {
                        returnCode = INV_PROP;
                    } */
                    tempProp = (*obj)->fn;
                    validFN = true;
                    if(symbol == ';') {     // If next symbol is ; a parameter is up next
                        readingParameters = true;
                    }
                    // Case if property only has value(s) and no parameters
                    else if (symbol == ':') {
                        readingParameters = false;
                        readingValues = true;
                    }
                    free(tempString);
                    //break;
                } else if (!validVersion && strcasecmp(tempString, "version") == 0) {
                        tempPair =  (Pair *)tempNode->next->data;
                        subS = stringSplice(buffer, end + 1, tempPair->index);
                        validVersion = true;
                        free(subS);
                        free(tempString);
                        break;
                // Case for parsing all optional properties and malloc'ing space for the property
                } else if (!isspace(tempString[0])) {
                    // Checks if last property added has atleast one value
                    if ((*obj)->optionalProperties->head != (*obj)->optionalProperties->tail) {
                        Node *zeroNode = (*obj)->optionalProperties->tail;
                        Property *zeroProp = (Property *)zeroNode->data;
                        if (zeroValues(zeroProp->values) && returnCode == OK) {
                            returnCode = INV_PROP;
                        }
                    }

                    tempProp = malloc(sizeof(Property));
                    if (tempProp == NULL) {
                        deleteCard(*obj);
                        free(tempString);
                        freeList(myList);
                        fclose(fptr);
                        *obj = NULL;
                        return OTHER_ERROR;
                    } if (!validColon(buffer) && returnCode == OK) {    // Checks if a colon is found in line including the property name
                        returnCode = INV_PROP;
                    }
                    if ((subS = strstr(tempString, ".")) != NULL) {     // Checks if property name includes a group name
                        tempProp->group = stringSplice(tempString, 0, groupEndIndex(tempString));
                        subS = stringSplice(buffer, groupEndIndex(tempString) + 1, end);
                        free (tempString);
                        tempString = subS;
                    } 
                    else {      // If no group is found, set group pointer to empty string
                        tempProp->group = emptyString();
                    }
                    tempProp->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
                    tempProp->values = initializeList(valueToString, deleteValue, compareValues);
                    insertBack((*obj)->optionalProperties, tempProp);
                    tempProp->name = tempString;
                    if (strlen(tempString) == 0 && returnCode == 0) {
                        returnCode = INV_PROP;
                    }
                    if(symbol == ';') {     // If next symbol is ; a parameter is up next
                        readingParameters = true;
                    }
                    // Case if property only has value(s) and no parameters
                    else if (symbol == ':') {
                        readingParameters = false;
                        readingValues = true;
                    }
                }
                else {      // Case if first character is a white space, i.e. signals line folding
                    foldDetected = true;        // Set boolean
                    //tempProp = (Property *)(getFromBack((*obj)->optionalProperties));
                    readingParameters = false;
                    readingValues = true;
                    ++start;        // Move start to exclude the white space
                    free(tempString);
                    tempString = NULL;
                    continue;       // Continue reloop and push node to next special character
                }
            } else {
                // Case for parsing parameters and values
                tempString = stringSplice(buffer, start, end);
                if (readingParameters) {        // Case for reading parameters
                    tempParam = malloc(sizeof(Parameter));
                    if (tempParam == NULL) {
                        freeList(myList);
                        free(tempString);
                        fclose(fptr);
                        *obj = NULL;
                        return OTHER_ERROR;
                    }
                    tempParam->name = paramName(tempString);
                    tempParam->value = paramValue(tempString);
                    // Changes error code if parameter has no name or has no value
                    if ((strlen(tempParam->name) == 0 || strlen(tempParam->value) == 0) && returnCode == OK) { 
                        returnCode = INV_PROP;
                    }
                    insertBack(tempProp->parameters, tempParam);
                    free(tempString);
                    tempString = NULL;
                } else if (readingValues) {
                    if (foldDetected) {
                        Node * endNode = tempProp->values->tail;
                        char * reallocPtr;
                        reallocPtr = realloc(endNode->data, sizeof(char) * (strlen(endNode->data) + strlen(tempString) + 1));
                        endNode->data = reallocPtr;
                        strcat(endNode->data, tempString);
                        free(tempString);
                        foldDetected = false;
                    }
                    else {
                        insertBack(tempProp->values, tempString);
                    }
                    //  if (strcasecmp((char *) tempProp->name, "version") == 0) {
                    //     if (strcasecmp((char *)tempProp->values->tail->data, "4.0") != 0 && returnCode == OK) {
                    //         returnCode = INV_CARD;
                    //     }
                    //     else {
                    //         validVersion = true;

                    //     }
                    // }
                }
                if (symbol == ':') {
                    readingParameters = false;
                    readingValues = true;
                }
            }
            start = tempPair->index + 1;
            tempNode = tempNode->next;
            if (symbol == '\r') {
                ++lineCount;
                break;
            }

        }
    }

    // Checks if last property added has atleast one value
    if ((*obj)->optionalProperties->head != (*obj)->optionalProperties->tail) {
        Node *zeroNode = (*obj)->optionalProperties->tail;
        Property *zeroProp = (Property *)zeroNode->data;
        if (zeroValues(zeroProp->values) && returnCode == OK) {
            returnCode = INV_PROP;
        }
    }
    
    freeList(myList);
    fclose(fptr);
  
    if (validBegin && validEnd && validFN  && returnCode == OK) {
        return OK;
    }
    else if (!validBegin) {
        deleteCard(*obj);
        return INV_CARD;
    }
    else if (returnCode != OK) {
        deleteCard(*obj);
        return returnCode;
    }
    else {
        deleteCard(*obj);
        return INV_CARD;
    }
}

void deleteCard(Card* obj) {
    if (obj == NULL) {
        return;
    }
    deleteProperty(obj->fn);
    freeList(obj->optionalProperties);
    if (obj->birthday) {
        deleteDate(obj->birthday);
    }
    if (obj->anniversary) {
        deleteDate(obj->anniversary);
    }
    free(obj);
}

char* cardToString(const Card* obj) {
    char *tempString = malloc(sizeof(char) * 5);
    strcpy(tempString, "null");
    if(obj == NULL) {
        return tempString;
    }
    free(tempString);
    char * result = malloc(sizeof(char) * 2000);
    char * propString = propertyToString(obj->fn);
    char * optPropString = toString(obj->optionalProperties);
    char * birthdayString = dateToString(obj->birthday);
    char * anniversaryString = dateToString(obj->anniversary);
    sprintf(result, "%sProperties: \n%s\nBirthday: %s\nAnniversary: %s\n", propString, optPropString, birthdayString, anniversaryString);
    free(propString);
    free(optPropString);
    free(birthdayString);
    free(anniversaryString);
    return result;
}

char* errorToString(VCardErrorCode err) {
    char * s = malloc(sizeof(char) * 50);
    if (err == OK) {
        strcpy(s, "Code OK: vCard read sucessfully");
    } else if (err == INV_CARD) {
        strcpy(s, "ERROR: Invalid vCard");
    } else if (err == INV_FILE) {
        strcpy(s, "ERROR: File could not be read");
    } else if (err == INV_PROP) {
        strcpy(s, "ERROR: Invalid Property");
    } else if (err == INV_DT) {
        strcpy(s, "ERROR: Invalid DateTime property");
    } else if (err == WRITE_ERROR) {
        strcpy(s, "ERROR: Invalid Write Error");
    } else {
        strcpy(s, "ERROR: Unknown Other Error");
    }
    return s;
}

void deleteProperty(void* toBeDeleted){
    if (toBeDeleted == NULL) {
        return;
    }
    Property * temp = (Property *)toBeDeleted;
    free(temp->name);
    free(temp->group);
    freeList(temp->parameters);
    freeList(temp->values);
    free(toBeDeleted);
}

int compareProperties(const void* first,const void* second){
    
    return 0;
}

char* propertyToString(void* prop) {
    char *tempString = malloc(sizeof(char) * 5);
    strcpy(tempString, "null");
    if(prop == NULL) {
        return tempString;
    }
    free(tempString);
    char * result = malloc(sizeof(char) * 500);
    Property * myProp = (Property *)prop;
    char * paramString = toString(myProp->parameters);
    char * valuesString = toString(myProp->values);
    sprintf(result, "Property name: [%s] Group: [%s] Parameters: %s Values: %s\n", myProp->name, myProp->group, paramString, valuesString);
    free(paramString);
    free(valuesString);
    return result;
}

void deleteParameter(void* toBeDeleted) {
    if (toBeDeleted == NULL) {
        return;
    }
    Parameter * temp = (Parameter *)toBeDeleted;
    free(temp->name);
    free(temp->value);
    free(toBeDeleted);
}

int compareParameters(const void* first,const void* second) {
    Parameter * item1 = (Parameter *)first;
    Parameter * item2 = (Parameter *)second;
    if (strcmp(item1->name, item2->name) == 0 && strcmp(item1->value, item2->value)) {
        return 0;
    }
    return -1;
}

char* parameterToString(void* param) {
    char *tempString = malloc(sizeof(char) * 5);
    strcpy(tempString, "null");
    if (param == NULL) {
        return tempString;
    }
    free(tempString);
    Parameter * temp = (Parameter *)param;
    int len = strlen(temp->name) + strlen(temp->value) + 10;
    char * paramString = malloc(sizeof(char) * len);
    sprintf(paramString, "[%s = %s] ", temp->name, temp->value);
    return paramString;
}

void deleteValue(void* toBeDeleted) {
    if (toBeDeleted == NULL) {
        return;
    }
    free(toBeDeleted);
}

int compareValues(const void* first,const void* second) {
    if (first == NULL || second == NULL) {
        return -1;
    }
    if (strcmp((char *)first, (char *)second) == 0) {
        return 0;
    }
    return -1;
}

char* valueToString(void* val) {
    char *tempString = malloc(sizeof(char) * 5);
    strcpy(tempString, "null");
    if(val == NULL) {
        return tempString;
    }
    free(tempString);
    char * s = (char *)val;
    int len = strlen(s) + 15;
    char * valString = malloc(sizeof(char) * len);
    sprintf(valString, "[%s]", s);
    return valString;
}

void deleteDate(void* toBeDeleted) {
    if (toBeDeleted == NULL) {
        return;
    }
    DateTime * temp = (DateTime *)toBeDeleted;
    free(temp->date);
    free(temp->text);
    free(temp->time);
    free(toBeDeleted);
}

int compareDates(const void* first,const void* second) {
    if (!first || !second) {
        return -1;
    }
    DateTime * firstDT = (DateTime *)first;
    DateTime * secondDT = (DateTime *)second;
    if ((!firstDT->UTC && secondDT->UTC) ||(firstDT->UTC && !secondDT->UTC)) {
        return -1;
    }
    if ((!firstDT->isText && secondDT->isText) ||(firstDT->isText && !secondDT->isText)) {
        return -1;
    }
    if (strcmp(firstDT->text, secondDT->text) != 0 || strcmp(firstDT->date, secondDT->date) != 0 || strcmp(firstDT->time, secondDT->time) != 0) {
        return -1;
    }
    return 0;
}

char* dateToString(void* date) {
    char * tempString = malloc(sizeof(char) * 5);
    strcpy(tempString, "null");
    char *hour = NULL;
    char *minute = NULL;
    char *second = NULL;
    char *year = NULL;
    char *month = NULL;
    char *day = NULL;
    int len = 0;
    if (date == NULL) {
        return tempString;
    }
    free(tempString);
    DateTime * temp = (DateTime *)date;
    if (strlen(temp->time) == 6) {
        hour = stringSplice(temp->time, 0, 2);
        minute = stringSplice(temp->time, 2, 4);
        second = stringSplice(temp->time, 4, 6);
    }
    if (strlen(temp->date) == 6) {
        year = stringSplice(temp->date, 0, 2);
        month = stringSplice(temp->date, 2, 4);
        day = stringSplice(temp->date, 4, 6);
    }
    else if (strlen(temp->date) == 8) {
        year = stringSplice(temp->date, 0, 4);
        month = stringSplice(temp->date, 4, 6);
        day = stringSplice(temp->date, 6, 8);
    }
    if (temp->isText) {
        tempString = malloc(sizeof(char) * strlen(temp->text) + 1);
        strcpy(tempString, temp->text);
        return tempString;
    }
    else {
        if (hour != NULL && day != NULL) {
            len = strlen(hour) + strlen(minute) + strlen(second) + strlen(year) + strlen(month) + strlen(day) + 100;
            tempString = malloc(sizeof(char) * len);
            if (temp->UTC) {
                sprintf(tempString, "Date: %s%s%s Time: %s%s%s (UTC)\n", day, month, year, hour, minute, second);
            }
            else {
                sprintf(tempString, "Date: %s%s%s Time: %s%s%s\n", day, month, year, hour, minute, second);
            }
        } else if (hour != NULL) {
            len = strlen(hour) + strlen(minute) + strlen(second) + 20;
            tempString = malloc(sizeof(char) * len);
            if (temp->UTC) {
                sprintf(tempString, "Time: %s%s%s (UTC)\n", hour, minute, second);
            }
            else {
                sprintf(tempString, "Time: %s%s%s\n", hour, minute, second);
            }
        }
        else {
            len = strlen(year) + strlen(month) + strlen(day) + 20;
            tempString = malloc(sizeof(char) * len + 1);
            sprintf(tempString, "Date: %s%s%s \n", day, month, year);

        }
    }
    
    if (strlen(temp->time) > 1) {
        free(hour);
        free(minute);
        free(second);
    }
    if (strlen(temp->date) > 1) {
        free(year);
        free(month);
        free(day);
    }
    return tempString;
}

// Writes the Card obj to the vcard file named fileName
VCardErrorCode writeCard(const char* fileName, const Card* obj) {
    if (!obj || !fileName) return WRITE_ERROR;
    if (strcmp(strrchr(fileName, '.'), ".vcf") != 0 && strcmp(strrchr(fileName, '.'), ".vcard") != 0) {      
        return WRITE_ERROR;
    }
    FILE * fptr;
    fptr = fopen(fileName, "w");
    if (fptr == NULL) {
        return WRITE_ERROR;
    }
    fprintf(fptr, "BEGIN:VCARD\r\nVERSION:4.0\r\n");
    fprintf(fptr, "%s:%s\r\n", obj->fn->name, (char *)getFromFront(obj->fn->values));   //Write FN property to file
    if (obj->birthday) {
        writeOutDates(fptr, 0, obj->birthday);
    }
    if (obj->anniversary) {
        writeOutDates(fptr, 1, obj->anniversary);
    }
    void * elem;
    ListIterator propIter = createIterator(obj->optionalProperties);
    while((elem = nextElement(&propIter)) != NULL) {
        Property * tempProp = (Property *)elem;
        writeOptionalProperties(fptr, tempProp);
    }
    fprintf(fptr, "END:VCARD\r\n");
    fclose(fptr);
    return OK;
}

// Validates the vCard obj
VCardErrorCode validateCard (const Card *obj) {
    VCardErrorCode result = validateCardShape(obj); // validate the surface level fields of the vCard obj
    if (result != OK) return result;
    if (obj->birthday) result = validDT(obj->birthday);  // Validate birthday if one exists
    if (result != OK) return result;
    if (obj->anniversary) result = validDT(obj->anniversary);   // Validate anniversary if one exists
    if (result != OK) return result;

    void * elem;
    ListIterator iter = createIterator(obj->optionalProperties);    // Iterate over each Property and validate each one
    while ((elem = nextElement(&iter)) != NULL) {
        Property * tempProp = (Property *)elem;
        if (tempProp->name == NULL) return INV_PROP;
        result = validProperty(tempProp, numPropOccurence(obj->optionalProperties, tempProp->name));
        if (result != OK) return result;
    }
    return OK;
}
