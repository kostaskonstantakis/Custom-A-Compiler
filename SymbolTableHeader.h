#ifndef SYMBOLTABLEHEADER_H
#define SYMBOLTABLEHEADER_H
#include <iostream>
#include <string.h>
#include <vector>
#include <assert.h>
#include <map>
#include <stack>
#include <utility>
#define EXPAND_SIZE 1024
#define CURR_SIZE (total*sizeof(quad))
#define NEW_SIZE (EXPAND_SIZE*sizeof(quad)+CURR_SIZE)



using namespace std;

/*
#define execute_add execute_arithmetic
#define execute_sub execute_arithmetic
#define execute_mul execute_arithmetic
#define execute_div execute_arithmetic
#define execute_mod execute_arithmetic
*/


extern unsigned int scope;
extern int yylineno;
extern std::vector<struct SymbolTableEntry*> ST;
typedef enum scopespace_t{
    programvar,
    functionlocal,
    formalarg
}scopespace_t;

typedef enum symbol_t{
    var_s,
    programfunc_s,
    libraryfunc_s
}symbol_t;

typedef void (*library_func_t)(void);
library_func_t avm_getlibraryfunc(char* id); //typical hashing

std::stack<unsigned>scopeoffsetStack;
unsigned programVarOffset=0;
unsigned functionLocalOffset=0;
unsigned formalArgOffset=0;
unsigned scopeSpaceCounter=1;

scopespace_t currscopespace(void) {
	if(scopeSpaceCounter==1)
		return programvar;
	else if(scopeSpaceCounter%2==0)
		return formalarg;
	else return functionlocal;
}
	
unsigned currscopeoffset() {
	switch(currscopespace()){
		case programvar: return programVarOffset;
		case functionlocal: return functionLocalOffset;
		case formalarg: return formalArgOffset;
		default: assert(0);
	}
}
	
enum SymbolType{
GLOBAL, LOKAL, FORMAL,
USERFUNC, LIBFUNC
};

struct SymbolTableEntry{
	bool isActive;
	/*const*/ char *name;
	unsigned int scope;
	unsigned int line;
	
	enum SymbolType type;
	
	symbol_t symbol_t_type;
	scopespace_t space;
	unsigned offset;

	unsigned iaddress;
	unsigned totalcalls;
	unsigned taddress;
	std::stack<int>ReturnList;
	

	SymbolTableEntry(const char *name,  enum SymbolType type, unsigned int scope, unsigned int line)
	{
		this->isActive=true;
		this->type=type;
		this->scope=scope;
		this->line=line;
		this->name=strdup(name);
		this->space=currscopespace();
		this->offset=currscopeoffset();

		if(type == LIBFUNC) this->symbol_t_type=libraryfunc_s;
		if(type == USERFUNC) this->symbol_t_type=programfunc_s;
	}
	
};

typedef enum iopcode {
	
	assign, add, sub, 
	mul, _div, mod, 
	uminus, _and, _or,
	_not, if_eq, if_noteq,
	if_lesseq, if_greatereq, if_less,
	if_greater, call, param,
	ret, getretval, funcstart,
	funcend, tablecreate,
	tablesetelem, tablegetelem, jump
	
}iopcode; 

typedef enum expr_t {
	
	var_e,
	tableitem_e,
	programfunc_e,
	libraryfunc_e,
	arithexpr_e,
	boolexpr_e,
	assignexpr_e,
	newtable_e,
	constnum_e,
	constbool_e,
	conststring_e,
	nil_e
}expr_t;

struct symbol
{
    symbol_t type;
    char* name;
    scopespace_t space;
    unsigned offset;
    unsigned scope;
    unsigned line;
};

struct expr {
   	expr_t type;
	struct SymbolTableEntry* entry;
    struct expr* index;
    double numConst;
    char* strConst;
    bool boolConst;
	unsigned int test;
	unsigned int enter;
    struct expr* next;
	std::vector<expr*> *elstack;
	std::vector<std::pair<expr*,expr*>*> *indexstack;
	
};

struct quad{
    iopcode op;
    struct expr*  result;
    struct expr*   arg1;
    struct expr*   arg2;
    unsigned label;
    unsigned line;
	
	unsigned taddress;
};

struct call{

	struct expr* elist;
	unsigned char method;
	char* name;

};

struct stmt_t{
	int breakList, contList;
};

void make_stmt(stmt_t* s){
	s->breakList = 0;
	s->contList = 0;
}


struct quad* quads = (struct quad*) 0;
unsigned total = 0;
unsigned int currQuad = 0;


int newList(int i){
	quads[i].label = 0; return i;
}

