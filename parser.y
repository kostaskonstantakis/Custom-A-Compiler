%{
	#include <fstream>
	#include <cstdio>
	#include <iostream>
	#include <vector>
	#include "phase4.h"
	//#define YY_DECL int alpha_yyparse(void);
	
	extern int yylex(void);

	//using namespace std;
	unsigned int scope=0; //global scope

	std::vector<struct SymbolTableEntry*> ST;

	extern int yylineno;
     	//extern int* yylval; //void* doesn't compile
        extern char * yytext;
        extern FILE * yyin;
        extern FILE * yyout;
		void yyerror(const char* yaccProvidedMessage)
		{
       			std::cerr<<yaccProvidedMessage<<" at line "<<yylineno<<", before token: '"<<yytext<<"'\n";
       			std::cerr<<"INPUT INVALID."<<std::endl;
		}
	int anonymous_function_counter = 0;
	int loopcounter = 0;
%}

%defines 

%start program 
%token <stringValue> ID
%token <intValue> DECIMAL
%token <realValue> FLOAT
%token <stringValue> STRING
%token IF
%token ELSE
%token WHILE
%token FOR
%token FUNCTION
%token RETURN
%token BREAK
%token CONTINUE
%token AND
%token NOT
%token OR
%token LOCAL
%token TRUE
%token FALSE
%token NIL

%token ASSIGN 
%token PLUS 
%token MINUS
%token MULTIPLY
%token DIVIDE
%token MODULO 
%token EQUAL 
%token NOT_EQUAL 
%token PLUS_PLUS 
%token MINUS_MINUS 
%token GREATER_THAN 
%token LESS_THAN 
%token GREATER_OR_EQUAL 
%token LESS_OR_EQUAL 

%token OPEN_BRACKET 
%token CLOSE_BRACKET 
%token OPEN_SQUARE_BRACKET 
%token CLOSE_SQUARE_BRACKET 
%token OPEN_PARENTHESIS
%token CLOSE_PARENTHESIS 
%token SEMICOLON 
%token COLON 
%token DOUBLE_COLON 
%token COMMA
%token DOT 
%token DOUBLE_DOT 

%token COMMENT 
%token BLOCK_COMMENT 

%right ASSIGN
%left OR
%left AND
%nonassoc EQUAL NOT_EQUAL
%nonassoc GREATER_THAN GREATER_OR_EQUAL LESS_THAN LESS_OR_EQUAL
%left PLUS MINUS
%left MULTIPLY DIVIDE MODULO
%right NOT PLUS_PLUS MINUS_MINUS UMINUS
%left DOT DOUBLE_DOT
%left OPEN_SQUARE_BRACKET CLOSE_SQUARE_BRACKET
%left OPEN_PARENTHESIS CLOSE_PARENTHESIS 

%type <exprValue> lvalue term expr assignexpr exprs primary objectdef const
%type <exprValue> member elist indexed call tableitem forprefix forstmt returnstmt indexedelems
%type <symbolTableValue> idlist ifstmt whilestmt funcdef funcprefix
%type <callValue> callsuffix normcall methodcall
%type <stringValue> funcname
%type <intValue> funcbody ifprefix  elseprefix whilestart whilecond M N loopstart loopend
%type <pairValue> indexedelem 
%type <stmtValue> stmt stmts block loopstmt

%union {
	struct expr* exprValue;
	struct call* callValue;
	char* stringValue;
	int intValue;
	double realValue;
	struct SymbolTableEntry* symbolTableValue;
	std::pair<expr*,expr*> *pairValue;
	struct stmt_t* stmtValue;
}


%% 

program: stmts {cout<<"program -> stmts in line "<<yylineno<<endl;}
	;

