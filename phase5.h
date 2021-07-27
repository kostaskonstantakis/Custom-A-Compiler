//#include "phase4.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <math.h>

#define AVM_STACKSIZE 4096
#define AVM_WIPEOUT(m) memset(&(m), 0, sizeof(m))
#define AVM_TABLE_HASHSIZE 211
#define AVM_ENDING_PC codeSize
#define AVM_MAX_INSTRUCTIONS (unsigned)jump_v
#define AVM_STACKENV_SIZE 4
#define AVM_NUMACTUALS_OFFSET +4
#define AVM_SAVEDPC_OFFSET +3
#define AVM_SAVEDTOP_OFFSET +2
#define AVM_SAVEDTOPSP_OFFSET +1

extern std::vector<double> numConsts;
extern std::vector<char *> stringConsts;
extern std::vector<char *> namedLibfuncs;
extern std::vector<struct userfunc *> userFuncs;
extern std::vector<struct instruction> instrVec;

unsigned totalActuals = 0;
bool executionFinished;
unsigned pc;
unsigned codeSize = 0;
unsigned currLine = 0;
unsigned top, topsp;

typedef enum avm_memcell_t
{
    number_m = 0,
    bool_m = 2,
    string_m = 1,
    table_m = 3,
    userfunc_m = 4,
    libfunc_m = 5,
    nil_m = 6,
    undef_m = 7
} type;

struct userfunc
{
    unsigned address;
    unsigned localSize;
    char *id;
};

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

struct instruction
{
    enum vmopcode vmop;
    struct vmarg result;
    struct vmarg arg1;
    struct vmarg arg2;
    unsigned srcLine;
};

struct instruction *code = (struct instruction *)0;

std::string typeStrings[8] = {
    "number",
    "string",
    "bool",
    "table",
    "userfunc",
    "libfunc",
    "nil",
    "undef"};

struct avm_table;

typedef struct avm_memcell
{
    enum avm_memcell_t type;
    union
    {
        double numVal;
        char *strVal;
        unsigned char boolVal;
        struct avm_table *tableVal;
        unsigned funcVal;
        char *libfuncVal;
    } data;

} avm_memcell;

struct avm_memcell ax, bx, cx;
struct avm_memcell retval;

struct avm_table_bucket
{
    struct avm_memcell *key;
    struct avm_memcell *value;
    struct avm_table_bucket *next;
};

struct avm_table
{
    unsigned refCounter;
    struct avm_table_bucket *strIndexed[AVM_TABLE_HASHSIZE];
    struct avm_table_bucket *numIndexed[AVM_TABLE_HASHSIZE];
    unsigned total;
};

avm_memcell avm_stack[AVM_STACKSIZE];
static void avm_initstack()
{
    for (unsigned i = 0; i < AVM_STACKSIZE; ++i)
    {
        AVM_WIPEOUT(avm_stack[i]);
        avm_stack[i].type = undef_m;
    }
}

typedef unsigned char (*tobool_func_t)(avm_memcell *);

unsigned char number_tobool(avm_memcell *m) { return m->data.numVal != 0; }
unsigned char string_tobool(avm_memcell *m) { return m->data.strVal[0] != 0; }
unsigned char bool_tobool(avm_memcell *m) { return m->data.boolVal; }
unsigned char table_tobool(avm_memcell *m) { return 1; }
unsigned char userfunc_tobool(avm_memcell *m) { return 1; }
unsigned char libfunc_tobool(avm_memcell *m) { return 1; }
unsigned char nil_tobool(avm_memcell *m) { return 0; }
unsigned char undef_tobool(avm_memcell *m)
{
    assert(0);
    return 0;
}
void avm_error(const char *s, ...);
tobool_func_t toboolFuncs[] = {
    number_tobool,
    string_tobool,
    bool_tobool,
    table_tobool,
    userfunc_tobool,
    libfunc_tobool,
    nil_tobool,
    undef_tobool};

unsigned char avm_tobool(avm_memcell *m)
{
    assert(m->type >= 0 && m->type < undef_m);
    return (*toboolFuncs[m->type])(m);
}

void avm_tableincrefcounter(avm_table *t) { ++t->refCounter; }

void avm_tabledestroy(avm_table *t);

//automatic garbage collection for tables when reference counter gets zero
void avm_tabledecrefcounter(avm_table *t)
{
    assert(t->refCounter > 0);
    if (!(--t->refCounter))
        avm_tabledestroy(t);
}