int mergeList(int l1, int l2){
	if(!l1)
		return l2;
	else if (!l2)
		return l1;
	else{
		int i = l1;
		while(quads[i].label)
			i = quads[i].label;
		quads[i].label =l2;
		return l1;
	}
}

void patchlist(int list, int label){
	while(list){
		int next = quads[list].label;
		quads[list].label = label;
		list = next;
	}
}

void expand(void){
    assert(total==currQuad);
    quad* p = (quad *) malloc(NEW_SIZE);
    if(quads){
        memcpy(p,quads, CURR_SIZE);
        free(quads);
    }
    quads = p;
    total += EXPAND_SIZE;
}






void comperror(const char* format, const char* context)
		{
       			std::cerr<<format<<context<<std::endl;
		}



const char* OpcodeToString(iopcode opc){
	const std::map<iopcode, const char*>OTS{
		{assign, "ASSIGN\t"},
		{add, "ADD\t"},
		{sub, "SUB\t"},
		{mul, "MULTIPLY\t"},
		{_div, "DIV\t"},
		{mod, "MOD\t"},
		{uminus, "UMINUS\t"},
		{_and, "AND\t"},
		{_or, "OR\t"},
		{_not, "NOT\t"},
		{if_eq, "IF_EQ\t"},
		{if_noteq, "IF_NOTEQ\t"},
		{if_greatereq, "IF_GREATEREQ\t"},
		{if_greater,"IF_GREATER\t"},
		{if_less, "IF_LESS\t"},
		{if_lesseq, "IF_LESSEQ\t"},
		{call, "CALL\t"},
		{getretval, "GETRETVAL\t"},
		{funcstart, "FUNCSTART\t"},
		{funcend, "FUNCEND\t"},
		{tablecreate, "TABLECREATE\t"},
		{tablegetelem, "TABLEGETELEM\t"},
		{tablesetelem, "TABLESETELEM\t"},
		{ret, "RET\t"},
		{param, "PARAM\t"},
		{jump, "JUMP\t"}
		};
	auto it = OTS.find(opc);
	return it == OTS.end() ? "*******\t"  : it->second;					//not checking range
}

const char* ExprToString(expr* e){
	/*const std::map<expr_t, const char*>ETS{
			{var_e, "VAR_E\t"},
			{tableitem_e, "TABLEITEM_E\t"},
			{programfunc_e, "PROGRAMFUNC_E\t"},
			{libraryfunc_e, "LIBRARYFUNC_E\t"},
			{arithexpr_e, "ARITHEXPR_E\t"},
			{boolexpr_e,	"BOOLEXPR_E\t"},
			{assignexpr_e, "ASSIGNEXPR_E\t"},
			{newtable_e,	"NEWTABLE_E\t"},
			{constnum_e,	"CONSTNUM_E\t"},
			{constbool_e, "CONSTBOOL_E\t"},
			{conststring_e, "CONSTSTRING_E\t"},
			{nil_e, "NIL_E\t"}
	};
	auto it = ETS.find(e->type);*/
	if( e == NULL) return "******* ";

	switch(e->type){
		case constnum_e:
			std::cout << e->numConst;
			return "\t";
			break;
		case constbool_e:
			std::cout << e->boolConst;
			return "\t";
			break;
		case conststring_e:
			std::cout <<e->strConst;
			return "\t";
			break;
		case nil_e:
			std::cout  << "NIL";
			return "\t";
			break;
		
	}

	if( e->entry!=NULL)
		std::cout << e->entry->name;
	//if( e->index!=NULL)
	//	std::cout << e->index->sym->name;
	return "\t";

	//return e == NULL ? "*******" : e->entry->name;

	//return it == ETS.end() ? "********\t" : it->second;
}

void QuadPrint(){
	for (int i = 0; i < currQuad; ++i){ //mhpws prepei na paei to total?
		std::cout << i+1<< ":\t";
		std::cout << OpcodeToString((quads+i)->op) ;
		std::cout<< ExprToString((quads+i)->result);
		std::cout << ExprToString((quads+i)->arg1);
		std::cout << ExprToString((quads+i)->arg2); 
		std::cout << ((int)(quads+i)->label==-1 ? "" : to_string((int)(quads+i)->label) )<< std::endl;
		//std::cout<<i+1<<"\t"<<(quads+i)->op<<"\t"<<(quads+i)->result<<"\t"<<(quads+i)->arg1<<"\t"<<(quads+i)->arg2<<"\t"<<(int)(quads+i)->label<<std::endl;
	}
}