stmt: 	expr SEMICOLON {cout<<"stmt -> expr; in line "<<yylineno<<endl; $$=NULL;}
      	| ifstmt {cout<<"stmt -> ifstmt in line "<<yylineno<<endl;$$=NULL;} 
      	| whilestmt {cout<<"stmt -> whilestmt in line "<<yylineno<<endl;$$=NULL;}
      	| forstmt {cout<<"stmt -> forstmt in line "<<yylineno<<endl;$$=NULL;}
	| returnstmt {cout<<"stmt -> returnstmt in line "<<yylineno<<endl;$$=0;}
	| BREAK SEMICOLON {
		if(loopcounter && currscopespace()!=functionlocal){
			$$ = (stmt_t*)malloc(sizeof(stmt_t));
			make_stmt($$);
			$$->breakList = newList(nextquad()); 
			emit(jump, NULL, NULL, 0,0, yylineno);
			cout<<"stmt -> break; in line "<<yylineno<<endl;
		}else{
			yyerror("Break outside of loop\n");
		}
	}
	| CONTINUE SEMICOLON {
		if(loopcounter && currscopespace()!=functionlocal){
			$$ = (stmt_t*)malloc(sizeof(stmt_t));
			make_stmt($$);
			$$->contList = newList(nextquad()); 
			emit(jump, NULL, NULL, 0,0, yylineno);
			cout<<"stmt -> continue; in line "<<yylineno<<endl;
		}else{
			yyerror("Continue outside of loop\n");
		}
	}
	| block {cout<<"stmt -> block in line "<<yylineno<<endl;$stmt=$block;}
	| funcdef {cout<<"stmt -> funcdef in line "<<yylineno<<endl;$$=0;}
	| SEMICOLON {cout<<"stmt -> ; in line "<<yylineno<<endl;$$=NULL;}
      	;

stmts: stmt stmts {
	resettemp();
	$$ = (stmt_t*)malloc(sizeof(stmt_t));
	make_stmt($$);
	
	if($1!=NULL && $2 == NULL){
		$$=$1;
	}else if($1==NULL && $2 != NULL){
		$$=$2;
	}else if($1==NULL && $2 == NULL){
		//tipota
	}else if($1!=NULL && $2 != NULL){
		$$->breakList = mergeList($1->breakList, $2->breakList);
		$$->contList = mergeList($1->contList, $2->contList);
	}


	//$$->breakList = newList(nextquad());
	//if($1!=NULL)$$->breakList = mergeList($1->breakList, $2->breakList);
	//else $$ = $2;
	cout<<"stmts -> stmt stmts in line "<<yylineno<<endl;}
    | {cout<<"stmts -> empty in line "<<yylineno<<endl; 
	  	resettemp();
		$$ = (stmt_t*)malloc(sizeof(stmt_t));
	  	make_stmt($$);
	  } 
      ; 

