#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "siparse.h"
#include "siparseutils.h"


/* Static pool of structures for parsing shell commands. */
/* (overkill allocation) */

#define MAX_ARGS (MAX_LINE_LENGTH/2)
#define MAX_REDIRS (MAX_LINE_LENGTH/2)
#define MAX_REDIRSEQS MAX_REDIRS
#define MAX_COMMANDS (MAX_LINE_LENGTH/2)
#define MAX_COMMANDSEQS MAX_COMMANDS
#define MAX_PIPELINES MAX_COMMANDS
#define MAX_PIPELINESEQS MAX_PIPELINES

argseq argseq_pool[MAX_ARGS];
redir redir_pool[MAX_REDIRS];
redirseq redirseq_pool[MAX_REDIRSEQS];
command command_pool[MAX_COMMANDS];
commandseq commandseq_pool[MAX_COMMANDSEQS];
pipeline pipeline_pool[MAX_PIPELINES];
pipelineseq pipelineseq_pool[MAX_PIPELINESEQS];

int next_argseq, next_redir, next_redirseq, next_pipeline, next_pipelineseq,  next_command, next_commandseq;

void resetutils(void){
	next_argseq =next_redir =next_redirseq =next_pipeline =next_pipelineseq =next_command =next_commandseq=0;
	resetbuffer();
}

/* static getters */

static argseq * get_argseq(void){
	if (next_argseq==MAX_ARGS)
        return NULL; // no mem
	argseq * new_seq = argseq_pool+next_argseq;
	next_argseq++;
	return new_seq;
}

static redirseq * get_redirseq(void){
	if (next_redirseq==MAX_REDIRSEQS)
        return NULL; // no mem
	redirseq * new_seq = redirseq_pool+next_redirseq;
	next_redirseq++;
	return new_seq;
}

static commandseq * get_commandseq(void){
	if (next_commandseq==MAX_COMMANDSEQS)
        return NULL; // no mem
	commandseq * new_commandseq = commandseq_pool+next_commandseq;
	next_commandseq++;
	return new_commandseq;
}


static pipeline * get_pipeline(void){
	if (next_pipeline==MAX_PIPELINES)
        return NULL; // no mem
	pipeline * new_pipeline = pipeline_pool+next_pipeline;
	next_pipeline++;
	return new_pipeline;
}

static pipelineseq * get_pipelineseq(void){
	if (next_pipelineseq==MAX_PIPELINESEQS)
        return NULL; // no mem
	pipelineseq * new_pipelineseq = pipelineseq_pool+next_pipelineseq;
	next_pipelineseq++;
	return new_pipelineseq;
}

/* argseq */

argseq * start_args(char * name){
	argseq * new_seq = get_argseq();
	if (!new_seq)
        return NULL;
	new_seq->arg 	= name;
	new_seq->next 	= new_seq;
	new_seq->prev 	= new_seq;
	return new_seq;
}

argseq * append_to_args(argseq * args, char * name){
	argseq * new_seq = get_argseq();
	if (!new_seq)
        return NULL;
	new_seq->arg = name;
	new_seq->next = args;
	new_seq->prev = args->prev;
	args->prev->next = new_seq;
	args->prev= new_seq;
	return args;	
}

/* redir */

redir * new_redir(void){
	if (next_redir==MAX_REDIRS)
        return NULL; // no mem
	redir * rd  = redir_pool+next_redir;
	next_redir++;
	return rd;
}

/* redirseq */
redirseq * start_redirs(redir * initial_redir){
	redirseq * new_seq = get_redirseq();
	if (!new_seq)
        return NULL;
	new_seq->r 	= initial_redir;
	new_seq->next 	= new_seq;
	new_seq->prev 	= new_seq;
	return new_seq;
}
redirseq * append_to_redirs(redirseq * redirs, redir * next_redir){
	redirseq * new_seq = get_redirseq();
	if (!new_seq)
        return NULL;
	new_seq->r = next_redir;
	new_seq->next = redirs;
	new_seq->prev = redirs->prev;
	redirs->prev->next = new_seq;
	redirs->prev= new_seq;
	return redirs;	
}

/* command */

command * new_command(void){
	if (next_command==MAX_COMMANDS)
        return NULL; // no mem
	command * new_com  = command_pool+next_command;
	next_command++;
	return new_com;
}

/* commandseq */
commandseq * start_commandseq(command * initial_command){
	commandseq * new_csq = get_commandseq();
	new_csq->com = initial_command;
	new_csq->next= new_csq;
	new_csq->prev= new_csq;
	return new_csq;
}

commandseq * append_to_commandseq(commandseq * csq, command * com){
	commandseq * new_csq = get_commandseq();
	if (!new_csq)
        return NULL;
	new_csq->com = com;

	new_csq->next= csq;
	new_csq->prev= csq->prev;
	csq->prev->next= new_csq;
	csq->prev= new_csq;
	return new_csq;
}

/* pipeline */
pipeline * start_pipeline(command * initial_command){	
	pipeline * new_ppl = get_pipeline();
	commandseq * com_seq = start_commandseq(initial_command);
	if ((new_ppl==NULL) || (com_seq==NULL))
        return NULL;
	new_ppl->commands = com_seq;
	new_ppl->flags = 0;
	return new_ppl;
}
pipeline * append_to_pipeline(pipeline * ppl, command * comm){
	append_to_commandseq(ppl->commands, comm);
	return ppl;
}

/* pipelineseq */

pipelineseq * start_pipelineseq(pipeline * initial_pipeline){
	pipelineseq * new_psq = get_pipelineseq();
	if (new_psq==NULL)
        return NULL;
	new_psq->pipeline = initial_pipeline;
	new_psq->next= new_psq;
	new_psq->prev= new_psq;
	return new_psq;
}


pipelineseq * append_to_pipelineseq(pipelineseq * psq, pipeline * pp){
	pipelineseq * new_psq = get_pipelineseq();
	if (new_psq==NULL)
        return NULL;

	new_psq->pipeline = pp;
	new_psq->next= psq;
	new_psq->prev= psq->prev;
	psq->prev->next= new_psq;
	psq->prev= new_psq;
	return psq;
}

int push_last_to_bg(pipelineseq * psq){
	if ((!psq) || (!psq->prev) || (!psq->prev->pipeline))
        return FAIL;
	psq->prev->pipeline->flags |= INBACKGROUND;
	return OK; //OK
}




//----------------------------------------------------------------------------------------------------------
/*
 * buffer for strings from the parsed line
 */
static char linebuffer[MAX_LINE_LENGTH+1];
static const char *linebufferend= linebuffer+MAX_LINE_LENGTH;
static char *bufptr=linebuffer;

/*
 * name is 0 terminated string
 * length include terminating 0
 */
char * copytobuffer(const char *name, const short length){
	char *saved;
	int copied;

	saved=bufptr;
	bufptr+=length;
	if (bufptr > linebufferend)
        return NULL;
	strncpy(saved, name, length);
	return saved;
}

void resetbuffer(void){
	bufptr= linebuffer;
}