void avm_tablebucketsinit(avm_table_bucket **p)
{
    for (unsigned i = 0; i < AVM_TABLE_HASHSIZE; ++i)
        p[i] = (avm_table_bucket *)0;
}

avm_table *avm_tablenew()
{
    avm_table *t = (avm_table *)malloc(sizeof(avm_table));
    AVM_WIPEOUT(*t);

    t->refCounter = t->total = 0;
    avm_tablebucketsinit(t->numIndexed);
    avm_tablebucketsinit(t->strIndexed);

    return t;
}
char *avm_tostring(avm_memcell *avm);
typedef void (*memclear_func_t)(avm_memcell *);

typedef char *(*tostring_func_t)(avm_memcell *);

char *number_tostring(avm_memcell *avm)
{
    std::string s = std::to_string(avm->data.numVal); /*.c_str();*/
    return strdup((char *)s.c_str());
}
char *string_tostring(avm_memcell *avm) { return avm->data.strVal; }
char *bool_tostring(avm_memcell *avm)
{
    std::string s;
    if (avm->data.boolVal)
    {
        s = std::string("true");
    }
    else
    {
        s = std::string("false");
    }
    return strdup((char *)s.c_str());
}

char *userfunc_tostring(avm_memcell *avm);
char *libfunc_tostring(avm_memcell *avm);

char *table_tostring(avm_memcell *avm)
{
    if (avm->type == table_m)
    {
        std::string strtable = "{ ";
        for (int i = 0; i < AVM_TABLE_HASHSIZE; ++i)
        {
            while (avm->data.tableVal->strIndexed[i] != NULL)
            {
                if (avm->data.tableVal->strIndexed[i]->key->type == table_m){
                    if(avm->data.tableVal->strIndexed[i]->key !=NULL)
                        strtable.append(table_tostring(avm->data.tableVal->strIndexed[i]->key));
                }
                else{
                    switch (avm->data.tableVal->strIndexed[i]->key->type){
                        case(number_m):{
                            strtable.append(number_tostring(avm->data.tableVal->strIndexed[i]->key));
                            break;
                        }
                        case(string_m):{
                            strtable.append(avm->data.tableVal->strIndexed[i]->key->data.strVal);
                            break;
                        }
                        case(userfunc_m):{
                            strtable.append(userfunc_tostring(avm->data.tableVal->strIndexed[i]->key));
                            break;
                        }
                        case(libfunc_m):{
                            strtable.append(libfunc_tostring(avm->data.tableVal->strIndexed[i]->key));
                            break;
                        }
                        case(bool_m):{
                            strtable.append(bool_tostring(avm->data.tableVal->strIndexed[i]->key));
                        }
                    }
                }

                strtable.append(": ");
                
                if (avm->data.tableVal->strIndexed[i]->value->type == table_m){
                    if(avm->data.tableVal->strIndexed[i]->value !=NULL)
                        strtable.append(table_tostring(avm->data.tableVal->strIndexed[i]->value));
                }
                else{
                    switch (avm->data.tableVal->strIndexed[i]->value->type){
                        case(number_m):{
                            strtable.append(number_tostring(avm->data.tableVal->strIndexed[i]->value));
                            break;
                        }
                        case(string_m):{
                            strtable.append(avm->data.tableVal->strIndexed[i]->value->data.strVal);
                            break;
                        }
                        case(userfunc_m):{
                            strtable.append(userfunc_tostring(avm->data.tableVal->strIndexed[i]->value));
                            break;
                        }
                        case(libfunc_m):{
                            strtable.append(libfunc_tostring(avm->data.tableVal->strIndexed[i]->value));
                            break;
                        }
                        case(bool_m):{
                            strtable.append(bool_tostring(avm->data.tableVal->strIndexed[i]->value));
                        }
                    }
                }

                avm->data.tableVal->strIndexed[i] = avm->data.tableVal->strIndexed[i]->next;
            }

            while(avm->data.tableVal->numIndexed[i] != NULL){
                if (avm->data.tableVal->numIndexed[i]->key->type == table_m){
                    if(avm->data.tableVal->numIndexed[i]->key !=NULL)
                        strtable.append(table_tostring(avm->data.tableVal->numIndexed[i]->key));
                }
                else{
                    switch (avm->data.tableVal->numIndexed[i]->key->type){
                        case(number_m):{
                            strtable.append(number_tostring(avm->data.tableVal->numIndexed[i]->key));
                            break;
                        }
                        case(string_m):{
                            strtable.append(avm->data.tableVal->numIndexed[i]->key->data.strVal);
                            break;
                        }
                        case(userfunc_m):{
                            strtable.append(userfunc_tostring(avm->data.tableVal->numIndexed[i]->key));
                            break;
                        }
                        case(libfunc_m):{
                            strtable.append(libfunc_tostring(avm->data.tableVal->numIndexed[i]->key));
                            break;
                        }
                        case(bool_m):{
                            strtable.append(bool_tostring(avm->data.tableVal->numIndexed[i]->key));
                        }
                    }
                }

                strtable.append(": ");

                if (avm->data.tableVal->numIndexed[i]->value->type == table_m){
                    if(avm->data.tableVal->numIndexed[i]->value !=NULL)
                        strtable.append(avm_tostring(avm->data.tableVal->numIndexed[i]->value));
                }
                else{
                    switch (avm->data.tableVal->numIndexed[i]->value->type){
                        case(number_m):{
                            strtable.append(number_tostring(avm->data.tableVal->numIndexed[i]->value));
                            break;
                        }
                        case(string_m):{
                            strtable.append(avm->data.tableVal->numIndexed[i]->value->data.strVal);
                            break;
                        }
                        case(userfunc_m):{
                            strtable.append(userfunc_tostring(avm->data.tableVal->numIndexed[i]->value));
                            break;
                        }
                        case(libfunc_m):{
                            strtable.append(libfunc_tostring(avm->data.tableVal->numIndexed[i]->value));
                            break;
                        }
                        case(bool_m):{
                            strtable.append(bool_tostring(avm->data.tableVal->numIndexed[i]->value));
                        }
                    }
                }
                avm->data.tableVal->numIndexed[i] = avm->data.tableVal->numIndexed[i]->next;
            }
        }
        strtable.append(" }");

        return (char *)strtable.c_str();
    }
}
char *userfunc_tostring(avm_memcell *avm)
{
    std::string s;
    s = std::string(userFuncs[avm->data.funcVal]->id);
    return strdup((char *)s.c_str());
}
char *libfunc_tostring(avm_memcell *avm)
{
    std::string s;
    s = std::string(avm->data.libfuncVal);
    return strdup((char *)s.c_str());
}
char *nil_tostring(avm_memcell *avm)
{
    return strdup("nil");
}
char *undef_tostring(avm_memcell *avm)
{
    return strdup("undef");
}