expr: 	assignexpr {cout<<"expr -> assignexpr in line "<<yylineno<<endl;}
	| expr PLUS expr {
		$$ = newexpr(arithexpr_e);
		$$->entry = newtemp();
		emit(add, $1, $3, $$, -1, yylineno);
		cout<<"expr -> expr + expr in line "<<yylineno<<endl; }
	| expr MINUS expr {
		$$ = newexpr(arithexpr_e);
		$$->entry = newtemp();
		emit(sub, $1, $3, $$, -1, yylineno);
		
		cout<<"expr -> expr - expr in line "<<yylineno<<endl; }
	| expr MULTIPLY expr {
		$$ = newexpr(arithexpr_e);
		$$->entry = newtemp();
		emit(mul, $1, $3, $$, -1, yylineno);
		
		cout<<"expr -> expr * expr in line "<<yylineno<<endl; }
	| expr DIVIDE expr  {
		$$ = newexpr(arithexpr_e);
		$$->entry = newtemp();
		emit(_div, $1, $3, $$, -1, yylineno);
		
		cout<<"expr -> expr / expr in line "<<yylineno<<endl; }
	| expr MODULO expr {

		$$ = newexpr(arithexpr_e);
		$$->entry = newtemp();
		emit(mod, $1, $3, $$, -1, yylineno);
		cout<<"expr -> expr % expr in line "<<yylineno<<endl; }
	| expr EQUAL expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(if_eq, $1, $3, NULL, nextquad()+3 ,  yylineno);
		emit(assign, newexpr_constbool(false),NULL,$$, -1, yylineno);
		emit(jump,NULL, NULL,NULL, nextquad() +2, yylineno);
		emit(assign, newexpr_constbool(true), NULL, $$, -1, yylineno);
		cout<<"expr -> expr == expr in line "<<yylineno<<endl; }
	| expr NOT_EQUAL expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(if_noteq, $1, $3, NULL, nextquad()+3 ,  yylineno);
		emit(assign, newexpr_constbool(false),NULL,$$, -1, yylineno);
		emit(jump,NULL, NULL,NULL, nextquad() +2, yylineno);
		emit(assign, newexpr_constbool(true), NULL, $$, -1, yylineno);
		cout<<"expr -> expr != expr in line "<<yylineno<<endl; }
	| expr GREATER_THAN expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(if_greater, $1, $3, NULL, nextquad()+3 ,  yylineno);
		emit(assign, newexpr_constbool(false),NULL,$$, -1, yylineno);
		emit(jump,NULL, NULL,NULL, nextquad() +2, yylineno);
		emit(assign, newexpr_constbool(true), NULL, $$, -1, yylineno);
		cout<<"expr -> expr > expr in line "<<yylineno<<endl; }
	| expr GREATER_OR_EQUAL expr {
	
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(if_greatereq, $1, $3, NULL, nextquad()+3 ,  yylineno);
		emit(assign, newexpr_constbool(false),NULL,$$, -1, yylineno);
		emit(jump,NULL, NULL,NULL, nextquad() +2, yylineno);
		emit(assign, newexpr_constbool(true), NULL, $$, -1, yylineno);
		cout<<"expr -> expr >= expr in line "<<yylineno<<endl; }
	| expr LESS_THAN expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(if_less, $1, $3, NULL, nextquad()+3 ,  yylineno);
		emit(assign, newexpr_constbool(false),NULL,$$, -1, yylineno);
		emit(jump,NULL, NULL,NULL, nextquad() +2, yylineno);
		emit(assign, newexpr_constbool(true), NULL, $$, -1, yylineno);
		cout<<"expr -> expr < expr in line "<<yylineno<<endl; }
	| expr LESS_OR_EQUAL expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(if_lesseq, $1, $3, NULL, nextquad()+3 ,  yylineno);
		emit(assign, newexpr_constbool(false),NULL,$$, -1, yylineno);
		emit(jump,NULL, NULL,NULL, nextquad() +2, yylineno);
		emit(assign, newexpr_constbool(true), NULL, $$, -1, yylineno);
		cout<<"expr -> expr <= expr in line "<<yylineno<<endl; }
	| expr AND expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(_and, $1, $3, $$, -1, yylineno);
		cout<<"expr -> expr and expr in line "<<yylineno<<endl; }
	| expr OR expr {
		$$ = newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(_or, $1, $3, $$, -1, yylineno);
		cout<<"expr -> expr or expr in line "<<yylineno<<endl; }
	| term {
		cout<<"expr -> term in line "<<yylineno<<endl;$$=$1;}
	;

