#include <windows.h>
#include <filereader.h>
#include <str.h>
#include <stdio.h>
#include <time.h>
#include <dynarr.h>
#include <iostream>
#include <fstream>

int index_Size;
int size_boost;
int ind_curr;		// Current free node pointer
string fname;		// constant file name;

index_S *index_arr;		// Array to store index
avail_s *head_avail;	// head of availability list

int file_off;		// Global file offset pointer
filereader fp;

int main( int argc, char *argv[] )
{
  char buffer[1024];
  string param[4];		// SID|LAST|FIRST|MAJOR
  string cin_str;	// Take input command + Param into this string from CIN
  
  int count;
  int i;
  
  i = 0;
  count = 0;
  index_Size = 1;	// index_arr current size. Initialize by 5;
  size_boost = 1;	// Set size step to 5;
  ind_curr = 0;
  file_off = 0;		// start file with offset 0;

  index_arr = (index_S*)malloc(sizeof(index_S) * index_Size);
  
  head_avail = NULL;
  
  if(argc > 1)
  {
	  fname = argv[1];
  }
  else
  {
	  fname="student";	// Default file name
  }

  // Open student file
  fp.open(fname+".txt", 'x',1,1);

  read_index_file();

  read_avail_file();
  
  while(std::cin >> cin_str)
  {
	string tmp[2];	// tmp[0] -> Command [add|delete etc] tmp[1] -> SID|LAST|FIRST|MAJOR

	cin_str.token(tmp, strlen(cin_str), " ");		// Split cin_str to tmp[0] and tmp[1]
	  
	  // 1. Add Command
	  if(strcmp(tmp[0], "add") == 0)
	  {
		  tmp[1].token(param, strlen(tmp[1]), "|");

		  // Check if the SID (param[0]) exists in index.
		  if(find_key(param[0]) >= 0)
		  {
			  // Continue to next input. Nothing to do if element already exists
			  continue;
		  }

		  // Size required 
		  count = strlen(tmp[1]);

		  // Get free memory for this node:
		  int hole_off;		// hole offset if present
		  hole_off = get_free_mem(count + 4);
		  
		  int final_off;
		  if(hole_off != -1)
		  {
			  final_off = hole_off;
		  }
		  else 
		  {
			  final_off = file_off;
		  }

		  // Create new index node
		  index_S* new_index_node = new index_S();
		  new_index_node->key = param[0];
		  new_index_node->off = final_off;	// Offset to this node comes from the global variable
		  
		  // File current pointer may have shifted. Reset it to point where we can add new record
		  fp.seek(final_off, BEGIN);

		  // Ind_CURR -> Current fresh node pointer, Index_Size -> Total size of array
		  if(ind_curr >= index_Size)
		  {
			  // Increase size of array by size_boost amount.
			  increase_size();
		  }

		  index_arr[ind_curr++] = *new_index_node;

		  // Sort the array now
		  sort_index();

		  // Write to file
		  fp.write_raw((char*) &count, sizeof(int));
		  fp.write_raw(tmp[1], strlen(tmp[1]));

		  // Update file offset to accomodate recent addition
		  if(hole_off == -1)
		  {
			  file_off += (count + 4);
		  }
	  }

	  // 2. Find Command
	  else if(strcmp(tmp[0], "find") ==	 0)
	  {
		  // No need to open file for read mode. Already in x mode
		  tmp[1].token(param, strlen(tmp[1]), "|");

		  // Check if the SID (param[0]) exists in index.
		  int element_index;
		  int tmp_offset;
		  int tmp_count;	// record length
		  
		  element_index = find_key(param[0]);
		  
		  if(element_index >= 0)
		  {
			  // Element found. Retrieve it's details from the file using in-memory index
			  // 1. Find offset of record in file
			  tmp_offset = index_arr[element_index].off;
			  
			  // 2. Go to that location and retrieve count value
			  fp.seek(tmp_offset, BEGIN);
			  fp.read_raw((char *) &tmp_count, sizeof(int));
			  
			  // 3. Read the record
			  fp.read_raw(buffer, tmp_count);
			  
			  // 4. Print record
			  i = 0; 
			  printf("\n");
			  while( i < (tmp_count))
			  {
				  printf("%c", buffer[i]);
				  i++;
			  }
		  }
		  else
		  {
			  std::cout << "\nNo record with SID=" << param[0] << " exists";
		  }
	  }

	  // 3. Del command
	  else if(strcmp(tmp[0], "del") ==	 0)
	  {
		  tmp[1].token(param, strlen(tmp[1]), "|");

		  // Check if the SID (param[0]) exists in primary key index.
		  int element_index;
		  
		  element_index = find_key(param[0]);

		  if(element_index >=0)
		  {
			  // Find location and size.
			  free_node(element_index);

			  // Remove entry for this record from primary key index
			  del_key_from_index(element_index);
		  }
		  else
		  {
			  std::cout << "\nNo record with SID=" << param[0] << " exists";
		  }
	  }

	  // 4. End command
	  else if(strcmp(tmp[0], "end") == 0)
	  {
		  fp.close();

		  // Create index file
		  create_index_file();

		  // Create availability list file
		  create_avail_file();

		  print();

		  print_avail_list();
	  }
  }
   
   free(index_arr);
   
   return 1;
}


