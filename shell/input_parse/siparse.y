%{
	#include <errno.h>
	#include <siparse.h>
	#include <stdio.h>

	#include "siparseutils.h"
    
	extern int yyleng;
	int parse_flags;

	int yylex(void);
	void yyerror(char *);
  
	void switchinputbuftostring(const char *);
	void freestringinputbuf(void);

	pipelineseq * parsed_line;
%}


%union{
	int flags;
	char *name;
	argseq *args;
	redir *redir;
	redirseq *redirseq;
	command *comm;
	pipeline * pipeline;
	pipelineseq * pipelineseq;
}

%token SSTRING
%token OAPPREDIR
%token COMMENT
%%

line:
	pipelineseq mpipelinesep mcomment mendl {
			if ($2.flags==INBACKGROUND){
				if  (push_last_to_bg($1.pipelineseq) != OK) {/* unexpected */ YYABORT;};
			}
			
			parsed_line = $1.pipelineseq;
			$$ = $1;
		}
	;

mcomment:
	COMMENT
	|
	;

mendl:
	'\n'
	|
	;

mpipelinesep:
	pipelinesep   	{$$.flags = $1.flags;}
	|		{$$.flags = 0;}
	;


pipelineseq:
	pipelineseq pipelinesep pipeline{
			if ($2.flags==INBACKGROUND) push_last_to_bg($1.pipelineseq);
			$$.pipelineseq = append_to_pipelineseq($1.pipelineseq, $3.pipeline);
			if (! $$.pipelineseq) { parse_flags|=ALLOCFAILED; YYABORT;};
		}
	| pipeline{
			$$.pipelineseq = start_pipelineseq($1.pipeline);
			if (! $$.pipelineseq) { parse_flags|=ALLOCFAILED; YYABORT;};
		}
	;

pipelinesep:
	';'	{$$.flags = 0;}
	| '&'   {$$.flags = INBACKGROUND;};
	;


pipeline:
	pipeline '|' single {
			$$.pipeline = append_to_pipeline($1.pipeline, $3.comm);
			if (! $$.pipeline) { parse_flags|=ALLOCFAILED; YYABORT;};
		}
	| single {
			$$.pipeline = start_pipeline($1.comm);
			if (! $$.pipeline) { parse_flags|=ALLOCFAILED; YYABORT;};
		}
	;

single:
	mnames mredirs {
			if ($1.args==NULL) {
				$$.comm= NULL;	
			} else {
				command *com= new_command();
				if (!com) { parse_flags|=ALLOCFAILED; YYABORT;};
				com->args = $1.args;
				com->redirs = $2.redirseq;
				$$.comm = com;
			}
		}
	;


mnames:
	names
	|
	;

names:
	names name {
			$$.args = append_to_args($1.args, $2.name);
			if (! $$.args) { parse_flags|=ALLOCFAILED; YYABORT;};
		} 
	| name {
			$$.args= start_args($1.name);
			if (! $$.args) { parse_flags|=ALLOCFAILED; YYABORT;};
		}	 
	;

name:	SSTRING {
			$$.name= copytobuffer(yyval.name, yyleng+1);
			if (! $$.name) { parse_flags|=ALLOCFAILED; YYABORT;};
		};


mredirs:
	redirs
	|
	;

redirs:
	redirs redir {
			$$.redirseq = append_to_redirs($1.redirseq, $2.redir);
			if (! $$.redirseq) { parse_flags|=ALLOCFAILED; YYABORT;};
		}
	| redir	{
			$$.redirseq = start_redirs($1.redir); 
			if (! $$.redirseq) { parse_flags|=ALLOCFAILED; YYABORT;};
		};
	;

redir:
	redirIn
	| redirOut
	;

redirIn:
	'<' rname { $2.redir->flags = RIN; $$=$2; }
	;

redirOut:
	OAPPREDIR rname		{ $2.redir->flags = ROUT | RAPPEND ; $$=$2; }
	| '>' rname		{ $2.redir->flags = ROUT; $$=$2; }
	;

rname:
	 name {
			redir * red;

			red=new_redir();
			if (!red) { parse_flags|=ALLOCFAILED; YYABORT;};
			red->filename = $1.name;
			$$.redir= red;
		}
	;




%%

void yyerror(char *s) {
	parse_flags|=YYERRORFLAG;
}


pipelineseq * parseline(char *str){
	int parseresult;

	parse_flags=0;
	resetutils();
	switchinputbuftostring(str);
	parseresult = yyparse();
	freestringinputbuf();

	if (parseresult){
		errno = parse_flags;
		return NULL;
	}
	return parsed_line;
}