tostring_func_t tostringFuncs[8] = {

    number_tostring,
    string_tostring,
    bool_tostring,
    table_tostring,
    userfunc_tostring,
    libfunc_tostring,
    nil_tostring,
    undef_tostring};

typedef bool (*cmp_func)(double, double);

char *avm_tostring(avm_memcell *avm)
{
    assert(avm->type >= 0 && avm->type <= undef_m);
    return (*tostringFuncs[avm->type])(avm);
}

void memclear_string(avm_memcell *m)
{
    assert(m->data.strVal);
    free(m->data.strVal);
}

void memclear_table(avm_memcell *m)
{
    assert(m->data.strVal);
    avm_tabledecrefcounter(m->data.tableVal);
}

memclear_func_t memclearFuncs[] = {
    0, //number
    memclear_string,
    0, //bool
    memclear_table,
    0, //userfunc
    0, //libfunc
    0, //nil
    0  //undef
};

void avm_memcellclear(struct avm_memcell *m)
{
    if (m->type != undef_m)
    {
        memclear_func_t f = memclearFuncs[m->type];
        if (f)
            (*f)(m);
        m->type = undef_m;
    }
}

void avm_warning(std::string format) { std::cout << format << std::endl; } //maybe, more arguments
void avm_error(const char *format, ...) { std::cerr << format << std::endl; }
int avm_assign(avm_memcell *lv, avm_memcell *rv)
{
    if (lv == rv)
        avm_error("Same cells. Destructive for assign!");
    if (lv->type == table_m && rv->type == table_m && lv->data.tableVal == rv->data.tableVal)
        return 0; //same tables, no need to assign
    if (rv->type == undef_m)
        avm_warning("Assigning from 'undef' content!");

    avm_memcellclear(lv);

    memcpy(lv, rv, sizeof(avm_memcell)); //In C++, dispatch instead.

    //Now, take care of copied values of reference counting.
    if (lv->type == string_m)
        lv->data.strVal = strdup(rv->data.strVal);
    else if (lv->type == table_m)
        avm_tableincrefcounter(lv->data.tableVal);

    return 0;
}

unsigned avm_get_envvalue(unsigned i)
{
    assert(avm_stack[i].type == number_m);
    unsigned val = (unsigned)avm_stack[i].data.numVal;
    assert(avm_stack[i].data.numVal == ((double)val));
    return val;
}

