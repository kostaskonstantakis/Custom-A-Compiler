#include <iostream>
#include <string>
#include <list>

int token = 0;
int line = 0;
char *tmp;

class alpha_token_t
{
private:
    int numline;
    int numToken;
    std::string content;
    std::string type;
    alpha_token_t *next;

public:
    alpha_token_t();
    int getLine();
    int getToken();
    std::string getContent();
    std::string getType();
    void PrintList(std::list<alpha_token_t *> const &TokenList);
};



