# Submission by:
 Anson Truong (1102843)
 CIS2750 A3

### Grade breakdown
The grades below reflect full functionality - UI connected to the C shared library and a MySQL database.
Marks will be deducted if functionality is missing or buggy.
Each UI component that is not connected to the database and the C code would only be worth 20% of
its overall grade - e.g DB login is only 2 marks, Main view - 5 marks, etc..

### DB Login view (10 marks)
[x] User enters all the credentials
[x]Correctly creates a connection to the database with entered credentials
[x]Displays an error if a DB connection could not be established for any reason - e.g. because incorrect
credentials were entered.
[x]Tables in the DB are correctly created, and creation happens only if they do not already exist

### Main view (30 marks)
[x]The view displays all the required details in table format, as specified in Module 1
[x]Does not display invalid files
[x]Files are clickable
[x]Correctly updated when a file is modified or created
[x]Tables in the DB are correctly populated with the file information
[x]Correctly exists program

### Card Detail view (30 marks) - showing parameters and adding / editing cards is graded separately
[x]Displays correct information for the selected file
[x]Information can be modified as stated in Module 1
[x]Changes to the contents of the Card Detail view are reflected on the disk and in the DB tables
[x]Correctly exists to main view

### Create vCard (10 marks)
[x]Create a new Card object, pass it to the parser, save it to the vCard file
[x]User enters all details using forms
[x]Validates user input and does not allow a user to create an invalid vCard file
[x]Files updated in the DB

### DB View (20 marks)
[x]Runs the two required queries and correctly displays the information stored in the tables
[x]Correctly exists to main view
Total: 100 marks