term: 	OPEN_PARENTHESIS  expr CLOSE_PARENTHESIS {
		$$=$2;
		cout<<"term -> (expr) in line "<<yylineno<<endl;}
	| MINUS expr %prec UMINUS {
		
		check_arith($2, "UMINUS");
		$$ = newexpr(arithexpr_e);
		$$->entry = newtemp();
		emit(uminus, $2, newexpr_constnum(-1), $$, -1, yylineno);		//10,32
		
		cout<<"term -> -expr in line "<<yylineno<<endl;}
	| NOT expr {
		$$=newexpr(boolexpr_e);
		$$->entry = newtemp();
		emit(_not, $2, NULL, $$, -1, yylineno);		//10,32
		cout<<"term -> not expr in line "<<yylineno<<endl;}
	|PLUS_PLUS lvalue {
					if(CheckIfLibFunction2(yytext, ST, $2) || CheckIfUserFunction2(yytext, ST, $2)){
						yyerror("++ operator to Function\n");

					}
				check_arith($2, "lvalue++ ");
				if($2->type == tableitem_e){
					$$ = emit_iftableitem($2);
					emit(add, $$, newexpr_constnum(1), $$, -1, yylineno);
					emit(tablesetelem, $2,$2->index, $$, -1, yylineno);
				}else{
					emit(add, $2, newexpr_constnum(1), $2, -1, yylineno);
					$$ = newexpr(arithexpr_e);
					$$->entry = newtemp();
					emit(assign, $2, NULL, $$, -1, yylineno);
				}
				cout<<"term -> ++lvalue in line "<<yylineno<<endl;}
	| lvalue	{
					if(CheckIfLibFunction2(yytext, ST, $1) || CheckIfUserFunction2(yytext, ST, $1))
						yyerror("++ operator to Function\n");
				} PLUS_PLUS {
					check_arith($1, "lvalue++ ");
					$$ = newexpr(var_e);
					$$->entry = newtemp();
					if($1->type == tableitem_e){
						expr* val = emit_iftableitem($1);
						emit(assign, val, NULL, $$, -1, yylineno);
						emit(add, val, newexpr_constnum(1), val, -1, yylineno);
						emit(tablesetelem, $1,$1->index,val , -1, yylineno);
					}else{
						emit(assign,$1, NULL, $$, -1, yylineno);
						emit(add, $1, newexpr_constnum(1), $1, -1, yylineno);		//10.33
					}
					cout<<"term -> lvalue++ in line "<<yylineno<<endl;
				}
	|  MINUS_MINUS lvalue {
						if(CheckIfLibFunction2(yytext, ST, $2) || CheckIfUserFunction2(yytext, ST, $2))
							yyerror("-- operator to Function\n");
						check_arith($2, "--lvalue ");
						if($2->type == tableitem_e){
							$$ = emit_iftableitem($2);
							emit(sub, $$, newexpr_constnum(1), $$, -1, yylineno);
							emit(tablesetelem, $2,$2->index, $$, -1, yylineno);
						}else{
							emit(sub, $2, newexpr_constnum(1), $2, -1, yylineno);
							$$ = newexpr(arithexpr_e);
							$$->entry = newtemp();
							emit(assign, $2, NULL, $$, -1, yylineno);
						}
						cout<<"term -> --lvalue in line "<<yylineno<<endl;}
	| lvalue {
				if(CheckIfLibFunction2(yytext, ST, $1) || CheckIfUserFunction2(yytext, ST, $1))
						yyerror("-- operator to Function\n");}
						 MINUS_MINUS {
					check_arith($1, "lvalue-- ");
					$$ = newexpr(var_e);
					$$->entry = newtemp();
					if($1->type == tableitem_e){
						expr* val = emit_iftableitem($1);
						emit(assign, val, NULL, $$, -1, yylineno);
						emit(sub, val, newexpr_constnum(1), val, -1, yylineno);
						emit(tablesetelem, $1,$1->index,val , -1, yylineno);
					}else{
						emit(assign,$1, NULL, $$, -1, yylineno);
						emit(sub, $1, newexpr_constnum(1), $1, -1, yylineno);		//10.33
					}
		cout<<"term -> lvalue-- in line "<<yylineno<<endl;}
	| primary {
		$$=$1;
		cout<<"term -> primary in line "<<yylineno<<endl; $$=$1;}
	;

assignexpr: lvalue{
	if(CheckIfLibFunction2(yytext, ST, $1) || CheckIfUserFunction2(yytext, ST, $1))
							yyerror("Assignment to Function\n");
	} ASSIGN expr {
				cout<<"assignexpr -> lvalue = expr in line "<< yylineno<<endl;
					if ($1->type == tableitem_e){ 
						emit(//lvalue[index] = expr
							tablesetelem,
							$1,
							$1->index,
							$4,-1, yylineno
						);
						$$ = emit_iftableitem( $1 );
						$$->type = assignexpr_e;
					}else{
						emit(// that is: lvalue = expr
							assign,
							$4,
							NULL,
							$1, -1, yylineno
						);
						$$ = newexpr(assignexpr_e);
						$$->entry = newtemp();

						emit(assign, $1, NULL, $$, -1, yylineno);
					}
				}
	;

primary: lvalue {
	$$ = emit_iftableitem($1);
	cout<<"primary -> lvalue in line "<<yylineno<<endl;$$=$1;}
	| call {cout<<"primary -> call in line "<<yylineno<<endl;}
	| objectdef {
		$$=$1;
		cout<<"primary -> objectdef in line "<<yylineno<<endl;}
	| OPEN_PARENTHESIS funcdef CLOSE_PARENTHESIS  {
		$$=newexpr(programfunc_e);
		$$->entry = $2;
		cout<<"primary -> (funcdef) in line "<<yylineno<<endl;}
	| const  {
		$$=$1;
		cout<<"primary -> const in line "<<yylineno<<endl;}
	;