void avm_tablebucketsdestroy(avm_table_bucket **p)
{

    for (unsigned i = 0; i < AVM_TABLE_HASHSIZE; ++i, ++p)
    {
        for (avm_table_bucket *b = *p; b;)
        {
            avm_table_bucket *del = b;
            b = b->next;
            avm_memcellclear(del->key);   //&del->key stin dialeksi
            avm_memcellclear(del->value); //&del->value stin dialeksi
            free(del);
        }
        p[i] = (avm_table_bucket *)0;
    }
}

void avm_tabledestroy(avm_table *t)
{
    avm_tablebucketsdestroy(t->strIndexed);
    avm_tablebucketsdestroy(t->numIndexed);
    free(t);
}

typedef bool (*cmp_func)(double, double);

avm_memcell *avm_translate_operand(vmarg *arg, avm_memcell *reg);

//NOTE: Den kserw an einai apolytws swstes, des 'tes kai 'sy!
avm_memcell *avm_tablegetelem(avm_table *table, avm_memcell *index)
{
//return table[index];
}
void avm_tablesetelem(avm_table *table, avm_memcell *index, avm_memcell *context)
{
//table[index]=context;
}

void execute_tablegetelem(instruction *instr)
{

    avm_memcell *lv = avm_translate_operand(&instr->result, (avm_memcell *)0);
    avm_memcell *t = avm_translate_operand(&instr->arg1, (avm_memcell *)0);
    avm_memcell *i = avm_translate_operand(&instr->arg2, &ax);

    assert(lv && (&avm_stack[0] <= lv && &avm_stack[top] > lv || lv == &retval));
    assert(t && &avm_stack[0] <= t && &avm_stack[top] > t);
    assert(i);

    avm_memcellclear(lv);
    lv->type = nil_m;
    if (t->type != table_m)
        avm_error("Illegal use of type %s as table!", typeStrings[t->type]);
    else
    {
        avm_memcell *content = avm_tablegetelem(t->data.tableVal, i);
        if (content)
            avm_assign(lv, content);
        else
        {
            char *ts = avm_tostring(t);
            char *is = avm_tostring(i);
            avm_error("%s[%s] not found!", ts, is); //avm_warning() originally
            free(ts);
            free(is);
        }
    }
}

void execute_tablesetelem(instruction *instr)
{
    avm_memcell *t = avm_translate_operand(&instr->result, (avm_memcell *)0);
    avm_memcell *i = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *c = avm_translate_operand(&instr->arg2, &bx);

    assert(t && &avm_stack[0] <= t && &avm_stack[top] > t);
    assert(i);

    if (t->type != table_m)
        avm_error("Illegal use of type %s as table!", typeStrings[t->type]);
    else
        avm_tablesetelem(t->data.tableVal, i, c);
}

void execute_newtable(instruction *instr)
{
    avm_memcell *lv = avm_translate_operand(&instr->result, (avm_memcell *)0);
    assert(lv && (&avm_stack[AVM_STACKSIZE-1] >= lv && &avm_stack[top] < lv || lv == &retval));

    avm_memcellclear(lv);
    lv->type = table_m;
    lv->data.tableVal = avm_tablenew();
    avm_tableincrefcounter(lv->data.tableVal);
}

void execute_jeq(instruction *instr)
{

    assert(instr->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&instr->arg2, &bx);

    unsigned char result = 0;

    if (rv1->type == undef_m || rv2->type == undef_m)
        avm_error("'undef' involved in equality!");
    else if (rv1->type == nil_m || rv2->type == nil_m)
        result = rv1->type == nil_m && rv2->type == nil_m;
    else if (rv1->type == bool_m || rv2->type == bool_m)
        result = (avm_tobool(rv1) == avm_tobool(rv2));
    else if (rv1->type != rv2->type)
        avm_error("%s==%s is illegal!", typeStrings[rv1->type], typeStrings[rv2->type]);
    else //equality check with dispatching. Arkei na kanw dispatch ws pros ton typo toy rv1
    {
        if (rv1->type == number_m)
        {
            if (rv1->data.numVal == rv2->data.numVal)
            {
                result = 1;
            }
        }
    }

    if (!executionFinished && result)
        pc = instr->result.val;
}

userfunc *avm_getfuncinfo(unsigned address)
{	
    assert(address >= 0 && address < userFuncs.size());
    return userFuncs[address];
}

