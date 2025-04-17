/*
CIS*2750 A1
Submission By: Anson Truong (1102843)
Due: Feb 10th 2025
File name: VCHelper.h
*/

#ifndef _HELPER_H
#define _HELPER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "LinkedListAPI.h"
#include "VCParser.h"

typedef struct myPair {
    int index;
    char special;
} Pair;

// Function intializes a declared myList which Pair data type, which contains a symbol and index.
void specialCharIndexFinder(List *myList, char * s, int sLen);

// Returns a dynamically allocated string representation of the Pair item
char * pairToString(void *item);

// Function to compare Pair data type. Return 0 if two items are equal, and -1 otherwise.
int pairCompare(const void *item1, const void *item2);

// Returns the index number of where \r would be found in the original string by looking through myList which contains symbol, index pairs
int getCRIndex(List *myList);

// Returns an integer representing the index where the = character is found
int indexOfValue(char * s);

// Returns a dynamically allocated string containing the substring of s from index start to index end (exclusive)
char * stringSplice(char *s, int start, int end);

// Returns true if the string S contains the character T and false otherwise.
bool containsTime(char * s);

// Returns a dynamically allocated empty string, containing the '\0' character. Must be freed by caller.
char * emptyString();

// Returns a dynamically allocated string containing the name of a parameter string. Must be freed by caller
char * paramName (char * s);

// Returns a dynamically allocated string containing the value of a parameter string. Must be freed by caller
char * paramValue (char *s);

// Returns the node  of where the value ends which is where the next ; or \r is found.
Node * valueEndIndex(List *myList, int start);

// Returns the index of the '.' character within a property name that includes the group name
int groupEndIndex(char * s);

// Returns true if string s terminates with /r/n and false otherwise
bool validEnding(List *myList, char * s);

// Returns true if string s contains a colon and false otherwise
bool validColon(char * s);

// Returns true if the given values list is missing values and false otherwise
bool zeroValues(List *myList);

// Function writes the given Property out to a file using the FILE pointer given as argument
void writeOptionalProperties(FILE *fptr, Property * prop);

// Function writes the give DateTime structure out to a file using the FILE pointer given as argument
void writeOutDates(FILE *fptr, int mode, DateTime *aDate);

// Returns the total number of values in the values list
int valuesLength(List * values);

// Returns the total number of occurence of propName in the list of optionalProperties
int numPropOccurence(List * optionalProperties, char * propName);

// Returns error code OK if the given Card obj has a valid surface level structure and INV_Card otherwise
VCardErrorCode validateCardShape(const Card *obj);

// Validates the Property tempProp and returns an approriate error code
VCardErrorCode validProperty (Property * tempProp, int propOccurence);

// Validates the DateTime aTime and returns an appropriate error code
VCardErrorCode validDT (DateTime *aTime);

// Wrapper function that calls createCard and validateCard by reading the vCard file given by filename and stores it into obj.
VCardErrorCode createValidate(char *filename, Card ** obj);

VCardErrorCode editFN (char *newName, Card *obj);

bool hasDT(DateTime *aTime);

bool validFileExt(char * fileName);

VCardErrorCode initCard(char *fn, char *fileName, Card **obj);

#endif