lvalue: 
	ID {
		std::cout << "lvalue -> ID\tLine: "<< yylineno << std::endl; /*mporw na typwsw to yytext, alla oxi $$ kai $1 etc akoma.*/
		SymbolTableEntry* tmp = NULL;
		for(auto i = (int)scope; i>=0; --i) //begin from the highest scope, decreasing 'till you reach the global scope.
		{ 
		 	tmp = LookupInScope(i, yytext, ST);
				if(tmp!=NULL){ //found something, I refer to that, from now on.
					break;
				}//check whether I can access this symbol or not  
		}
		if(tmp==NULL) { //nothing found. Insert the new symbol
					if(scope==0) Insert(scope, yytext, GLOBAL, yylineno, &ST);
					else Insert(scope, yytext, LOKAL, yylineno, &ST); 
					$$ = lvalue_expr(ST.back()); //grab last element
					inccurrscopeoffset();
		}else{
			$$ = lvalue_expr(tmp); 
		}
	}
	| LOCAL ID {
				SymbolTableEntry* tmp = LookupInScope(scope, yytext, ST);
				if(tmp==NULL){
					if (CheckIfLibFunction(yytext, ST))
						yyerror("Library Function shadowing");
					else{
						switch(scope){
							case(0):
								Insert(scope, yytext, GLOBAL, yylineno, &ST);
								break;
							default:
								Insert(scope,yytext, LOKAL, yylineno, &ST);
								break;
						}
						inccurrscopeoffset();
						$$ = lvalue_expr(ST.back());
					}
				}else{
					$$ = lvalue_expr(tmp);
				}
				std::cout<<"lvalue -> local id in line "<<yylineno<<std::endl; 

	}
	| DOUBLE_COLON ID {
				SymbolTableEntry* tmp = LookupInScope(0, yytext, ST); 
				if(tmp==NULL) { //nothing found, so that's an error
					yyerror("There's no such global element");
				}else
					$$ = lvalue_expr(tmp);
		std::cout<<"lvalue -> ::id in line "<<yylineno<<std::endl;
	}
	| member {
		$$=$1;

		cout<<"lvalue -> member in line "<<yylineno<<endl;}
	| tableitem {
		$$=$1;
		std::cout <<"lvalue -> tableitem in line " << yylineno <<std::endl;
	}
	;

member: lvalue OPEN_SQUARE_BRACKET expr CLOSE_SQUARE_BRACKET {
		$1 = emit_iftableitem($1);
		$$ = newexpr(tableitem_e);
		$$->entry = $1->entry;
		$$->index = $3;
		cout<<"member -> lvalue[expr] in line "<<yylineno<<endl;} 
	| call DOT ID {cout<<"member -> call.id in line "<<yylineno<<endl;}
	| call OPEN_SQUARE_BRACKET expr CLOSE_SQUARE_BRACKET {
		cout<<"member -> call[expr] in line "<<yylineno<<endl;}
	;

tableitem: lvalue DOT ID {
	$$=member_item($1,$3,-1, yylineno);
	cout<<"member -> lvalue.id in line "<<yylineno<<endl;} 
	/*| lvalue OPEN_SQUARE_BRACKET expr CLOSE_SQUARE_BRACKET{
		$1 = emit_iftableitem($1);
		$$ = newexpr(tableitem_e);
		$$->entry = $1->entry;
		$$->index = $3;
	}*/;



call: call OPEN_PARENTHESIS elist CLOSE_PARENTHESIS {
	$$ = make_call($1, $3);
	cout<<"call -> call ( elist ) in line "<<yylineno<<endl;}
	| lvalue callsuffix {
		$1 = emit_iftableitem($1);
	
		if($2->method){
			expr* t = $1;
			//std::cout << $1->entry->name << std::endl;

			$1 = emit_iftableitem(member_item(t,$2->name, -1, yylineno));
			$2->elist->elstack->insert($2->elist->elstack->begin(),t);
			//$2->elist->next = t;
		}
			//std::cout << $1->entry->name << std::endl;
		$$ = make_call($1, $2->elist);		
		
		cout<<"call -> lvalue callsuffix in line "<<yylineno<<endl;}
	| OPEN_PARENTHESIS funcdef CLOSE_PARENTHESIS OPEN_PARENTHESIS elist CLOSE_PARENTHESIS {
		expr* func = newexpr(programfunc_e);
		func->entry = $2;
		$$ = make_call(func, $5);
		cout<<"call -> (funcdef)(elist) in line "<<yylineno<<endl;}
	;

callsuffix: normcall {
	$$ = $1;
	cout<<"callsuffix -> normcall in line "<<yylineno<<endl;}
	| methodcall {
		$$ = $1;
		cout<<"callsuffix -> methodcall in line "<<yylineno<<endl;}
	;

