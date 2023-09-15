#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define PROC_DIR "/proc"
#define PROC_MAX 32768
#define PROC_SON 100
struct Proc {
	int pid;
	int ppid;
	char name[100];
} procs[PROC_MAX];

struct ProcessTree {
	int size;
	struct Proc *proc;
	struct ProcessTree *sons[PROC_SON];
} *process_tree;

void scan();
void build();
void sort();
void print(int pid);

int main(int argc, char *argv[]) {
	int pid=0, numeric_sort=0, version=0;
	int opt_char;
	int option_index;
	struct option long_options[4]= {
		{"show-pids", no_argument, 0, 'p'},
		{"numeric-sort", no_argument, 0, 'n'},
		{"version", no_argument, 0, 'V'}
	};
	while ((opt_char=getopt_long(argc, argv, "pnV", long_options, &option_index))!=-1) {
		switch (opt_char) {
			case 'p':
				pid=1;
				break;
			case 'n':
				numeric_sort=1;
				break;
			case 'V':
				version=1;
				break;
			default:
				break;
		}
	};
	
	if (version) {
		printf(
			"pstree 1.0\n"
			"Copyright (C) 2022 CookiePie\n"
		);
	} else {
		scan();
		build();
		if (numeric_sort==1)
			sort();
		print(pid);
	}
	return 0;
}

int is_number_(const char *d_name) {
	int i;
	for (i=0; i<256; ++i) {
		if (d_name[i]==0)
			break;
		else if (d_name[i]>'9' || d_name[i]<'0')
			return 0;
	}
	return 1;
}

void read_status_(const char *d_name, int i) {
	// pid
	procs[i].pid=atoi(d_name);
	// /proc/[pid]/status
	char dir_path[100]="";
	sprintf(dir_path, "/proc/%s/status", d_name);
	FILE *fp=fopen(dir_path, "r");
	// read status
	int j;
	char line[100] = {0};
	for (j=0; j<10; ++j) {
		char* e = fgets(line, 100, fp);
        if (!e) {
            exit(-1);
        }
		switch(j) {
			case 0:
				strcpy(procs[i].name, &line[6]);
				procs[i].name[strlen(procs[i].name) - 1] = '\0';
				break;
			case 6:
				procs[i].ppid=atoi(&line[6]);
				break;
		}
	}
	return;
}

void scan() {
	int i=0;
	DIR *proc_dir = opendir(PROC_DIR);
	struct dirent *dir;
	while ((dir=readdir(proc_dir)) != NULL) {
		if (!is_number_(dir->d_name))
			continue;
		read_status_(dir->d_name, i++);
	}
	closedir(proc_dir);
	procs[i].pid=0;
	strcpy(procs[i].name, "?");
	process_tree=(struct ProcessTree *)malloc(sizeof(struct ProcessTree));
	process_tree->proc=&procs[i];
	int k;
	for (k=0; k<PROC_SON; ++k)
		process_tree->sons[k]=NULL;
}

void build_node_(struct ProcessTree *node) {
	int i, j=0;
	for (i=0; i<PROC_MAX && j<PROC_SON; ++i) {
		if (procs[i].pid==0)
			break;
		if (procs[i].ppid==node->proc->pid) {
			node->sons[j]=(struct ProcessTree *)malloc(sizeof(struct ProcessTree));
			int k;
			for (k=0; k<PROC_SON; ++k)
				node->sons[j]->sons[k]=NULL;
			node->sons[j]->size=0;
			node->sons[j]->proc=&procs[i];
			build_node_(node->sons[j]);
			++j;
		}
	}
	node->size=j;
}

void build() {
	build_node_(process_tree);
}

int bigger_(struct ProcessTree *a, struct ProcessTree *b) {
	int i=0;
	while (i<100) {
		if (a->proc->name[i]==0) {
			if (b->proc->name[i]==0)
				return 1;
			return 0;
		}
		if (b->proc->name[i]==0)
			return 1;
		if (a->proc->name[i]<b->proc->name[i])
			return 1;
		if (a->proc->name[i]>b->proc->name[i])
			return 0;
		i++;
	}
	return 0;
}

void sort_node_(int l, int r, struct ProcessTree *node) {
	if (l>=r)
		return;
	int ll=l, rr=r;
	struct ProcessTree *key=node->sons[l];
	while (ll<rr) {
		while (ll<rr && bigger_(key, node->sons[rr]))
			rr--;
		node->sons[ll]=node->sons[rr];
		while (ll<rr && bigger_(node->sons[ll], key))
			ll++;
		node->sons[rr]=node->sons[ll];
	}
	node->sons[ll]=key;
	sort_node_(ll+1, r, node);
	sort_node_(l, ll-1, node);
}

void sort_(struct ProcessTree *node) {
	sort_node_(0, node->size-1, node);
	for (int i=0; i<node->size; ++i)
		sort_(node->sons[i]);
}

void sort() {
	sort_(process_tree);
}

int digits_(int i) {
	if (i==0)
		return 1;
	int j=0;
	while (i) i/=10, j++;
	return j;
}

void print_layer_(struct ProcessTree *node, int start, int sib, int pid, int space, char *line) {
    line[space] = line[space + 1] = line[space + 2] = line[space + 3] = line[space + 4] = ' ';
	if (start) {
		if (sib) {
			printf("─┬─");
			sprintf(&line[space+1], "%s", "│");
            space += 3;
		} else {
			printf("───");
		}
	} else {
		int l;
		for (l=0; l<space; ++l) {
			putchar(line[l]);
		}
		if (sib) {
			printf(" ├─");
			sprintf(&line[space+1], "%s", "│");
            space += 3;
		}
		else
			printf(" └─");
	}
	int i=0;
	if (pid) {
		space+=5+strlen(node->proc->name)+digits_(node->proc->pid);
		printf("%s(%d)", node->proc->name, node->proc->pid);
	} else {
		space+=3+strlen(node->proc->name);
		printf("%s", node->proc->name);
	}
	if (!node->sons[0]) {
		putchar('\n');
		return;
	}
	if (node->sons[1])
		print_layer_(node->sons[i++], 1, 1, pid, space, line);
	else {
		print_layer_(node->sons[i++], 1, 0, pid, space, line);
		return;
	}
	while (node->sons[i+1])
		print_layer_(node->sons[i++], 0, 1, pid, space, line);
	print_layer_(node->sons[i], 0, 0, pid, space, line);
}

void print(int pid) {
	int space=1;
	if (pid) {
		printf("?(0)");
		space=4;
	} else {
		printf("?");
	}
	int i=0;
	char line[1000];
	for (int i=0; i<1000; ++i)
		line[i]=' ';
	print_layer_(process_tree->sons[i++], 1, 1, pid, space, line);
	while (process_tree->sons[i+1])
		print_layer_(process_tree->sons[i++], 0, 1, pid, space, line);
    line[space] = line[space + 1] = line[space + 2] = line[space + 3] = line[space + 4] = ' ';
    if (pid) {
		space = 4;
	} else {
		space = 1;
	}
	print_layer_(process_tree->sons[i], 0, 0, pid, space, line);
	putchar('\n');
}
