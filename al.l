%{
    	#include "alpha_token.h"
	#include "parser.hpp"
	#include <fstream>
	#include <cstdio>
	#include <iostream>
	#ifdef __cplusplus
		static int yyinput (void );
	#else
		static int input (void );
	#endif

	using namespace std;
	/*#define YY_DECL int alpha_yylex (void* yylval)
	#endif*/
	int lineNo=0; //used to increase yylineno
	

%}

IF "if"
ELSE "else"
WHILE "while"
FOR "for"
BREAK "break"
CONTINUE "continue"
FUNCTION "function"
RETURN "return"
AND "and"
OR "or"
NOT "not"
LOCAL "local"
TRUE "true"
FALSE "false"
NIL "nil"


ASSIGN "="
PLUS "+"
MINUS "-"
MULTIPLY "*"
DIVIDE "/"
MODULO "%"
EQUAL "=="
NOT_EQUAL "!="
PLUS_PLUS "++"
MINUS_MINUS "--"
GREATER_THAN ">"
LESS_THAN "<"
GREATER_OR_EQUAL ">="
LESS_OR_EQUAL "<="

DECIMAL (0[xX]*[0-9A-Fa-f]+)|([0-9]+)
FLOAT ([0-9]*\.[0-9](e|E)?[0-9]*)

OPEN_BRACKET \{
CLOSE_BRACKET \}
OPEN_SQUARE_BRACKET \[
CLOSE_SQUARE_BRACKET \]
OPEN_PARENTHESIS \(
CLOSE_PARENTHESIS \)
SEMICOLON \;
COLON \:
DOUBLE_COLON \:\:
COMMA \,
DOT \.
DOUBLE_DOT \.\.

ID [a-zA-Z][a-zA-Z_0-9]*


COMMENT \/\/.* 
BLOCK_COMMENT "/*"


STRING \"
WHITESPACE [ \t]*

%option noyywrap 
%option yylineno

%%

"\n" {lineNo++;}
{IF}  {return IF;}
{ELSE}  {return ELSE;}
{WHILE} {return WHILE;}
{FOR}  {return FOR;}
{FUNCTION} {return FUNCTION;} 
{RETURN} {return RETURN;}	
{BREAK}    {return BREAK;}  
{CONTINUE} {return CONTINUE;}
{TRUE}	 {return TRUE;}
{FALSE} {return FALSE;}
{LOCAL}	{return LOCAL;}
{NIL}	{return NIL;}
{AND}	{return AND;}
{NOT} 	{return NOT;}
{OR}	{return OR;}
{STRING} {
						
						char character;
						int flag = 1;
						std::string str;
						while((character=yyinput())!=0){  //character=yyinput()!=NULL
							//while there's still input left
							if(character == '\\'){ //Now, it seems we've encountered a special character
								character  = yyinput();
								switch (character){
									case 'n': //newline handling
										str += '\n';
										break;
									case 't': //tab handling
										str += '\t';
										break;
									case '\\':
										str += '\\';
										break;
									
									case '"':
										str += '"';
										break;
									default:
										str += '\\';
										unput(character);//str += character;
								}
								
							}else if(character == '"'){
								std::cout << yylineno << ": #" << token << " " << "STRING" << " <-char" << "\n";
								flag = 0;
								break;
							}else{ //keep reading
								str += character;
							}
						}
						if(flag){ //Here, we have an unclosed string case 
							std::cout << yylineno << ": Error! No matching double quote. "  << "STRING "<<str << " <-char" << "\n";
						}else{
							cout << "\n\nstring:[" <<str <<"]" << endl;
						}
						yylval.stringValue = strdup(str.c_str());
						return STRING;
					}
{BLOCK_COMMENT}        {
							int character;
							int opened_flag = 1;
							int nest = 0;
							while ((character = yyinput()) !=0 && opened_flag){
								
								if (character == '*'){
									character = yyinput();
									switch (character){
										case '/':
											opened_flag--;
											break;
										default: 
											unput(character);
									}
								}else if(character == '/'){
									character = yyinput();
									switch (character){
										case '*':
											opened_flag++;
											nest++;
											break;
										default: 
											unput(character);
									}
									//unput(character);
								}
							}

							if (opened_flag){
								std::cout << " " << "BLOCK_COMMENT in line " << yylineno << " " << token << " " << "Error! Not properly closed nested comment " << yytext << "\n";
								
							}else if(nest){
								std::cout << " " << "BLOCK_COMMENT in line " << yylineno << " " << token << " " << "Properly closed nested comment " << yytext << "\n";
							}else{
								std::cout << " " << "BLOCK_COMMENT in line " << yylineno << " " << token << " " << "Properly closed comment " << yytext << "\n";
							}							
							
						}
{DECIMAL}	{
			yylval.intValue=atoi(yytext); 
			return DECIMAL; 
		}
 
{FLOAT}       	{
			yylval.realValue=atof(yytext); 
			return FLOAT; 
		} 
{ASSIGN}	{return ASSIGN;}
{PLUS}		{return PLUS;}
{MINUS}		{return MINUS;}
{MULTIPLY}	{return MULTIPLY;}
{DIVIDE}        {return DIVIDE;}
{MODULO}	{return MODULO;}
{EQUAL}		{return EQUAL;}
{NOT_EQUAL}	{return NOT_EQUAL;}
{PLUS_PLUS}	{return PLUS_PLUS;}
{MINUS_MINUS}    	{return MINUS_MINUS;}
{GREATER_THAN}		{return GREATER_THAN;}
{LESS_THAN}		{return LESS_THAN;}
{GREATER_OR_EQUAL} 	{return GREATER_OR_EQUAL;}
{LESS_OR_EQUAL}		{return LESS_OR_EQUAL;}
{OPEN_BRACKET}		{return OPEN_BRACKET;}
{CLOSE_BRACKET}		{return CLOSE_BRACKET;}
{OPEN_SQUARE_BRACKET}	{return OPEN_SQUARE_BRACKET;}
{CLOSE_SQUARE_BRACKET}	{return CLOSE_SQUARE_BRACKET;}
{OPEN_PARENTHESIS}	{return OPEN_PARENTHESIS;}
{CLOSE_PARENTHESIS}	{return CLOSE_PARENTHESIS;}
{SEMICOLON}		{return SEMICOLON;}
{COMMA}			{return COMMA;}
{COLON}			{return COLON;}
{DOUBLE_COLON}		{return DOUBLE_COLON;}
{DOT}			{return DOT;}
{DOUBLE_DOT}	        {return DOUBLE_DOT;}
{ID} 		        {  
				yylval.stringValue=strdup(yytext);
				return ID; 
				
			}

{COMMENT} 		{}					

%%

