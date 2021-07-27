#include "SymbolTableHeader.h"
#include <fstream>

#define EXPAND_SIZE_JUMP 1024
#define CURR_SIZE_JUMP (ij_total * sizeof(struct incomplete_jump))
#define NEW_SIZE_JUMP (EXPAND_SIZE_JUMP * sizeof(struct incomplete_jump) + CURR_SIZE_JUMP)

bool executionFinished = false;
unsigned pc = 0;
unsigned currLine = 0;
unsigned codeSize = 0;
unsigned totalNumConsts;
unsigned totalStringConsts;
unsigned totalNamedLibfuncs;
unsigned totalUserFuncs;
std::vector<double> numConsts;
std::vector<char *> stringConsts;
std::vector<char *> namedLibfuncs;
std::vector<struct userfunc *> userFuncs;
std::stack<struct SymbolTableEntry *> funcstack;
std::vector<struct instruction> instrVec;

typedef double (*arithmetic_func_t)(double d, double y);
double add_impl(double x, double y) { return x + y; }
double sub_impl(double x, double y) { return x - y; }
double mul_impl(double x, double y) { return x * y; }
double div_impl(double x, double y) {
	if (y != 0)
        return x / y;
    else
        return EXIT_FAILURE; }
double mod_impl(double x, double y) {
	if (y != 0)
        return ((unsigned)x) % ((unsigned)y);
    else
        return EXIT_FAILURE;  }
int currProccessedQuad = 0;

arithmetic_func_t arithmeticFuncs[] = {
    add_impl,
    sub_impl,
    mul_impl,
    div_impl,
    mod_impl};

enum vmopcode
{
    assign_v,
    add_v,
    sub_v,
    mul_v,
    div_v,
    mod_v,
    uminus_v,
    and_v,
    cr_v,
    not_v,
    jeq_v,
    jne_v,
    jle_v,
    jge_v,
    jlt_v,
    jgt_v,
    call_v,
    pusharg_v,
    funcenter_v,
    funcexit_v,
    newtable_v,
    tablegetelem_v,
    tablesetelem_v,
    nop_v,
    jump_v
};

typedef enum vmarg_t
{
    label_a = 0,
    global_a = 1,
    formal_a = 2,
    local_a = 3,
    number_a = 4,
    string_a = 5,
    bool_a = 6,
    nil_a = 7,
    userfunc_a = 8,
    libfunc_a = 9,
    retval_a = 10
} vmarg_t;

typedef struct vmarg
{
    enum vmarg_t type;
    unsigned val;

    bool isActive;
} vmarg;

struct userfunc
{
    unsigned address;
    unsigned localSize;
    char *id;
};

unsigned consts_newstring(char *s)
{
    for (std::vector<char *>::iterator i = stringConsts.begin(); i != stringConsts.end(); ++i)
    {
        if (strcmp(*i, s) == 0)
        {
            int p = std::distance(stringConsts.begin(), i);
            return p;
        }
    }
    stringConsts.push_back(s);
    return totalStringConsts++;
}

unsigned consts_newnumber(double n)
{
    for (std::vector<double>::iterator i = numConsts.begin(); i != numConsts.end(); ++i)
    {
        if (*i == n)
        {
            int p = std::distance(numConsts.begin(), i);
            return p;
        }
    }
    numConsts.push_back(n);
    return totalNumConsts++;
}

unsigned libfuncs_newused(char *s)
{
    for (std::vector<char *>::iterator i = namedLibfuncs.begin(); i != namedLibfuncs.end(); ++i)
    {
        if (strcmp(*i, s) == 0)
        {
            int p = std::distance(namedLibfuncs.begin(), i);
            return p;
        }
    }
    namedLibfuncs.push_back(s);
    return totalNamedLibfuncs++;
}