normcall: OPEN_PARENTHESIS elist CLOSE_PARENTHESIS {
	$$=(struct call*)malloc(1*sizeof(struct call));
	$$->elist = $2;
	$$->method = 0;
	$$->name = NULL;
	cout<<"normcall -> (elist) in line "<<yylineno<<endl;
	
/*
	for(unsigned i = 0; i<$2->elstack->size(); ++i){
		std::cout << $2->elstack->at(i)->entry->name << std::endl;
	}*/
	
	}
	;

methodcall: DOUBLE_DOT ID OPEN_PARENTHESIS elist CLOSE_PARENTHESIS {
	/*equivalent to lvalue.id(lvalue, elist)*/ 
	$$=(struct call*)malloc(1*sizeof(struct call));
	$$->elist = $4;
	$$->method = 1;
	$$->name = $2;		//10,27
	cout<<"methodcall -> ..id( elist ) in line "<<yylineno<<endl;
	}			
	;

elist: expr exprs{
	$$ = (expr *)malloc(sizeof(expr));
	$$->elstack = new std::vector<expr*>();
	$$->elstack->insert($$->elstack->begin(),$1);
	$$->elstack->insert($$->elstack->end(), $2->elstack->begin(), $2->elstack->end());
	
	/*while(!($2->elstack->empty())){
		$$->elstack->push_back($2->elstack->top());
		$2->elstack->pop();
	}*/
	cout<<"elist -> expr in line "<<yylineno<<endl;}
	|  {
		cout<<"elist -> exprs in line "<<yylineno<<endl;
		$$ = (expr *)malloc(sizeof(expr));
		$$->elstack = new std::vector<expr*>();
		}
	;

exprs: COMMA expr exprs {
	$$ = (expr *)malloc(sizeof(expr));
	$$->elstack = new std::vector<expr*>();
	$$->elstack->insert($$->elstack->begin(),$2);
	$$->elstack->insert($$->elstack->end(), $3->elstack->begin(), $3->elstack->end());

/*
	while($3&&!($3->elstack->empty())){
		$$->elstack->push_back($3->elstack->top());
		$3->elstack->pop();
	}*/
	cout<<"exprs -> expr , expr in line "<<yylineno<<endl;}
      | {
		 $$ = (expr *)malloc(sizeof(expr));
			$$->elstack = new std::vector<expr*>();
		  cout<<"exprs -> empty in line "<<yylineno<<endl;}
      ;

objectdef: OPEN_SQUARE_BRACKET elist CLOSE_SQUARE_BRACKET {
	expr* t = newexpr(newtable_e);
	t->entry = newtemp();
	emit(tablecreate, t, NULL, NULL, -1, yylineno);
	//traverse to $2->elstack
	int j=0;
	for(std::vector<expr*>::iterator i = $2->elstack->begin(); i!=$2->elstack->end(); ++i){
		emit(tablesetelem, t, newexpr_constnum(j++), *i, -1, yylineno);
		//$2->elstack->at(i)->
	}
	$$ = t;
	std::cout<<"objectdef -> [ elist ] in line "<<yylineno<<std::endl;}
	|  OPEN_SQUARE_BRACKET indexed CLOSE_SQUARE_BRACKET {
		expr* t = newexpr(newtable_e);
		t->entry = newtemp();
		emit(tablecreate, t, NULL, NULL, -1, yylineno);
		int j=0;
		for(std::vector<std::pair<expr*,expr*>*>::iterator i = $2->indexstack->begin(); i!=$2->indexstack->end(); ++i){
			emit(tablesetelem, (*i)->first, (*i)->second, t, -1, yylineno);
			//$2->elstack->at(i)->
		}
		$$ = t;
		std::cout<<"objectdef -> [ indexed ] in line "<<yylineno<<std::endl;}
	;

indexedelems:  COMMA indexedelem indexedelems {
	$$ = (expr* )malloc(sizeof(expr));
	$$->indexstack = new std::vector<std::pair<expr*, expr*>*>();
	$$->indexstack->insert($$->indexstack->begin(),$2);
	$$->indexstack->insert($$->indexstack->end(), $3->indexstack->begin(), $3->indexstack->end());
	cout<<"indexedelems -> indexedelem indexedelems in line "<<yylineno<<endl;}   
	| {cout<<"indexedelems -> indexedelem(*empty) in line "<<yylineno<<endl;
		$$ = (expr* )malloc(sizeof(expr*));
		$$->indexstack = new std::vector<std::pair<expr*, expr*>*>();
	 }
			;	    