unsigned avm_totalactuals()
{
    return avm_get_envvalue(topsp + AVM_NUMACTUALS_OFFSET);
}

avm_memcell *avm_getactual(unsigned i)
{
    assert(i < avm_totalactuals());
    return &avm_stack[topsp + AVM_STACKENV_SIZE + 1 + i];
}

typedef void (*library_func_t)(void);

//implementation of the library function 'print'
void libfunc_print()
{
    unsigned n = avm_totalactuals();
    for (unsigned i = 0; i < n; ++i)
    {
        char *s = avm_tostring(avm_getactual(i));
        puts(s);
        //    free(s);
    }
}

void libfunc_input()
{
    std::cout << "Awaiting input..." << std::endl;
    std::string input;
    std::cin >> input;
    if (input.compare("true"))
    {
        retval.data.boolVal = true;
        retval.type = bool_m;
        return;
    }
    else if (input.compare("false"))
    {
        retval.data.boolVal = false;
        retval.type = bool_m;
        return;
    }
    std::string::iterator it = input.begin();
    while (it != input.end() && std::isdigit(*it))
        ++it;
    if (!input.empty() && it == input.end()) //phres arigthmo
    {
        retval.data.numVal = std::atof(input.c_str());
        retval.type = number_m;
        return;
    }
    else if (!input.empty() && it != input.end()) //diavases string
    {
        if (std::isalpha(*it))
        {
            retval.data.strVal = (char *)input.c_str();
            retval.type = string_m;
            return;
        }
    }else{
        avm_error("Error! Invalid use of 'input()' ", input);
    }
}
void libfunc_objectmemberkeys() {}
void libfunc_objectcopy() {}
void libfunc_totalarguments()
{

    //get topsp of previous activation record

    unsigned p_topsp = avm_get_envvalue(topsp + AVM_SAVEDTOPSP_OFFSET);
    if (!p_topsp)
    { //If 0, no previous activation record
        avm_error("'totalarguments' called outside a function!");
        retval.type = nil_m;
    }
    else
    {
        //extract the number of actual arguments for the previous activation record
        retval.type = number_m;
        retval.data.numVal = avm_get_envvalue(p_topsp + AVM_NUMACTUALS_OFFSET);
    }
}
void libfunc_argument()
{
    unsigned n = avm_totalactuals();
    if (n != 1)
    {
        avm_error("One argument (not %d) expected in 'argument'!", n);
        executionFinished = true;
    }
    else
    {
        //that's how a lib func returns a result
        //it has to just set the retval register!
        avm_memcellclear(&retval);
        memcpy(&retval,  (0), sizeof(avm_memcell));
    }
}
void libfunc_typeof()
{

    unsigned n = avm_totalactuals();
    if (n != 1)
    {
        avm_error("One argument (not %d) expected in 'typeof'!", n);
        executionFinished = true;
    }
    else
    {
        //that's how a lib func returns a result
        //it has to just set the retval register!
        avm_memcellclear(&retval);
        retval.type = string_m;
        retval.data.strVal = strdup(typeStrings[avm_getactual(0)->type].c_str());
    }
}
void libfunc_strtonum()
{
    unsigned n = avm_totalactuals();
    if (n != 1)
    {
        avm_error("One argument (not %d) expected in 'typeof'!", n);
        executionFinished = true;
    }
    else
    {
        avm_memcellclear(&retval);
        retval.type = number_m;
        retval.data.numVal = std::atof(avm_getactual(0)->data.strVal);
    }
}
void libfunc_sqrt()
{
    unsigned n = avm_totalactuals();
    if (n != 1)
    {
        avm_error("One argument (not %d) expected in 'sqrt'!", n);
        executionFinished = true;
    }
    else
    {
        if (avm_getactual(0)->type == number_m)
        {
            //that's how a lib func returns a result
            //it has to just set the retval register!
            avm_memcellclear(&retval);
            retval.type = number_m;
            retval.data.numVal = sqrt(avm_getactual(0)->data.numVal);
        }
        else
        {
            avm_error("'sqrt' gets only arguments of type number!", n);
            executionFinished = true;
        }
    }
}
void libfunc_cos() {
    unsigned n = avm_totalactuals();
    if (n != 1)
    {
        avm_error("One argument (not %d) expected in 'cos'!", n);
        executionFinished = true;
    }
    else
    {
        if (avm_getactual(0)->type == number_m)
        {
            //that's how a lib func returns a result
            //it has to just set the retval register!
            avm_memcellclear(&retval);
            retval.type = number_m;
            retval.data.numVal = cos(avm_getactual(0)->data.numVal);
        }
        else
        {
            avm_error("'cos' gets only arguments of type number!", n);
            executionFinished = true;
        }
    }
}
void libfunc_sin() {
    unsigned n = avm_totalactuals();
    if (n != 1)
    {
        avm_error("One argument (not %d) expected in 'sin'!", n);
        executionFinished = true;
    }
    else
    {
        if (avm_getactual(0)->type == number_m)
        {
            //that's how a lib func returns a result
            //it has to just set the retval register!
            avm_memcellclear(&retval);
            retval.type = number_m;
            retval.data.numVal = sin(avm_getactual(0)->data.numVal);
        }
        else
        {
            avm_error("'sin' gets only arguments of type number!", n);
            executionFinished = true;
        }
    }
}