unsigned userfuncs_newfunc(SymbolTableEntry *s)
{
    for (std::vector<struct userfunc *>::iterator i = userFuncs.begin(); i != userFuncs.end(); ++i)
    {
        if (strcmp((*i)->id, s->name) == 0)
        {
            int p = std::distance(userFuncs.begin(), i);
            return p;
        }
    }
    userfunc *my_userfunc = (struct userfunc *)malloc(sizeof(struct userfunc));
    my_userfunc->id = s->name;
    my_userfunc->address = s->taddress;
    my_userfunc->localSize = s->totalcalls;
    userFuncs.push_back(my_userfunc);
    return totalUserFuncs++;
}

struct incomplete_jump
{
    unsigned instrNo;      //jump instruction mumber
    unsigned iaddress;     //i-code jump-target address
    incomplete_jump *next; //trivial linked list
} incomplete_jump;

struct instruction
{
    enum vmopcode vmop;
    struct vmarg result;
    struct vmarg arg1;
    struct vmarg arg2;
    unsigned srcLine;
};

struct incomplete_jump *ij_head = (struct incomplete_jump *)0;
unsigned ij_total = 0;
unsigned ij_Curr = 0;
struct instruction *code = (struct instruction *)0;

void expand_jump(void)
{
    assert(ij_Curr == ij_total);
    struct incomplete_jump *p = (struct incomplete_jump *)malloc(NEW_SIZE_JUMP);
    if (ij_head)
    {
        memcpy(p, ij_head, CURR_SIZE_JUMP);
        free(ij_head);
    }
    ij_head = p;
    ij_Curr += EXPAND_SIZE_JUMP;
}

void add_incomplete_jump(unsigned instrNo, unsigned iaddress)
{

    if (ij_total == ij_Curr)
        expand_jump();

    struct incomplete_jump *j = &ij_head[ij_total++];
    j->instrNo = instrNo;
    j->iaddress = iaddress;
}

void patch_incomplete_jumps()
{
    struct incomplete_jump *temp;// = (struct incomplete_jump *)malloc(sizeof(incomplete_jump));
    temp = ij_head;

    for (int  i = 0 ; i < ij_total; ++i){
        if (temp[i].iaddress == currQuad)
        {
            instrVec[temp[i].instrNo].result.val = instrVec.size();
        }
        else
        {
            instrVec[temp[i].instrNo].result.val = quads[temp[i].iaddress].taddress;
        }
    }



}

typedef void (*generator_func_t)(quad *);

void make_operand(expr *e, vmarg *arg)
{
    
    switch (e->type)
    {
    case var_e:
    case tableitem_e:
    case arithexpr_e:
    case boolexpr_e:
    case assignexpr_e:
    case newtable_e:
    {
        assert(e->entry);
        arg->val = e->entry->offset;
        if (e->entry->space == programvar)
        {
            arg->type = global_a;
        }
        else if (e->entry->space == functionlocal)
        {
            arg->type = local_a;
        }
        else if (e->entry->space == formalarg)
        {
            arg->type = formal_a;
        }
        break;
    }
    case constbool_e:
    {
        arg->val = e->boolConst;
        arg->type = bool_a;
        break;
    }
    case conststring_e:
    {
        arg->val = consts_newstring(e->strConst);
        arg->type = string_a;
        break;
    }
    case constnum_e:
    {
        arg->val = consts_newnumber(e->numConst);
        arg->type = number_a;
        break;
    }
    case nil_e:
    {
        arg->type = nil_a;
        break;
    }
    case programfunc_e:
    {
        arg->type = userfunc_a;
        arg->val = e->entry->iaddress; //taddress originally
        arg->val = userfuncs_newfunc(e->entry);
        break;
    }
    case libraryfunc_e:
    {
        arg->type = libfunc_a;
        arg->val = libfuncs_newused(e->entry->name);
        break;
    }
    }
    arg->isActive = true;
}

void reset_operand(vmarg *arg)
{
    arg->isActive = false;
}
//Helper functions to produce common arguments for
//generated instructions, like 1, 0, "true", "false"
//and function return value
void make_numberoperand(vmarg *arg, double val)
{
    arg->val = consts_newnumber(val);
    arg->type = number_a;
    arg->isActive = true;
}

void make_booloperand(vmarg *arg, unsigned val)
{
    arg->val = val;
    arg->type = bool_a;
    arg->isActive = true;
}