void inccurrscopeoffset() {
	switch(currscopespace()) {
		case programvar: ++programVarOffset; break;
		case functionlocal: ++functionLocalOffset;  break;
		case formalarg: ++formalArgOffset; break;
		default: assert(0); 
	}
		
}

void emit(enum iopcode op,
		struct expr* arg1,
		struct expr* arg2,
		struct expr* result,
		unsigned label,
		unsigned line){
				if(currQuad==total)
					expand();
				quad* p=quads+currQuad++;
				p->op=op;
				p->arg1=arg1;
				p->arg2=arg2;
				p->result=result;
				p->label=label;
				p->line=line; 

}

void enterscopespace(){
    ++scopeSpaceCounter;
}

void exitscopespace(){
    assert(scopeSpaceCounter >1);
    --scopeSpaceCounter;
}

int tempcounter = 0;
void Insert(unsigned int scope, const char* n, enum SymbolType T, int line, std::vector<SymbolTableEntry*> *ST);
SymbolTableEntry* LookupInScope(unsigned int ScopeNum, const char* n, std::vector<SymbolTableEntry*> ST);
char* newtempname() { 
	std::string s="_t"; 
	s+= to_string(tempcounter); 
	++tempcounter;
	return strdup(s.c_str());
}

unsigned int nextquad(){
	return currQuad;
}
 
void resettemp() { tempcounter = 0; }

SymbolTableEntry* newtemp() {
	char * name = newtempname();
	SymbolTableEntry* sym = LookupInScope(scope,name,ST);
	if (sym == NULL) {
		Insert(scope, name, LOKAL, yylineno, &ST); 
		inccurrscopeoffset();
		return ST.back(); //grab last element
	}
	else
		return sym;
}

expr* lvalue_expr(SymbolTableEntry* sym) {
	assert(sym);
	expr* e=(expr*)malloc(sizeof(expr));
	memset(e, 0, sizeof(expr));
	e->next=(expr*) 0;
	e->entry=sym;
	switch(sym->symbol_t_type) {
		case var_s: e->type=var_e; break;
		case programfunc_s: e->type=programfunc_e; break;
		case libraryfunc_s: e->type=libraryfunc_e; break;
	}
	return e;
}

expr* newexpr(expr_t t){
    expr* e = (expr*)malloc(sizeof(expr));
    memset(e, 0, sizeof(expr));
    e->type = t;
    return e;
}

expr* emit_iftableitem(expr* e){
	if(e->type != tableitem_e)
		return e;
	else{
		expr* result = newexpr(var_e);
		result->entry = newtemp();
		emit(tablegetelem,e,e->index,result, -1, yylineno);
		return result;
	}	
}
expr* newexpr_constnil() {
	expr* e=newexpr(nil_e);
	//e->numConst=i;
	return e;	
}

expr* newexpr_constnum(double i) {
	expr* e=newexpr(constnum_e);
	e->numConst=i;
	return e;	
}

void check_arith(expr* e,const char* context) {
	if(e->type==constbool_e || 
	e->type==conststring_e ||
	e->type==nil_e ||
	e->type==newtable_e ||
	e->type==programfunc_e ||
	e->type==libraryfunc_e ||
	e->type==boolexpr_e) 
		comperror("Illegal expr used in %s!", context);
	
}

unsigned int istempname(char* s) {return *s=='_';}
//unsigned int istempexpr(expr* e) {return (e->entry && istempname(e->entry->name));}


void resetformalargoffset(void) {formalArgOffset=0; }
void resetfunctionlocaloffset() {functionLocalOffset=0;}
void restorecurrscopeoffset(unsigned n) {
	switch (currscopespace())
    {
		case programvar:
			programVarOffset = n;
			break;
		case functionlocal:
			functionLocalOffset = n;
			break;
		case formalarg:
			formalArgOffset = n;
			break;
		default:
			assert(0);
    }

}

expr* newexpr_constbool(bool b){
	expr* e = newexpr(constbool_e);
	e->boolConst = !!b;
	return e;
}

unsigned nextquadlabel(){
    return currQuad;
}

void patchlabel(unsigned quadNo, unsigned label){
    assert(quadNo < currQuad);
    quads[quadNo].label = label;
}


expr* newexpr_conststring(char* s){
    expr* e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}	

expr* member_item(expr* lv, char* name, int quadlabel , int line){
    lv = emit_iftableitem(lv);
    expr* ti = newexpr(tableitem_e);
    ti->entry = lv->entry;
    ti->index = newexpr_conststring(name);
    return ti;
}