indexed: indexedelem indexedelems{
	$$ = (expr* )malloc(sizeof(expr));
	$$->indexstack = new std::vector<std::pair<expr*, expr*>*>();
	$$->indexstack->insert($$->indexstack->begin(),$1);
	$$->indexstack->insert($$->indexstack->end(), $2->indexstack->begin(), $2->indexstack->end());
	cout<<"indexed -> indexedelem in line "<<yylineno<<endl;} 	
	;

indexedelem: OPEN_BRACKET expr COLON expr CLOSE_BRACKET {
	$$ = new std::pair<expr*, expr*>();
	$$->first = $2;
	$$->second = $4;
	cout<<"indexedelem -> { expr : expr } in line "<<yylineno<<endl;}
	   ;

block: OPEN_BRACKET {scope++;} stmts CLOSE_BRACKET {
	Hide(scope, ST);
	scope--;
	$block=$stmts;
	std::cout<<"block -> { stmts } in line "<<yylineno<<std::endl;}	
	;




funcname: ID{
	$$ = $1;
	}
	| {$$ = newtempfuncname(&anonymous_function_counter);};

funcprefix: FUNCTION funcname {

		SymbolTableEntry* tmp = LookupInScope(scope, $2, ST);
		if(tmp != NULL && !CheckIfLibFunction($2, ST)){
			yyerror("Multiple Declaration of Function ID \n");
		}else if(tmp !=NULL && CheckIfLibFunction($2, ST)){
			yyerror("Collision with Library Function \n");
		}else{
			Insert(scope,$2, USERFUNC, yylineno, &ST); 
			tmp=ST.back();
		}


	$$ = tmp; //newsymbol($2, function_s);
	$$->iaddress = nextquadlabel();
	expr* t = newexpr(programfunc_e);
	t->entry = $$;
	emit(funcstart, t, NULL, NULL, -1, yylineno);
	scopeoffsetStack.push(currscopeoffset()); //push(scopeoffsetStack,currscopeoffset());
	enterscopespace();
	resetformalargoffset();
	};

funcargs: OPEN_PARENTHESIS {scope++;} idlist{scope--;} CLOSE_PARENTHESIS{
	enterscopespace();
	resetfunctionlocaloffset();
	};

funcbody: block {
	$$ = currscopeoffset();
	exitscopespace();
	};

funcdef: funcprefix funcargs funcbody {
    exitscopespace();
    $1->totalcalls = $3;
    int offset = scopeoffsetStack.top();
	//std::cout <<"offset = " << offset <<std::endl;
	scopeoffsetStack.pop();//pop_and_top(scopeoffsetStack);
    restorecurrscopeoffset(offset);
    $$ = $1;
	expr* t = newexpr(programfunc_e);
	t->entry = $$;
    emit(funcend, t, NULL, NULL, -1, yylineno);			//10,7
	};


const: DECIMAL {
	$$= newexpr_constnum($1);
	cout<<"const -> decimal in line "<<yylineno<<endl;}
	| FLOAT {
		$$= newexpr_constnum($1);
	cout<<"const -> float in line "<<yylineno<<endl;}
	| STRING {
		$$ = newexpr_conststring($1);
		cout<<"const -> string in line "<<yylineno<<endl; printf("%s\n", $1);}
	| NIL {cout<<"const -> nil in line "<<yylineno<<endl;
	$$ = newexpr_constnil();
	}
	| TRUE {
		$$= newexpr_constbool(true);
		cout<<"const -> true in "<<yylineno<<endl;}
	| FALSE {
		$$= newexpr_constbool(false);
		cout<<"const -> false in line "<<yylineno<<endl;}
	;