void make_retvaloperand(vmarg *arg)
{
    arg->type = retval_a;
    arg->isActive = true;
}

int nextinstructionlabel()
{
    return instrVec.size();
}

void emit_v(instruction t)
{
    instrVec.push_back(t);
}

void generate(vmopcode vop, quad *q)
{
    instruction t;
    t.vmop = vop;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    reset_operand(&t.result);

    if (q->arg1)
        make_operand(q->arg1, &t.arg1);
    if (q->arg2)
        make_operand(q->arg2, &t.arg2);
    if (q->result)
        make_operand(q->result, &t.result);
    q->taddress = nextinstructionlabel();
    emit_v(t);
    currProccessedQuad++;
}

void generate_ADD(quad *q) { generate(add_v, q);    
}
void generate_SUB(quad *q) { generate(sub_v, q); }  
void generate_MUL(quad *q) { generate(mul_v, q); }
void generate_DIV(quad *q) { generate(div_v, q);  
 }
void generate_MOD(quad *q) { generate(mod_v, q);   
 }

void generate_relational(vmopcode vmop, quad *q)
{
    instruction t;
    t.vmop = vmop;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    reset_operand(&t.result);

    if (q->arg1)
        make_operand(q->arg1, &t.arg1);
    if (q->arg2)
        make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currProccessedQuad)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(nextinstructionlabel(), q->label);
    t.result.isActive = true;
    q->taddress = nextinstructionlabel();
    emit_v(t);
        currProccessedQuad++;

}

void generate_NEWTABLE(quad *q) { generate(newtable_v, q);   
 }
void generate_TABLEGETELM(quad *q) { generate(tablegetelem_v, q); 
 }
void generate_TABLESETELEM(quad *q) { generate(tablesetelem_v, q); 
}
void generate_ASSIGN(quad *q) { generate(assign_v, q); 
}
void generate_NOP(quad *q)
{
    instruction t;
    t.vmop = nop_v;
    emit_v(t);
        currProccessedQuad++;

}
void generate_JUMP(quad *q) { generate_relational(jump_v, q); 
 }
void generate_IF_EQ(quad *q) { generate_relational(jeq_v, q); 
 }
void generate_IF_NOTEQ(quad *q) { generate_relational(jne_v, q);   
 }
void generate_IF_GREATER(quad *q) { generate_relational(jgt_v, q);    
}
void generate_IF_GREATEREQ(quad *q) { generate_relational(jge_v, q);    
}
void generate_IF_LESS(quad *q) { generate_relational(jlt_v, q); 
 }
void generate_IF_LESSEQ(quad *q) { generate_relational(jle_v, q); 
 }
void generate_NOT(quad *q)
{
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = jeq_v;
    make_operand(q->arg1, &t.arg1);
    make_booloperand(&t.arg2, false);
    t.result.isActive = true;
    t.result.type = label_a;
    t.result.val = nextinstructionlabel() + 3;
    emit_v(t);
    t.vmop = assign_v;
    make_booloperand(&t.arg1, false);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_v(t);
    //... more
    t.vmop = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    t.result.val = nextinstructionlabel() + 2;
    emit_v(t);
    t.vmop = assign_v;
    make_booloperand(&t.arg1, true);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_v(t);
        currProccessedQuad++;

}

void generate_OR(quad *q)
{
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = jeq_v;
    make_operand(q->arg1, &t.arg1);
    make_booloperand(&t.arg2, true);
    t.result.isActive = true;
    t.result.type = label_a;
    t.result.val = nextinstructionlabel() + 4;
    emit_v(t);
    make_operand(q->arg2, &t.arg1);
    t.result.val = nextinstructionlabel() + 3;
    emit_v(t);
    t.vmop = assign_v;
    make_booloperand(&t.arg1, false);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_v(t);
    t.vmop = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    t.result.val = nextinstructionlabel() + 2;
    emit_v(t);
    t.vmop = assign_v;
    make_booloperand(&t.arg1, true);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_v(t);
        currProccessedQuad++;

}