expr* make_call(expr* lv, expr* reversed_elist){
	expr* func = emit_iftableitem(lv);
	for(std::vector<expr*>::reverse_iterator i = reversed_elist->elstack->rbegin(); i!=reversed_elist->elstack->rend(); ++i){
		emit(param, *i, NULL,NULL, -1, yylineno);
	}


	emit(call, func, NULL, NULL, -1, yylineno);
	expr* result = newexpr(var_e);
	result->entry = newtemp();
	emit(getretval, NULL, NULL, result, -1, yylineno);
	return result;
}

char* newtempfuncname(int * anonymous_function_counter){
	std::string s="_foo"; 
		s+= to_string(*anonymous_function_counter); 
		++*anonymous_function_counter;
		char *c=strdup(s.c_str());
		//Insert(scope, c, USERFUNC, yylineno, &ST);
		return c;
}

SymbolTableEntry* LookupInScope(unsigned int ScopeNum, const char* n, std::vector<SymbolTableEntry*> ST){
	for (auto i = 0; i < ST.size(); ++i){
		if( !strcmp(n,ST.at(i)->name ) && ST.at(i)->scope == ScopeNum && ST.at(i)->isActive){
			return ST.at(i);
		}
	}
	return NULL;
}
	
void PrintScope(unsigned int ScopeNum, std::vector<SymbolTableEntry*> ST){
	for (auto i = 0; i < ST.size(); ++i){
		//if( ST.at(i)->scope == ScopeNum){
			std::cout << ST.at(i)->name << "\t" << ST.at(i)->type << "\t (line " << ST.at(i)->line << ")\t (scope " << (int)ST.at(i)->scope << ")" << std::endl;
		//}
	}
}
	
void PrintAll(std::vector<SymbolTableEntry*> ST){
	//for (unsigned i = 0 ; i < 10; ++i){
		//std::cout << i << std::endl;
		PrintScope(0, ST); //i
	//}
}

bool CheckIfLibFunction( const char* n, std::vector<SymbolTableEntry*> ST){
	for (int i = 0; i < ST.size(); ++i){
		if( !strcmp(n,ST.at(i)->name ) && ST.at(i)->scope == 0 && ST.at(i)->type == LIBFUNC){
			return true;
		}
	}
	
	return false;
		
}

bool CheckIfUserFunction( const char* n, std::vector<SymbolTableEntry*> ST){
	for (int i = 0; i < ST.size(); ++i){
		if( !strcmp(n,ST.at(i)->name ) && ST.at(i)->isActive == true && ST.at(i)->type == USERFUNC){
			return true;
		}
	}

	
	return false;
		
}
bool CheckIfLibFunction2( const char* n, std::vector<SymbolTableEntry*> ST, struct expr * e){
	/*or (int i = 0; i < ST.size(); ++i){
		if( !strcmp(n,ST.at(i)->name ) && ST.at(i)->scope == 0 && ST.at(i)->type == LIBFUNC){
			return true;
		}
	}*/
	if(e->type == libraryfunc_e)
		return true;
	return false;
		
}

bool CheckIfUserFunction2( const char* n, std::vector<SymbolTableEntry*> ST, struct expr *e){
	/*for (int i = 0; i < ST.size(); ++i){
		if( !strcmp(n,ST.at(i)->name ) && ST.at(i)->isActive == 1 && ST.at(i)->type == USERFUNC){
			return true;
		}
	}*/

	if(e->type == programfunc_e)
		return true;
	return false;
		
}
void Insert(unsigned int scope, const char* n, enum SymbolType T, int line, std::vector<SymbolTableEntry*> *ST){
	SymbolTableEntry* tmp = new SymbolTableEntry(n, T, scope, line);
	ST->push_back(tmp);
}
	
void Hide(unsigned int scope, std::vector<SymbolTableEntry*> ST){
	for (auto i = 0; i < ST.size(); ++i){
		if(ST.at(i)->scope == scope){
			ST.at(i)->isActive = false;
		}
	}
	
}


void Initialize(std::vector<SymbolTableEntry*> *st){
	Insert(0, "print",  LIBFUNC ,0 , st);
	Insert(0, "input", LIBFUNC,0 , st);
	Insert(0,"objectmemberkeys",  LIBFUNC,0 , st);
	Insert(0, "objectcopy", LIBFUNC,0 , st);
	Insert(0, "totalarguments", LIBFUNC,0 , st);
	Insert(0, "argument", LIBFUNC,0 , st);
	Insert(0, "typeof", LIBFUNC,0 , st);
	Insert(0, "strtonum",LIBFUNC,0 , st);
	Insert(0, "sqrt", LIBFUNC,0 , st);
	Insert(0, "cos", LIBFUNC,0 , st);
	Insert(0, "sin", LIBFUNC,0 , st);
}




#endif
