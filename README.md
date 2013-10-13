## File Manager
###In Memory indexing with Availability Lists ###
----
###Goal

Maintain user data provided in a file efficiently and support Add, Delete, Find command.

1. Use field delimiters and record counts for field and record organization.
2. Build and maintain an in-memory primary key index to improve search efficiency.
3. Use an in-memory availability list to support the reallocation of space for records that are deleted.
 
### User Commands

1. **_add newrecord_**  Add a new record _newrecord_ to the file. For example: add 1234|Rohit|Kandhal|CSC
2. **_find id_** Finds the record with ID = _id_ if it exists.
3. **_del id_** Deletes the record with ID = _id_ if it exists.
4. **_end_** Ends the program, closes the file and writes index and availability lists to the correcponding index and availability files.

Multiple executing of program should maintain the index and availability lists i.e. index should not be 
recreated after every _end_ command.
