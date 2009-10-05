%{
#include <iostream>
#include <string>

static int level = 0, showlevels = 999999;
static bool in_text = false, first = true, just_opened = false;
%}

%x CMNT
%%

"<!--" {
    BEGIN CMNT;
    std::cout << yytext;
}
<CMNT>"-->" {
    BEGIN INITIAL;
    std::cout << yytext << "\n";
}
<CMNT>. |
<CMNT>\n { std::cout << yytext; }

(\<[^/!][^>]*\>)|(\<!\[CDATA\[) { bool emptyTag = false;
                 if (level < showlevels) {
                    if (!first) std::cout << '\n';
                    for (int i = 0; i < level; ++i) std::cout << ' ';
                    emptyTag = false;
                    size_t len = strlen(yytext);
                    if (yytext[len - 2] == '/')
                        emptyTag = true;
                    std::cout << yytext;
                    in_text = false;
                    just_opened = !emptyTag;
                }
                first = false;
                if (!emptyTag)
                    ++level;
              }
(\<\/[^>\n]*\>)|(\]\]\>) { if (level-- <= showlevels) {
                    if (!in_text && !just_opened) {
                        std::cout << '\n';
                        for (int i = 0; i < level; ++i)
                            std::cout << ' ';
                    }
                    std::cout << yytext;
                    just_opened = in_text = false;
                }
              }
\n            ;
[^<\]]*       { if (level <= showlevels && level) {
                    const char* p = yytext;
                    if (in_text) std::cout << ' '; 
                    else while (*p && strchr(" \t\r\n", *p)) ++p;
                    if (*p) {    
                        std::cout << p;
                        in_text = true;
                        just_opened = false;
                    }
                }
              }
          
%%

main(int ac, char **av)
{
    if (ac > 1) showlevels = atoi(av[1]);
    yylex();
    std::cout << '\n';
    return 0;
}

int yywrap() { return 1; }
int yyerror(const char *msg)
{
    std::cerr << "error: " << (msg ? msg : "unknown") << '\n';
    return 1;
}

