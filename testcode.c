#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define N 4
#define NxN (N*N)
#define TRUE 1
#define FALSE 0

struct node {
	int tiles[N][N];
	int f, g, h;
	short zero_r, zero_c;	/* location (row and colum) of blank tile 0 */
	struct node *next;
	struct node *parent;			/* used to trace back the solution */
};

int goal_r[NxN];
int goal_c[NxN];
struct node *start,*goal;
struct node *open = NULL, *closed = NULL;
struct node *succ_nodes[4];

int finish=0, multithread=0;

void print_a_node(struct node *pnode) {
	int i,j;
	for (i=0;i<N;i++) {
		for (j=0;j<N;j++) 
			printf("%d ", pnode->tiles[i][j]);
		printf("\n");
	}
	printf("\n");
}

struct node *initialize(int argc, char **argv){
	int i,j,k,index, tile;
	struct node *pnode;

	pnode=(struct node *) malloc(sizeof(struct node));
	index = 1;
	for (j=0;j<N;j++)
		for (k=0;k<N;k++) {
			tile=atoi(argv[index++]);
			pnode->tiles[j][k]=tile;
			if(tile==0) {
				pnode->zero_r=j;
				pnode->zero_c=k;
			}
		}
	pnode->f=0;
	pnode->g=0;
	pnode->h=0;
	pnode->next=NULL;
	pnode->parent=NULL;
	start=pnode;
	printf("initial state\n");
	print_a_node(start);

	pnode=(struct node *) malloc(sizeof(struct node));
	goal_r[0]=3;
	goal_c[0]=3;

	for(index=1; index<NxN; index++){
		j=(index-1)/N;
		k=(index-1)%N;
		goal_r[index]=j;
		goal_c[index]=k;
		pnode->tiles[j][k]=index;
	}
	pnode->tiles[N-1][N-1]=0;	      /* empty tile=0 */
	pnode->f=0;
	pnode->g=0;
	pnode->h=0;
	pnode->next=NULL;
	goal=pnode; 
	printf("goal state\n");
	print_a_node(goal);

	return start;
}

void insert(struct node **list, struct node *pnode){
	if(!(*list)){
		*list = pnode;
		(*list)->next = NULL;
		return;
	}
	if(pnode->f <= (*list)->f){
		pnode->next = *list;
		*list = pnode;
		return;
	}

	struct node* current = *list;
	while(current->next){
		if(pnode->f <= current->next->f) break;
		current = current->next;
	}
	pnode->next = current->next;
	current->next = pnode;
}

/* merge unrepeated nodes into open list after filtering */
void merge_to_open() { 
	int i;
	for(i=0; i<4; i++){
		if(succ_nodes[i]->parent!=NULL)
			insert(&open, succ_nodes[i]);
	}
}

/*swap two tiles in a node*/
void swap(int row1,int column1,int row2,int column2, struct node * pnode){
	int tile = pnode->tiles[row1][column1];
	pnode->tiles[row1][column1]=pnode->tiles[row2][column2];
	pnode->tiles[row2][column2]=tile;
}

int h(struct node *pnode){
	int i, j, tile, sum = 0;
	for (i=0;i<N;i++) {
		for (j=0;j<N;j++){
			tile = pnode->tiles[i][j];
			if(tile == 0) continue;
			sum += abs(goal_r[tile] - i);
			sum += abs(goal_c[tile] - j);
		}
	}
	return sum;
}
/*update the f,g,h function values for a node */
void update_fgh(struct node *pnode){
	pnode->h = h(pnode);
	pnode->g = pnode->parent->g + 1;
	pnode->f = pnode->h + pnode->g;
}

/* 0 goes down by a row */
void move_down(struct node * pnode){
	swap(pnode->zero_r, pnode->zero_c, pnode->zero_r+1, pnode->zero_c, pnode); 
	pnode->zero_r++;
}

/* 0 goes right by a column */
void move_right(struct node * pnode){
	swap(pnode->zero_r, pnode->zero_c, pnode->zero_r, pnode->zero_c+1, pnode); 
	pnode->zero_c++;
}

/* 0 goes up by a row */
void move_up(struct node * pnode){
	swap(pnode->zero_r, pnode->zero_c, pnode->zero_r-1, pnode->zero_c, pnode); 
	pnode->zero_r--;
}

/* 0 goes left by a column */
void move_left(struct node * pnode){
	swap(pnode->zero_r, pnode->zero_c, pnode->zero_r, pnode->zero_c-1, pnode); 
	pnode->zero_c--;
}