std::map<char *, library_func_t> registerMap;

//With the following, every library lfunction is manually added in the VM library function resolution map.
void avm_registerlibfunc(char *id, library_func_t addr)
{
    registerMap.insert(std::pair<char *, library_func_t>(id, addr));
}

void avm_initialize()
{

    avm_initstack();

    avm_registerlibfunc((char *)"print", libfunc_print);
    avm_registerlibfunc((char *)"input", libfunc_input);
    avm_registerlibfunc((char *)"objectmemberkeys", libfunc_objectmemberkeys);
    avm_registerlibfunc((char *)"objecttotalmembers", libfunc_objectmemberkeys);
    avm_registerlibfunc((char *)"objectcopy", libfunc_objectcopy);
    avm_registerlibfunc((char *)"totalarguments", libfunc_totalarguments);
    avm_registerlibfunc((char *)"argument", libfunc_argument);
    avm_registerlibfunc((char *)"typeof", libfunc_typeof);
    avm_registerlibfunc((char *)"strtonum", libfunc_strtonum);
    avm_registerlibfunc((char *)"sqrt", libfunc_sqrt);
    avm_registerlibfunc((char *)"cos", libfunc_cos);
    avm_registerlibfunc((char *)"sin", libfunc_sin);
}

//Synartisi metatropis vmargs se avm_memcell*
avm_memcell *avm_translate_operand(vmarg *arg, avm_memcell *reg)
{

    switch (arg->type)
    {
    //variables
    case global_a:
        return &avm_stack[AVM_STACKSIZE - 1 - arg->val];
    case local_a:
        return &avm_stack[topsp - arg->val];
    case formal_a:
        return &avm_stack[topsp + AVM_STACKENV_SIZE + 1 + arg->val];
    case retval_a:
        return &retval;

    case number_a:
    {
        reg->type = number_m;
        reg->data.numVal = numConsts[arg->val];
        return reg;
    }
    case string_a:
    {
        reg->type = string_m;
        reg->data.strVal = stringConsts[arg->val];
        return reg;
    }
    case bool_a:
    {
        reg->type = bool_m;
        reg->data.boolVal = arg->val;
        return reg;
    }
    case nil_a:
        reg->type = nil_m;
        return reg;

    case userfunc_a:
    {
        reg->type = userfunc_m;
        reg->data.funcVal = userFuncs[arg->val]->address;  //ARXIKA, arg->val  
        return reg;
    }
    case libfunc_a:
    {
        reg->type = libfunc_m;
        reg->data.libfuncVal = namedLibfuncs[arg->val];
        return reg;
    }
    }
}

typedef double (*arithmetic_func_t)(double d, double y);
double add_impl(double x, double y) { return x + y; }
double sub_impl(double x, double y) { return x - y; }
double mul_impl(double x, double y) { return x * y; }
double div_impl(double x, double y)
{
    if (y != 0)
        return x / y;
    else
        return EXIT_FAILURE;
}
double mod_impl(double x, double y)
{
    if (y != 0)
        return ((unsigned)x) % ((unsigned)y);
    else
        return EXIT_FAILURE;
}

arithmetic_func_t arithmeticFuncs[] = {
    add_impl,
    sub_impl,
    mul_impl,
    div_impl,
    mod_impl};

void execute_arithmetic(instruction *instr)
{

    avm_memcell *lv = avm_translate_operand(&instr->result, (avm_memcell *)0);
    avm_memcell *rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&instr->arg2, &bx);

    assert(lv && (&avm_stack[AVM_STACKSIZE - 1] >= lv && lv > &avm_stack[top] || lv == &retval));
    assert(rv1 && rv2);

    if (rv1->type != number_m || rv2->type != number_m)
    {
		
        avm_error("Not a number in arithmetic!");
        executionFinished = true;
    }
    else
    {
        arithmetic_func_t op = arithmeticFuncs[instr->vmop - add_v];
        avm_memcellclear(lv);
        lv->type = number_m;
        lv->data.numVal = (*op)(rv1->data.numVal, rv2->data.numVal);
    }
}

