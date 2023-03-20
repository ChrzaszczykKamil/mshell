#include <stdio.h>

#include "config.h"
#include "siparse.h"

void printcommand(command *pcmd, int k)
{
	int flags;

	printf("\tCOMMAND %d\n",k);
	if (pcmd==NULL){
		printf("\t\t(NULL)\n");
		return;
	}

	printf("\t\targv=:");
	argseq * argseq = pcmd->args;
	do{
		printf("%s:", argseq->arg);
		argseq= argseq->next;
	}while(argseq!=pcmd->args);

	printf("\n\t\tredirections=:");
	redirseq * redirs = pcmd->redirs;
	if (redirs){
		do{	
			flags = redirs->r->flags;
			printf("(%s,%s):",redirs->r->filename,IS_RIN(flags)?"<": IS_ROUT(flags) ?">": IS_RAPPEND(flags)?">>":"??");
			redirs= redirs->next;
		}while(redirs!=pcmd->redirs);
	}

	printf("\n");
}

void printpipeline(pipeline * p, int k)
{
	int c;
	command ** pcmd;

	commandseq * commands= p->commands;

	printf("PIPELINE %d\n",k);
	
	if (commands==NULL){
		printf("\t(NULL)\n");
		return;
	}
	c=0;
	do{
		printcommand(commands->com,++c);
		commands= commands->next;
	}while(commands!=p->commands);

	printf("Totally %d commands in pipeline %d.\n",c,k);
	printf("Pipeline %sin background.\n", (p->flags & INBACKGROUND) ? "" : "NOT ");
}

void printparsedline(pipelineseq * ln)
{
	int c;
	pipelineseq * ps = ln;

	if (!ln){
		printf("%s\n",SYNTAX_ERROR_STR);
		return;
	}
	c=0;

	do{
		printpipeline(ps->pipeline,++c);
		ps= ps->next;
	}while(ps!=ln);

	printf("Totally %d pipelines.",c);
	printf("\n");
}

command * pickfirstcommand(pipelineseq * ppls)
{
	if ((ppls==NULL)||(ppls->pipeline==NULL)||(ppls->pipeline->commands==NULL)||(ppls->pipeline->commands->com==NULL))
        return NULL;
	
	return ppls->pipeline->commands->com;
}
