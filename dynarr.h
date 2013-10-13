#include <str.h>

struct index_S
  {
	  int key;
	  long off;
  };

struct avail_s
{
	int size;
	long off;	
	avail_s *next;
};

void* increase_size();

void print();	// Prints index_arr array

int find_key(int key);

int sort_index();

//filereader openfile(char mode);

int del_key_from_index(int del_key);

void free_node(int del_key);

void print_avail_list();

void add_avail_node(int offset, int size);

int get_free_mem(int size);

void create_avail_file();

void create_index_file();

void read_avail_file();

void read_index_file();