library_func_t avm_getlibraryfunc(char *id)
{
    std::map<char *, library_func_t>::iterator it;
    for (it = registerMap.begin(); it != registerMap.end(); it++)
    {
        if (strcmp(it->first, id) == 0)
            return it->second;
    }

    return NULL;
}

//funcexit
void execute_funcexit(instruction *unused)
{
    unsigned oldTop = top;
    top = avm_get_envvalue(topsp + AVM_SAVEDTOP_OFFSET);
    pc = avm_get_envvalue(topsp + AVM_SAVEDPC_OFFSET);
    topsp = avm_get_envvalue(topsp + AVM_SAVEDTOPSP_OFFSET);

    while (++oldTop <= top) //intentionally ignoring first
        avm_memcellclear(&avm_stack[oldTop]);
}

void avm_calllibfunc(char *id)
{
    library_func_t f = avm_getlibraryfunc(id);
    if (!f)
    {
        avm_error("Unsupported lib func '%s' called!", id);
        executionFinished = true;
    }
    else
    {

        //Notice that enter/exit functions are called manually
        topsp = top; //enter function sequence. No stack locals.
        totalActuals = 0;
        (*f)();                 //call lib func
        if (!executionFinished) //error may naturally occur inside
            execute_funcexit((instruction *)0);
    }
}

void avm_dec_top()
{
    if (!top) //stack overflow
    {
        avm_error("Stack overflow!");
        executionFinished = true;
    }

    else
        --top;
}

void avm_push_envvalue(unsigned val)
{
    avm_stack[top].type = number_m;
    avm_stack[top].data.numVal = val;
    avm_dec_top();
}

//call, swsimo perivallontos kai klhsh
void avm_callsaveenvironment()
{
    avm_push_envvalue(totalActuals);
    avm_push_envvalue(pc + 1);
    avm_push_envvalue(top + totalActuals + 2);
    avm_push_envvalue(topsp);
}