void* increase_size()
{
	index_S* new_index_arr;

	index_Size += size_boost;

	new_index_arr = (index_S*)realloc(index_arr, index_Size * sizeof(index_S));

	if(new_index_arr == NULL)
	{
		printf("\nREALLOC ERROR");
	}
	else
	{
		// Copying new pointer to old pointer
		index_arr = new_index_arr;
	}
	return index_arr;
}


void print()
{
	int i;
	i = 0;
	printf("\nIndex:");
	//printf("\n File Offset %d", file_off);
	while(i<ind_curr)
	{
		index_S* tmp = &index_arr[i];
		printf("\n%d: %ld", tmp->key, tmp->off);
		i++;
	}
}

// http://stackoverflow.com/questions/21647/reading-from-text-file-until-eof-repeats-last-line
// Find the key in index array (ind_arr) which is global using Binary Search
// Returns the index number of node
int find_key(int key)
{
	if(ind_curr == 0)
	{
		return -1;
	}

	int start, end, mid;
	start = 0;
	end = ind_curr-1;	// ind_curr holds the latest fresh element i.e. last to 1
	mid = 0;

	// Binary Search
	while(start<= end)
	{
		mid = start + (end - start)/2;

		if(index_arr[mid].key < key)
		{
			start = mid+1;
		}
		else if(index_arr[mid].key > key)
		{
			end = mid-1;
		}
		else
		{
			// Found the element
			return mid;
		}
	}
	return -1;
}

int sort_index()
{
	// assuming this node's key does not exist in index array
	int start,end;
	start = 0;
	end = ind_curr-1;

	if(ind_curr < 2)
	{
		return 0;
	}

	index_S temp;
	
	while(end > 0)
	{
		if(index_arr[end].key < index_arr[end-1].key)
		{
			// Swap elements 
			temp = index_arr[end];
			index_arr[end] = index_arr[end-1];
			index_arr[end-1] = temp;
		}
		end--;
	}
	return 1;
}

// Deletes a node from Primary Key index
int del_key_from_index(int del_key)
{
	int start,end;
	end = ind_curr-1;

	if(ind_curr == 0)
	{
		return 0;
	}

	// Only element which needs to be deleted
	else if(ind_curr == 1 && del_key == 0)
	{
		memset(index_arr, NULL,sizeof(index_S));
		ind_curr--;
	}
	// Multiple elements
	else
	{
		index_S* temp;
		start = del_key;

		while(start < end)
		{
			index_arr[start] = index_arr[start+1];
			start++;
		}

		temp = &index_arr[start];
		memset(temp, NULL,sizeof(index_S));

		ind_curr--;
	}
}

// Frees a node: and adds it to availability list
void free_node(int del_key)
{
	int tmp_offset;
	int tmp_count;
	int hole_size;

	// 1. Find the offset of record in file or location
	tmp_offset = index_arr[del_key].off;
			  
	// 2. Go to that location and retrieve count value
	fp.seek(tmp_offset, BEGIN);
	fp.read_raw((char *) &tmp_count, sizeof(int));

	hole_size = tmp_count + 4;
	// add node to availability list
	add_avail_node(tmp_offset, hole_size);
}

// Add node to availability list
void add_avail_node(int offset, int size)
{
	avail_s* tmp;
	tmp = head_avail;	// set head to avail

	// Create new node with size and offset
	avail_s* new_node = new avail_s();
	new_node->off = offset;
	new_node->size = size;
	new_node->next = NULL;

	// List not empty
	if(tmp != NULL)
	{
		// Go to the end of list
		while(tmp->next!= NULL)
		{
			tmp = tmp->next;
		}

		// Add hole in the end
		tmp->next = new_node;
	}
	else
	{
		// Add hole in front of availability list
		head_avail = new_node;
	}
}

void print_avail_list()
{
	avail_s* tmp;
	tmp = head_avail;
	printf("\nAvailability:");
	while(tmp!= NULL)
	{
		printf("\n%ld: %d", tmp->off, tmp->size);
		tmp = tmp->next;
	}
}

