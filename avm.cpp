#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <map>
#include "phase5.h"

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

unsigned totalNumConsts;
unsigned totalStringConsts;
unsigned totalNamedLibfuncs;
unsigned totalUserFuncs;
extern unsigned codeSize;

std::vector<double> numConsts;
std::vector<char *> stringConsts;
std::vector<char *> namedLibfuncs;
std::vector<struct userfunc *> userFuncs;
std::vector<struct instruction> instrVec;

void ReadBinary()
{
    std::ifstream f("BinaryCode.abc", std::ios::binary);
    //check magicnum
    unsigned m;
    f.read((char *)&m, sizeof(unsigned));
    if (m != 1234)
    {
        std::cout << "Incorrect magic number." << std::endl;
    }
    else
    {
        std::cout << "Correct magic number." << std::endl;
    }

    f.read((char *)&totalStringConsts, sizeof(unsigned));

    std::cout << totalStringConsts << std::endl;
    for (int i = 0; i < totalStringConsts; ++i)
    {
        unsigned len;
        f.read((char *)&len, sizeof(unsigned));

        char *st = (char *)malloc(len + 1);

        f.read((char *)st, len);
        st[len] = '\0';
        //add to vector
        stringConsts.push_back(st);
        std::cout << st << std::endl;
    }

    f.read((char *)&totalNumConsts, sizeof(unsigned));

    for (int i = 0; i < totalNumConsts; ++i)
    {
        double n;
        f.read((char *)&n, sizeof(double));
        numConsts.push_back(n);
        //add to vector
        std::cout << n << std::endl;
    }

    f.read((char *)&totalUserFuncs, sizeof(unsigned));
    for (int i = 0; i < totalUserFuncs; ++i)
    {
        unsigned len;
        userfunc *uf = (struct userfunc *)malloc(sizeof(userfunc));
        f.read((char *)&len, sizeof(unsigned));
        //mhkos id
        char *id = (char *)malloc(len + 1);
        //id

        f.read((char *)id, len);
        id[len] = '\0';
        uf->id = id;

        std::cout << id << std::endl;

        f.read((char *)&len, sizeof(unsigned));
        uf->address = len;

        f.read((char *)&len, sizeof(unsigned));
        uf->localSize = len;

        userFuncs.push_back(uf);
    }

    f.read((char *)&totalNamedLibfuncs, sizeof(unsigned));

    std::cout << totalNamedLibfuncs << std::endl;
    for (int i = 0; i < totalNamedLibfuncs; ++i)
    {
        unsigned len;
        f.read((char *)&len, sizeof(unsigned));

        char *st = (char *)malloc(len + 1);

        f.read((char *)st, len);
        st[len] = '\0';
        namedLibfuncs.push_back(st);
        std::cout << st << std::endl;
    }

    f.read((char *)&codeSize, sizeof(unsigned));

    for (int i = 0; i < codeSize; ++i)
    {
        instruction t;

        f.read((char *)&t.vmop, sizeof(enum vmopcode));

        //printf("Diavaze entoli %d\n", t.vmop);
        switch (t.vmop)
        {
        case assign_v:
        {

            f.read((char *)&t.arg1.val, sizeof(unsigned));
            f.read((char *)&t.arg1.type, sizeof(vmarg_t));

            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(vmarg_t));
            t.result.isActive = true;
            t.arg1.isActive = true;
            t.arg2.isActive = false;
            break;
        }
        case call_v:
        {
            f.read((char *)&t.arg1.val, sizeof(unsigned));
            f.read((char *)&t.arg1.type, sizeof(vmarg_t));
            t.arg1.isActive = true;
            t.arg2.isActive = false;
            t.result.isActive = false;
            break;
        }
        case pusharg_v:
        {

            f.read((char *)&t.arg1.val, sizeof(unsigned));
            f.read((char *)&t.arg1.type, sizeof(vmarg_t));
            t.arg1.isActive = true;
            t.arg2.isActive = false;
            t.result.isActive = false;
            break;
        }
        case funcenter_v:
        {
            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(t.result.type));
            t.result.isActive = true;
            t.arg2.isActive = false;
            t.arg1.isActive = false;
            break;
        }
        case funcexit_v:
        {
            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(t.result.type));
            t.result.isActive = true;
            t.arg2.isActive = false;
            t.arg1.isActive = false;
            break;
        }
        case newtable_v:
        {
            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(vmarg_t));
            t.result.isActive = true;
            t.arg2.isActive = false;
            t.arg1.isActive = false;

            break;
        }
        case nop_v:
        {
            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(vmarg_t));
            t.result.isActive = true;
            t.arg2.isActive = false;
            t.arg1.isActive = false;

            break;
        }
        case jump_v:
        {
            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(vmarg_t));
            t.result.isActive = true;
            t.arg2.isActive = false;
            t.arg1.isActive = false;

            break;
        }
        default:
        {
            f.read((char *)&t.arg1.val, sizeof(unsigned));
            f.read((char *)&t.arg1.type, sizeof(vmarg_t));
            t.arg1.isActive = true;

            f.read((char *)&t.arg2.val, sizeof(unsigned));
            f.read((char *)&t.arg2.type, sizeof(vmarg_t));
            t.arg2.isActive = true;

            f.read((char *)&t.result.val, sizeof(unsigned));
            f.read((char *)&t.result.type, sizeof(vmarg_t));
            t.result.isActive = true;

            break;
        }
        }

        instrVec.push_back(t);
    }
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
    }
}

void InstructionPrint()
{
    for (int i = 0; i < instrVec.size(); ++i)
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

int main()
{
    top = AVM_STACKSIZE - (1 + 130);
    avm_initialize();
    ReadBinary();
    InstructionPrint();
    for (executionFinished = false; !executionFinished;)
        execute_cycle();
}
