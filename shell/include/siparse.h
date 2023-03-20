#ifndef _SIPARSE_H_
#define _SIPARSE_H_

#define OK 0
#define FAIL 1
/*
 * redirection flags
 */
#define RIN 	1
#define ROUT 	(1<<1)
#define RAPPEND	(1<<2)

#define RTMASK (RIN|ROUT|RAPPEND)
#define IS_RIN(x)		 (((x)&RTMASK) == RIN )
#define IS_ROUT(x)		 (((x)&RTMASK) == ROUT )
#define IS_RAPPEND(x)	 (((x)&RTMASK) == (ROUT|RAPPEND) )


/*
 * flags for pipeline
 */
#define INBACKGROUND 	1

/* structures */
typedef struct argseq {
	char * arg;
	struct argseq * next;
	struct argseq * prev;
} argseq;

typedef struct redir{
	char *filename;
	int flags;
} redir;

typedef struct redirseq {
	redir * r;
	struct redirseq * next;
	struct redirseq * prev;
} redirseq;

typedef struct command {
	argseq * args;
	redirseq * redirs;
} command;

typedef struct commandseq {
	command * com;
	struct commandseq * next;
	struct commandseq * prev;
} commandseq;

typedef struct pipeline {
	commandseq * commands;
	int flags;
} pipeline;

typedef struct pipelineseq {
	pipeline * pipeline;
	struct pipelineseq * next;
	struct pipelineseq * prev;
} pipelineseq;

/*
 * Parses given string containing sequence of pipelines separated by ';'. 
 * Each pipeline is a sequence of commands separated by '|'.
 * Function returns a pointer to the static structure pipelineseq or NULL if meets an error.
 * All structures referenced from the result of the function are statically allocated and shall not be freed.
 * Passing a string longer than MAX_LINE_LENGHT may result in an unspecified behaviour.
 * Consecutive calls to the function destroy the content of previously returned structures.
 */
pipelineseq * parseline(char *);

/* parse flags returned in errno when 'parseline' returns NULL */
#define ALLOCFAILED 1
#define YYERRORFLAG 2

#endif /* !_SIPARSE_H_ */