void generate_PARAM(quad *q)
{
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = pusharg_v;
    make_operand(q->arg1, &t.arg1);
    reset_operand(&t.result);

    reset_operand(&t.arg2);

    emit_v(t);
        currProccessedQuad++;

}

void generate_CALL(quad *q)
{
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = call_v;
    make_operand(q->arg1, &t.arg1);
    reset_operand(&t.result);

    reset_operand(&t.arg2);
    emit_v(t);
        currProccessedQuad++;

}

void generate_GETRETVAL(quad *q)
{
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = assign_v;
    make_operand(q->result, &t.result);
    make_retvaloperand(&t.arg1);

    reset_operand(&t.arg2);
    emit_v(t);
        currProccessedQuad++;

}

void generate_FUNCSTART(quad *q)
{
    SymbolTableEntry *f;
    f = q->arg1->entry;
	
	////// generate jump before funcstart
	instruction t;
	f->ReturnList.push(nextinstructionlabel());
    t.vmop = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
	t.result.isActive=true;
    emit_v(t);
	///////
	
	
    f->taddress = nextinstructionlabel();
    q->taddress = nextinstructionlabel();

    funcstack.push(f);
   
    t.vmop = funcenter_v;

    make_operand(q->arg1, &t.result);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);

    emit_v(t);
        currProccessedQuad++;

}
void backpatch(unsigned x, unsigned y) {}

void generate_FUNCEND(quad *q)
{
    SymbolTableEntry *f = funcstack.top();
    funcstack.pop();
    //iterate returnList
	//int first_loop=1;
	int j; //used for jump before funcstart, as well.
    while (!f->ReturnList.empty())
    {
         j = f->ReturnList.top();
		
        instrVec[j].result.val = nextinstructionlabel();
		
		
        f->ReturnList.pop();
    }
	instrVec[j].result.val++;
	
    //backpatch(5, nextinstructionlabel());
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = funcexit_v;
    make_operand(q->arg1, &t.result);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    emit_v(t);
        currProccessedQuad++;

}

void generate_TABLEGETELEM(quad *q)
{
    generate(tablegetelem_v, q);

}

void generate_UMINUS(quad *q)
{
    generate(mul_v, q);

}

void generate_AND(quad *q)
{
    q->taddress = nextinstructionlabel();
    instruction t;
    t.vmop = jeq_v;
    make_operand(q->arg1, &t.arg1);
    make_booloperand(&t.arg2, false);
    t.result.isActive = true;
    t.result.type = label_a;
    t.result.val = nextinstructionlabel() + 4;
    emit_v(t);
    make_operand(q->arg2, &t.arg1);
    t.result.val = nextinstructionlabel() + 3;
    emit_v(t);
    t.vmop = assign_v;
    make_booloperand(&t.arg1, true);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_v(t);
    t.vmop = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    t.result.val = nextinstructionlabel() + 2;
    emit_v(t);
    t.vmop = assign_v;
    make_booloperand(&t.arg1, false);
    reset_operand(&t.arg2);
    make_operand(q->result, &t.result);
    emit_v(t);
        currProccessedQuad++;

}
void generate_RETURN(quad *q)
{
    instruction t;
    q->taddress = nextinstructionlabel();
    t.vmop = assign_v; //??????????????? NO IDEA
    reset_operand(&t.arg2);

    make_retvaloperand(&t.result);
    make_operand(q->arg1, &t.arg1);
    emit_v(t);
    SymbolTableEntry *f = funcstack.top();
    f->ReturnList.push(nextinstructionlabel());
    t.vmop = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    emit_v(t);
        currProccessedQuad++;

}

void generate_TABLECREATE(quad *q)
{
    generate(newtable_v, q);
}