idlist:  ID {
			SymbolTableEntry* tmp = LookupInScope(scope, $1, ST);
			if(tmp != NULL){
				yyerror("Formal Redeclaration.\n");
			}else if(CheckIfLibFunction($1, ST)){
				yyerror("Formal shadows Library Function.\n");
			}else{
				Insert(scope, $1, FORMAL, yylineno, &ST);
				inccurrscopeoffset();
			}
			cout<<"idlist -> id in line "<<yylineno<<endl;}
	| ID {			
		SymbolTableEntry* tmp = LookupInScope(scope, $1, ST);
			if(tmp != NULL){
				yyerror("Formal Redeclaration.\n");
			}else if(CheckIfLibFunction($1, ST)){
				yyerror("Formal shadows Library Function.\n");
			}else{
				Insert(scope, $1, FORMAL, yylineno, &ST);
				inccurrscopeoffset();
			}
		} COMMA idlist {cout<<"idlist -> ids in "<<yylineno<<endl;}
	| {cout<<"idlist -> empty in line "<<yylineno<<endl;}
	;

ifprefix: IF OPEN_PARENTHESIS expr CLOSE_PARENTHESIS{
            emit(if_eq, $3, newexpr_constbool(true), NULL,  nextquad()+2, yylineno);
            $$ = nextquad();
            emit(jump, NULL, NULL, 0, -1, yylineno);
        };
elseprefix: ELSE{
    $$= nextquad();
    emit(jump, NULL, NULL, 0, -1, yylineno);
	};

ifstmt: ifprefix stmt{
            patchlabel($1, nextquad());
        }
        | ifprefix stmt elseprefix stmt {
            patchlabel($1, $elseprefix + 1);
            patchlabel($elseprefix, nextquad());
        };


loopstart: {++loopcounter;};

loopend: {--loopcounter;};

loopstmt: loopstart stmt loopend {$$ = $2;};


whilestart: WHILE {
    $$ = nextquad();
	};

whilecond: OPEN_PARENTHESIS expr CLOSE_PARENTHESIS{
    emit(if_eq, $2, newexpr_constbool(true),0, nextquad() +2, yylineno);
    $$ = nextquad();
    emit(jump, NULL, NULL, 0, -1, yylineno);
	};

whilestmt: whilestart whilecond loopstmt {
    emit(jump, NULL, NULL, 0, $1,  yylineno);
    patchlabel($2, nextquad());
	if($loopstmt != NULL){
	    patchlist($loopstmt->breakList, nextquad());
		patchlist($loopstmt->contList, $1);
	}
	};

N: {$$ = nextquad(); emit(jump,NULL,NULL,0,-1,yylineno);};

M: {$$ = nextquad();}

forprefix: FOR OPEN_PARENTHESIS elist SEMICOLON M expr SEMICOLON {
	$$ = (expr*)malloc(sizeof(expr));
	$$->test = $5;
	$$->enter = nextquad();
	emit(if_eq, $6, newexpr_constbool(true), 0, -1, yylineno);
};


forstmt: forprefix N elist CLOSE_PARENTHESIS N loopstmt N {
	patchlabel($1->enter, $5+1);
	patchlabel($2, nextquad());
	patchlabel($5, $1->test);
	patchlabel($7, $2+1);

	if($6 != NULL){
		patchlist($6->breakList, nextquad());
		patchlist($6->contList, $2+1);
	}

	cout<<"forstmt -> for ( elist; expr; elist) stmt in line "<<yylineno<<endl;}
	;

returnstmt: RETURN SEMICOLON {
			if(currscopespace()==functionlocal)
        		emit(ret, NULL, NULL, NULL, -1, yylineno);
			else
				yyerror("Return outside of Function body\n");
        cout<<"returnstmt -> return; in line "<<yylineno<<endl;}       
        | RETURN expr SEMICOLON {
			if(currscopespace()==functionlocal)
            	emit(ret, $2, NULL, NULL, -1, yylineno);
			else
				yyerror("Return outside of Function body\n");	
        cout<<"returnstmt -> return expr; in line "<<yylineno<<endl;}
		;

%%


int main(int argc, char **argv)
{
	
        if(argc > 1){
                if(!(yyin = fopen(argv[1], "r"))){
                        fprintf(stderr,"Cannot open %s\n", argv[1]);
                        return 1;
                }
        }
        else
                yyin = stdin;
	Initialize(&ST);
	yyparse(); 
	std::cout<<"\n---------- Symbol Table ----------"<<std::endl;
	PrintAll(ST);
	std::cout<<"\n---------- Intermediate Code ----------"<<std::endl;
	QuadPrint();
	std::cout<<"\n---------- Target Code ----------"<<std::endl;
	generate2();//metatrepei ton endiameso kwdika se teliko
	
	InstructionPrint();

	AVMbinaryfile();
	return 0;
}