/* expand a node, get its children nodes, and organize the children nodes using
 * array succ_nodes.
 */
void expand(struct node *selected) {
	int i;
	int canMove[N];
	canMove[0] = (selected->zero_r>0);
	canMove[1] = (selected->zero_r<N-1);
	canMove[2] = (selected->zero_c>0);
	canMove[3] = (selected->zero_c<N-1);

	for(i=0;i<4;i++){
		succ_nodes[i] = (struct node *) malloc(sizeof(struct node));
		if(!canMove[i]){
			succ_nodes[i]->parent = NULL;
			continue;
		}
		memcpy(succ_nodes[i], selected, sizeof(struct node));
		succ_nodes[i]->parent = selected;
		switch(i){
			case 0:
				move_up(succ_nodes[i]);
				break;
			case 1:
				move_down(succ_nodes[i]);
				break;
			case 2:
				move_left(succ_nodes[i]);
				break;
			case 3:
				move_right(succ_nodes[i]);
				break;
		}
	update_fgh(succ_nodes[i]);
	} 
}

int nodes_same(struct node *a,struct node *b) {
	int flg=FALSE;
	if (memcmp(a->tiles, b->tiles, sizeof(int)*NxN) == 0)
		flg=TRUE;
	return flg;
}

/* Filtering. Some nodes in succ_nodes may already be included in either open 
 * or closed list. Remove them. It is important to reduce execution time.
 * This function checks the (i)th node in succ_nodes array. You must call this
 & function in a loop to check all the nodes in succ_nodes.
 */ 
void filter(int i, struct node *pnode_list){ 
	if(succ_nodes[i]->parent == NULL)
		return;

	struct node *pnode = succ_nodes[i];
	struct node* current = pnode_list;

	while(current!=NULL){
		if(nodes_same(current, pnode)){
			pnode->parent = NULL;
			return;
		}
	current = current->next;
	}
}

void *filter_threads(void *id){
	int *myid = (int *)id;
	printf("thread %d\n",*myid); 
	while(1){
        //... /* barrier sync */
        //... /* check the found flag, and exit when found is set */
		filter(*myid, open);
		filter(*myid, closed);
        //... /* barrier sync */
	}
}

int main(int argc,char **argv) {
	int iter,cnt;
	struct node *copen, *cp, *solution_path;
	pthread_t thread[N-1];
	int ret, i, pathlen=0, index[N-1];

	solution_path=NULL;

	/* set multithread flag based on argv[1] */
	//if(!strcmp(argv[1], "-s")) multithread = FALSE;
	//else if (!strcmp(argv[1], "-m")) multithread = TRUE;
	//else {printf("Invalid option: %s\n", argv[1]); return 0;}

	start=initialize(argc,argv);	/* init initial and goal states */
	open=start; 
	if(multithread){
		//... /* initialize barriers */
		//... /* create threads */
	}

	iter=0; 
	while (open!=NULL) {	/* Termination cond with a solution is tested in expand. */
		copen=open;
		open=open->next;  /* get the first node from open to expand */
		if(nodes_same(copen,goal)){ /* goal is found */
			if(multithread){
				finish=1;
				/* barrier sync to allow other threads to return 
                                 * from their barrier calls and exit*/
				//...
			}
			do{ /* trace back and add the nodes on the path to a list */
				copen->next=solution_path;
				solution_path=copen;
				copen=copen->parent;
				pathlen++;
			} while(copen!=NULL);
			printf("Path (lengh=%d):\n", pathlen); 
			copen=solution_path;
			/* print out the nodes on the list */
			while (copen != NULL){
				print_a_node(copen);
				copen = copen->next;
			}
			break;
		}
		expand(copen);       /* Find new successors */
		if(multithread){
			//... /* barrier sync */
			filter(0,open);
			filter(0,closed);
			//... /* barrier sync */
		}
		else{
			for(i=0;i<4;i++){
				filter(i,open);
				filter(i,closed);
			}
		}
		merge_to_open(); /* New open list */
		copen->next=closed;
		closed=copen;		/* New closed */
		/* print out something so that you know your 
		 * program is still making progress 
		 */
		iter++;
		if(iter %1000 == 0)
			printf("iter %d\n", iter);
	}

	if(multithread){
		//...  /* destroy barriers */
		//...  /* join threads */
	}
	return 0;
} /* end of main */