void execute_assign(instruction *i);
void execute_add(instruction *i);
void execute_sub(instruction *i);
void execute_mul(instruction *i);
void execute_div(instruction *i);
void execute_mod(instruction *i);
void execute_uminus(instruction *i);
void execute_and(instruction *i);
void execute_or(instruction *i);
void execute_not(instruction *i);
void execute_jeq(instruction *i);
void execute_jne(instruction *i);
void execute_jle(instruction *i);
void execute_jge(instruction *i);
void execute_jlt(instruction *i);
void execute_jgt(instruction *i);
void execute_call(instruction *i);
void execute_pusharg(instruction *i);
void execute_funcenter(instruction *i);
void execute_funcexit(instruction *i);
void execute_newtable(instruction *i);
void execute_tablegetelem(instruction *i);
void execute_tablesetelem(instruction *i);
void execute_nop(instruction *i);

generator_func_t generatorp[26] = {
    generate_ASSIGN,
    generate_ADD,
    generate_SUB,
    generate_MUL,
    generate_DIV,
    generate_MOD,
    generate_UMINUS,
    generate_AND,
    generate_OR,
    generate_NOT,
    generate_IF_EQ,
    generate_IF_NOTEQ,
    generate_IF_LESSEQ,
    generate_IF_GREATEREQ,
    generate_IF_LESS,
    generate_IF_GREATER,
    generate_CALL,
    generate_PARAM,
    generate_RETURN,
    generate_GETRETVAL,
    generate_FUNCSTART,
    generate_FUNCEND,
    generate_TABLECREATE,
    generate_TABLESETELEM,
    generate_TABLEGETELEM,
    generate_JUMP};

void generate2(void)
{
    for (unsigned i = 0; i < currQuad; ++i)
        (*generatorp[quads[i].op])(quads + i);
}

const char *VmopcodeToString(vmopcode opc)
{
    const std::map<vmopcode, const char *> OTS{
        {assign_v, "ASSIGN\t"},
        {add_v, "ADD\t"},
        {sub_v, "SUB\t"},
        {mul_v, "MULTIPLY\t"},
        {div_v, "DIV\t"},
        {mod_v, "MOD\t"},
        {uminus_v, "UMINUS\t"},
        {and_v, "AND\t"},
        {cr_v, "OR\t"},
        {not_v, "NOT\t"},
        {jeq_v, "IF_EQ\t"},
        {jne_v, "IF_NOTEQ\t"},
        {jge_v, "IF_GREATEREQ\t"},
        {jgt_v, "IF_GREATER\t"},
        {jlt_v, "IF_LESS\t"},
        {jle_v, "IF_LESSEQ\t"},
        {call_v, "CALL\t"},
        {pusharg_v, "PUSHARG\t"},
        {funcenter_v, "FUNCENTER\t"},
        {funcexit_v, "FUNCEXIT\t"},
        {newtable_v, "NEWTABLE\t"},
        {tablegetelem_v, "TABLEGETELEM\t"},
        {tablesetelem_v, "TABLESETELEM\t"},
        {nop_v, "NOP\t"},
        {jump_v, "JUMP\t"}};
    auto it = OTS.find(opc);
    return it == OTS.end() ? "*******\t" : it->second; //not checking range
}

void vmarg_toString(vmarg v)
{
    if (!v.isActive)
        return;
    switch (v.type)
    {
    case (label_a):
    {
        std::cout << "label(" << v.val << ")";
        break;
    }
    case (global_a):
    {
        std::cout << "global(" << v.val << ")";
        break;
    }
    case (formal_a):
    {
        std::cout << "formal(" << v.val << ")";
        break;
    }
    case (local_a):
    {
        std::cout << "local(" << v.val << ")";
        break;
    }
    case (number_a):
    {
        std::cout << "number(" << v.val << ")" << numConsts[v.val];
        break;
    }
    case (string_a):
    {
        std::cout << "string(" << v.val << ")" << stringConsts[v.val];
        break;
    }
    case (bool_a):
    {
        std::cout << "bool(" << v.val << ")";
        break;
    }
    case (nil_a):
    {
        std::cout << "nil";
        break;
    }
    case (userfunc_a):
    {
        std::cout << "userfunc(" << v.val << ")" << userFuncs[v.val]->id;
        break;
    }
    case (libfunc_a):
    {
        std::cout << "libfunc(" << v.val << ")" << namedLibfuncs[v.val];
        break;
    }
    case (retval_a):
    {
        std::cout << "retval";
        break;
    }
    default:
    {
        std::cout << "Default";
        break;
    }
    }
}

