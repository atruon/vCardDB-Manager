# vCardDB-Manager Program  
This is a terminal-based program that allows parsing, viewing, and editing vCard files, which can then be stored in a SQL database through MySQL.  
The front-end, is implemented in **Python**, using the **asciimatics** module to allow for a terminal-based interactive GUI.  
The main functions involved in parsing, and editing are coded in **C**, which are called by the Python front-end via the **ctypes** module.  