void execute_assign(instruction *i)
{
    avm_memcell *lv = avm_translate_operand(&i->result, (avm_memcell *)0);
    avm_memcell *rv = avm_translate_operand(&i->arg1, &ax);

    assert(lv && (&avm_stack[AVM_STACKSIZE - 1] >= lv && lv > &avm_stack[top] || lv == &retval));
    assert(rv);

    avm_assign(lv, rv);
}
void execute_add(instruction *i) { execute_arithmetic(i); }
void execute_sub(instruction *i) { execute_arithmetic(i); }
void execute_mul(instruction *i) { execute_arithmetic(i); }
void execute_div(instruction *i) { execute_arithmetic(i); }
void execute_mod(instruction *i) { execute_arithmetic(i); }
void execute_uminus(instruction *i) { execute_arithmetic(i); } //uminus is multiplication by -1, so it's correct, roight?!
void execute_and(instruction *i) {}
void execute_or(instruction *i) {}
void execute_not(instruction *i) {}
void execute_jne(instruction *i)
{

    assert(i->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&i->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&i->arg2, &bx);

    unsigned char result = 0;

    if (rv1->type == undef_m || rv2->type == undef_m)
        avm_error("'undef' involved in equality!");
    else if (rv1->type == nil_m || rv2->type == nil_m)
        result = rv1->type == nil_m && rv2->type == nil_m;
    else if (rv1->type == bool_m || rv2->type == bool_m)
        result = (avm_tobool(rv1) == avm_tobool(rv2));
    else if (rv1->type != rv2->type)
        avm_error("%s==%s is illegal!", typeStrings[rv1->type], typeStrings[rv2->type]);
    else //equality check with dispatching. Arkei na kanw dispatch ws pros ton typo toy rv1
    {
        if (rv1->type == number_m)
        {
            if (rv1->data.numVal == rv2->data.numVal)
            {
                result = 1;
            }
        }
    }
    if (result == 1)
        result = 0;
    else
        result = 1;

    if (!executionFinished && result)
        pc = i->result.val;
}
void execute_jle(instruction *i)
{
    assert(i->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&i->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&i->arg2, &bx);

    unsigned char result = 0;

    if (rv1->type != number_m || rv2->type != number_m)
    {
        avm_error("Not a number in arithmetic!");
        executionFinished = true;
    }
    else
    {

        if (rv1->data.numVal <= rv2->data.numVal)
        {
            result = 1;
        }
    }

    if (!executionFinished && result)
        pc = i->result.val;
}
void execute_jge(instruction *i)
{
    assert(i->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&i->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&i->arg2, &bx);

    unsigned char result = 0;

    if (rv1->type != number_m || rv2->type != number_m)
    {
        avm_error("Not a number in arithmetic!");
        executionFinished = true;
    }
    else
    {

        if (rv1->data.numVal >= rv2->data.numVal)
        {
            result = 1;
        }
    }

    if (!executionFinished && result)
        pc = i->result.val;
}
void execute_jlt(instruction *i)
{
    assert(i->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&i->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&i->arg2, &bx);

    unsigned char result = 0;

    if (rv1->type != number_m || rv2->type != number_m)
    {
        avm_error("Not a number in arithmetic!");
        executionFinished = true;
    }
    else
    {

        if (rv1->data.numVal < rv2->data.numVal)
        {
            result = 1;
        }
    }

    if (!executionFinished && result)
        pc = i->result.val;
}
void execute_jgt(instruction *i)
{
    assert(i->result.type == label_a);
    avm_memcell *rv1 = avm_translate_operand(&i->arg1, &ax);
    avm_memcell *rv2 = avm_translate_operand(&i->arg2, &bx);

    unsigned char result = 0;

    if (rv1->type != number_m || rv2->type != number_m)
    {
        avm_error("Not a number in arithmetic!");
        executionFinished = true;
    }
    else
    {

        if (rv1->data.numVal > rv2->data.numVal)
        {
            result = 1;
        }
    }

    if (!executionFinished && result)
        pc = i->result.val;
}
void execute_call(instruction *i)
{
    avm_memcell *func = avm_translate_operand(&i->arg1, &ax);
    assert(func);
    avm_callsaveenvironment();

    switch (func->type)
    {

    case userfunc_m:
    {
        pc = func->data.funcVal;
        assert(pc < AVM_ENDING_PC);
        assert(instrVec[pc].vmop == funcenter_v);
        break;
    }

    case string_m:
        avm_calllibfunc(func->data.strVal);
        break;
    case libfunc_m:
        avm_calllibfunc(func->data.libfuncVal);
        break;

    default:
    {
        char *s = avm_tostring(func);
        avm_error("Call cannot bind '%s' to function!", s);
        free(s);
        executionFinished = true;
    }
    }
}
void execute_pusharg(instruction *i)
{

    avm_memcell *arg = avm_translate_operand(&i->arg1, &ax);
    assert(arg);

    avm_assign(&avm_stack[top], arg);
    ++totalActuals;
    avm_dec_top();
}
void execute_funcenter(instruction *i)
{
    avm_memcell *func = avm_translate_operand(&i->result, &ax);
    assert(func);
    assert(pc == func->data.funcVal); //func address should match PC

    //Callee actions here.
    totalActuals = 0;
    userfunc *funcInfo = userFuncs[i->result.val];//avm_getfuncinfo(pc);
    topsp = top;
    top = top - funcInfo->localSize;
}
void execute_nop(instruction *i) {}

void execute_jump_v(instruction *i)
{
    pc = i->result.val;
}

typedef void (*execute_func_t)(instruction *);
execute_func_t executeFuncs[25] = {
    execute_assign,
    execute_add,
    execute_sub,
    execute_mul,
    execute_div,
    execute_mod,
    execute_uminus,
    execute_and,
    execute_or,
    execute_not,
    execute_jeq,
    execute_jne,
    execute_jle,
    execute_jge,
    execute_jlt,
    execute_jgt,
    execute_call,
    execute_pusharg,
    execute_funcenter,
    execute_funcexit,
    execute_newtable,
    execute_tablegetelem,
    execute_tablesetelem,
    execute_nop,
    execute_jump_v};

void execute_cycle()
{
    if (executionFinished)
        return;
    else if (pc == AVM_ENDING_PC)
    {
        executionFinished = true;
        return;
    }
    else
    {
        assert(pc < AVM_ENDING_PC);
        instruction *instr = &instrVec[0] + pc;
        assert(instr->vmop >= 0 && instr->vmop <= AVM_MAX_INSTRUCTIONS);
        if (instr->srcLine)
            currLine = instr->srcLine;
        unsigned oldPC = pc;
        (*executeFuncs[instr->vmop])(instr);
        if (pc == oldPC)
            ++pc;
    }
}
