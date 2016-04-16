#include "ospf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char* lsa_file;
extern char* servers_file;
lsa*         lsa_hash = NULL;

void is_server(char* IP, lsa* myLSA){
	FILE* fp = fopen(servers_file, "r");
	if(fp = NULL) return;

	char* found = NULL;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	while((read = getline(&line, &len, fp)) != -1){
		found = strstr(line, IP);
		if(found != NULL){
			myLSA->server = 1;
			break;
		}
	}
	fclose(fp);
	return;
}

void parse_nbors(lsa* myLSA, char *nbors){
	int comma_count = 0;
	int token_count = 0;

	char* line 		= strstr(nbors, ",");
	char* token;
	while(line != NULL){
		comma_count++;
		line = strstr(line+1, ",");
	}

	myLSA->num_nbors = comma_count;
	myLSA->nbors = calloc(1, sizeof(char*));
	for(int i = 0; i <= comma_count; i++){
		myLSA->nbors[i] = calloc(1, MAX_IP_SIZE + 1);
	}

	token = strtok(nbors, ",");
	while(token){
		strcpy(myLSA->nbors[token_count], token);
		token = strtok(NULL, ",");
		token_count++;
	}
	return;
}

void parse_file(){
	FILE* fp = fopen(lsa_file, "r");
	if(fp = NULL) return;

	char* 	line = NULL;
	size_t 	len = 0;
	ssize_t read;
	char*	IP;
	int 	seq;
	char*	nbors;
	lsa*	temp;
	lsa*	find;

  /*************************************************************************/
  /* Comments from Big Brother Fadhil:                                     */
  /*                                                                       */
  /*   (1). char* IP and char* nbors need to be allocated memory.          */
  /*   sscanf does not automatically allocate memory for those strings.    */
  /*   This is buffer-overflow code.                                       */
  /*                                                                       */
  /*   (2). Line 80. char* IP will decay upon exiting function. Do         */
  /*   a strncpy. But make sure you first fix the sscanf bit.              */
  /*                                                                       */
  /*   (3). you have to free char* line. getline mallocs memory internally */
  /*   for it. It is up to you to free.                                    */
  /*                                                                       */
  /*   (4). Close the lsa_file after you're done.
  /*
  /*   Ciao ciao                                                           */
  /*************************************************************************/

	while((read = getline(&line, &len, fp)) != -1){
		sscanf(line, "%s %d %s", IP, seq, nbors);
		temp = calloc(1, sizeof(lsa));
		temp->sender = IP;
		temp->seq = seq;
		is_server(IP, temp);
		parse_nbors(temp, nbors);
		HASH_FIND_STR(lsa_hash, IP, find);
		if(find == NULL){
			HASH_ADD_STR(lsa_hash, sender, temp);
		} else {
			if(find->seq < temp->seq){
				for(int i = 0; i <= find->num_nbors; i++){
					free(find->nbors[i]);
				}
				free(find->nbors);
				find->nbors = temp->nbors;
				find->num_nbors = temp->num_nbors;
				find->seq = temp->seq;
			}
		}
	}
}

/*********************************************************/
/* @brief  Returns the nearest server to 'src'.          */
/* @param  graph The graph containing nodes and links.   */
/* @param  src   The source node to start Djikstra from. */
/*                                                       */
/* @return The lsa struct of the nearest server.         */
/*********************************************************/
lsa* shortest_path(lsa* graph, char* src)
{
  lsa*    lsa_info = NULL; lsa* visitcheck = NULL;
  heap_t *h        = calloc(1, sizeof (heap_t));
  size_t  dist     = 1;

  while(1)
    {
      /* Find the node from the table. */
      HASH_FIND_STR(graph, src, lsa_info);

      if(!lsa_info) return NULL; // This is an error, handle properly.

      /* Mark this nodes as visited */
      lsa_info->visited = 1;

      /* first server we see, is the nearest server */
      if(lsa_info->server) return lsa_info;  // cleanup PQ.

      /* Add its neighbors to the PQ. */
      for(size_t i = 0; i < lsa_info->num_nbors; i++)
        {
          HASH_FIND_STR(graph, lsa_info->nbors[i], visitcheck);

          if(!visitcheck) return NULL; // handle error properly.

          /* Do not add nodes that we've visited before */
          if(!visitcheck->visited)
            push(h, dist, lsa_info->nbors[i]);
        }
    }

  src  = pop(h);
  dist++;
}