// 1. Checks if there is any hole for required size.
// 2. If available then return the offset of that hole
// 3. Partition the hole if required and add in the end
// 4. Return -1 if no hole of required size found
int get_free_mem(int size)
{
	avail_s* point1;
	avail_s* point2;	// Pointer 1, Pointer 2
	point1 = head_avail;
	point2 = head_avail;

	// Check if there is any element in list
	if(point1 != NULL)
	{
		// Deleting a node from a linked list
		// Keep two pointer.
		// forward first pointer to a hole which suits us
		// bring back the second pointer to a node before that hole
		while(point1 != NULL && point1->size < size)
		{
			point1 = point1->next;
		}

		if(point1 == NULL)
		{
			// NO appropriate hole found
		}
		else
		{
			// First element 
			if(point2 == point1)
			{
				head_avail = (point1->next == NULL) ? NULL : point1->next;
			}
			else
			{
				// Forward second pointer
				while(point2->next != point1 && point2 != NULL)
				{
					point2 = point2->next;
				}

				if(point2->next->next != NULL)
				{
					point2->next = point2->next->next;
				}
				else
				{
					// Last element
					point2->next = NULL;
				}
			}

			// At this point, appropriate hole has been found and deleted from the availability list
			// Now if there is any extra space then add that to availability list.
			if(point1->size >= size)
			{
				int reqd_offset;
				int extra_size;

				reqd_offset = point1->off;
				extra_size = point1->size - size;
				
				if(extra_size > 0)
				{
					int new_node_off;	// New hole offset
					new_node_off = reqd_offset + size;

					// add it to availability list
					add_avail_node(new_node_off, extra_size);
				}

				return reqd_offset;
			}
		}
	}
	return -1;
}

void create_index_file()
{
	// Create index file
	std::fstream indexfile;

	indexfile.open(fname + ".idx", std::ios::out| std::ios::trunc);
		  
	if(!indexfile)
	{
		indexfile.open(fname + ".idx", std::ios::out| std::ios::trunc);
		indexfile.close();
		indexfile.open(fname + ".idx", std::ios::out| std::ios::in);
	}
		  
	if(indexfile.is_open())
	{
		// First of all push file_off - file offset of file 
		// File offset is required to write new data to file
		indexfile << "file_off" << "|" << file_off << "\n";

		int i;
		i = 0;
		while(i<ind_curr)
		{
			index_S* tmp = &index_arr[i];
			indexfile << tmp->key << "|" <<  tmp->off << "\n";
			i++;
		}

		indexfile.close();
	}
}

void create_avail_file()
{
	std::fstream availfile;

	availfile.open(fname + ".avl", std::ios::out| std::ios::trunc);
		  
	if(!availfile)
	{
		availfile.open(fname + ".avl", std::ios::out| std::ios::trunc);
		availfile.close();
		availfile.open(fname + ".avl", std::ios::out| std::ios::in);
	}
		  
	if(availfile.is_open())
	{
		avail_s* tmp;
		tmp = head_avail;
			  
		if(tmp != NULL)
		{
			availfile << tmp->off << "|" << tmp->size << "\n";

			while(tmp->next != NULL)
			{
				tmp = tmp->next;
				availfile << tmp->off << "|" << tmp->size<< "\n";
			}
		}

		availfile.close();
	}
}

void read_avail_file()
{
	std::fstream availfile;
	
	avail_s* avail_list;
	avail_list = NULL;
	availfile.open(fname + ".avl", std::ios::in);
	
	head_avail = avail_list;

	// If availability list file exists, Populate array
	if(availfile.is_open())
	{
		string str1;

		while(availfile >> str1)
		{
			string tmp[2];

			str1.token(tmp, strlen(str1), "|");

			avail_s* new_node = new avail_s();
			new_node->off = atol(tmp[0]);
			new_node->size = tmp[1];
			new_node->next = NULL;

			if(avail_list == NULL)
			{
				head_avail = new_node;
				avail_list = head_avail;
			}
			else
			{
				avail_list->next = new_node;
				avail_list = avail_list->next;
			}
		}

		availfile.close();
	}
}

void read_index_file()
{
	std::fstream indexfile;
	
	index_S* index_list;

	indexfile.open(fname + ".idx", std::ios::in);

	if(indexfile.is_open())
	{
		string str1;

		// Read index file one by one
		while(indexfile >> str1)
		{
			string tmp[2];

			str1.token(tmp, strlen(str1), "|");

			if(strcmp(tmp[0], "file_off") == 0)
			{
				file_off = tmp[1];
			}
			else
			{
				// Create new index node
				index_S* new_node = new index_S();
				new_node->key = tmp[0];
				new_node->off = atol(tmp[1]);

				if(ind_curr >= index_Size)
				{
					// Increase size of array by size_boost amount.
					increase_size();
				}
				index_arr[ind_curr++] = *new_node;
			}
		}
		indexfile.close();
	}
}
