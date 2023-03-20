
void resetutils(void);

/*
 * buffer for string from the parsed line
 */

char * copytobuffer(const char *, const short);
void resetbuffer(void);



/* argseq */
argseq * start_args(char * name);
argseq * append_to_args(argseq * args, char * name);

/* redir */
redir * new_redir(void);

/* redirseq */
redirseq * start_redirs(redir * initial_redir);
redirseq * append_to_redirs(redirseq * redirs, redir * r);
		
/* command */
command * new_command(void);


/* pipeline */
pipeline * start_pipeline(command * initial_command);
pipeline * append_to_pipeline(pipeline * ppl, command * comm);

/* pipelineseq */

pipelineseq * start_pipelineseq(pipeline * initial_pipeline);
pipelineseq * append_to_pipelineseq(pipelineseq * psq, pipeline * pp);
int push_last_to_bg(pipelineseq *);