void InstructionPrint()
{
    patch_incomplete_jumps();
    for (unsigned i = 0; i < instrVec.size(); ++i)
    {
        
        std::cout << "No: " << i << "\t";
        std::cout << " ";
        std::cout << VmopcodeToString(instrVec[i].vmop) << " ";
        vmarg_toString(instrVec[i].result);
        std::cout << " ";
        vmarg_toString(instrVec[i].arg1);
        std::cout << " ";
        vmarg_toString(instrVec[i].arg2);

        std::cout << std::endl;
    }
}

void MagicNumber(std::ostream &wf)
{
    unsigned m = 1234;
    wf.write((char *)&m, sizeof(m));
}

void strings(std::ostream &wf)
{

    unsigned size = stringConsts.size();

    wf.write((char *)&size, sizeof(unsigned));
    for (auto i = 0; i < stringConsts.size(); ++i)
    {
        unsigned len = strlen(stringConsts[i]);
        wf.write((char *)&len, sizeof(unsigned));
        wf.write((char *)stringConsts[i], len);
    }
}

void numbers(std::ostream &wf)
{
    unsigned size = numConsts.size();
    wf.write((char *)&size, sizeof(unsigned));
    for (auto i = 0; i < numConsts.size(); ++i)
    {
        wf.write((char *)&numConsts[i], sizeof(double));
    }
}

void userfunctions(std::ostream &wf)
{
    unsigned size = userFuncs.size();
    wf.write((char *)&size, sizeof(unsigned));
    for (int i = 0; i < userFuncs.size(); ++i)
    {
        unsigned len = strlen(userFuncs[i]->id);
        wf.write((char *)&len, sizeof(unsigned));
        wf.write((char *)userFuncs[i]->id, len);
        wf.write((char *)&userFuncs[i]->address, sizeof(unsigned));
        wf.write((char *)&userFuncs[i]->localSize, sizeof(unsigned));
    }
}

void libfunctions(std::ostream &wf)
{
    unsigned size = namedLibfuncs.size();

    wf.write((char *)&size, sizeof(unsigned));
    for (auto i = 0; i < namedLibfuncs.size(); ++i)
    {
        unsigned len = strlen(namedLibfuncs[i]);
        wf.write((char *)&len, sizeof(unsigned));
        wf.write((char *)namedLibfuncs[i], len);
    }
}

void Arrays(std::ofstream &f)
{
    strings(f);
    numbers(f);
    userfunctions(f);
    libfunctions(f);
}

void Code(std::ofstream &f)
{
    unsigned size = instrVec.size();
    f.write((char *)&size, sizeof(unsigned));
    for (int i = 0; i < instrVec.size(); ++i)
    {

        f.write((char *)&instrVec[i].vmop, sizeof(enum vmopcode));
        //printf("Grafw entoli %d\n", instrVec[i].vmop);
        if (instrVec[i].arg1.isActive)
        {
          //  printf("Grafw arg1\n");
            f.write((char *)&instrVec[i].arg1.val, sizeof(unsigned));
            f.write((char *)&instrVec[i].arg1.type, sizeof(instrVec[i].arg1.type));
        }
        if (instrVec[i].arg2.isActive)
        {
            //printf("Grafw arg2\n");
            f.write((char *)&instrVec[i].arg2.val, sizeof(unsigned));
            f.write((char *)&instrVec[i].arg2.type, sizeof(instrVec[i].arg2.type));
        }
        if (instrVec[i].result.isActive)
        {
            //printf("Grafw result\n");
            f.write((char *)&instrVec[i].result.val, sizeof(unsigned));
            f.write((char *)&instrVec[i].result.type, sizeof(instrVec[i].result.type));
        }
    }
}

void AVMbinaryfile()
{
    std::ofstream f("BinaryCode.abc", std::ios::binary);

    MagicNumber(f);
    Arrays(f);
    Code(f);
    f.close();
}
