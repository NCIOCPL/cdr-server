%{
/*
 * $Id: CdrSearch.y,v 1.1 2000-04-21 13:53:59 bkline Exp $
 *
 * Parser for CDR Search module.
 *
 * $Log: not supported by cvs2svn $
 */

/*
                         Grammar for query parser.
                         -------------------------

          Query ::= Root Filter? DocId
           Root ::= '//CdrDoc'
         Filter ::= '[' Assertion ']'
          DocId ::= '/CdrCtl/DocId'
    Disjunction ::= Conjuction | Conjunction S 'or' S Disjunction
    Conjunction ::= Assertion | Assertion S 'and' S Conjuction
      Assertion ::= Comparison | '(' Assertion ')' | Negation
     Comparison ::= LValue S ComparisonOp S? RValue
         LValue ::= CtlPath | AttrPath
        CtlPath ::= 'CdrCtl/' CtlElement
     CtlElement ::= 'DocId' | 'Creator' | 'Created' | 'ValStatus' | 
                    'ValDate' | 'Approved' | 'DocType' | 'Title' |
                    'Modified' | 'Modifier'
       AttrPath ::= 'CdrAttr/' AttrName
       AttrName ::= NameStart NameChar*
      NameStart ::= Letter
         Letter ::= Uppercase | Lowercase
      Uppercase ::= 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' |
                    'I' | 'J' | 'K' | 'L' | 'M' | 'N' | 'O' | 'P' |
                    'Q' | 'R' | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' |
                    'Y' | 'Z'
      Lowercase ::= 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' |
                    'i' | 'j' | 'k' | 'l' | 'm' | 'n' | 'o' | 'p' |
                    'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w' | 'x' |
                    'y' | 'z'
       NameChar ::= Letter | Digit | '-' | '_' | '.' | ':'
          Digit ::= '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' |
                    '8' | '9'
   ComparisonOp ::= EqualityOp | InequalityOp | SubstringOp
     EqualityOp ::= '=' | 'eq'
   InequalityOp ::= '!=' | 'ne' | 'lt' | 'gt' | 'lte' | 'gte'
    SubstringOp ::= 'contains'
         RValue ::= "'" String "'" | '"' String '"'
         String ::= ("\'" | [^'#xD#xA#x9])* | ('\"' | [^"#xD#xA#x9])*
       Negation ::= 'not(' Disjunction ')'
              S ::= (#x20 | #x9 | #xD | #xA)+

*/

#include <iostream>
#include "CdrString.h"
#include "CdrSearch.h"
#include "CdrParserInput.h"

static int yylex(void*, void*);
static void yyerror(char*);

static cdr::QueryNode::OpType comparisonOp;

#define YYPARSE_PARAM parserParam
#define YYLEX_PARAM const_cast<cdr::QueryParam*>(parserParam)->parserInput

%}

%pure_parser

%union {
    const wchar_t*              str;
    cdr::QueryNode*             node;
    cdr::QueryNode::OpType      op;
    cdr::QueryNode::LValueType  ctl;
}

%token  <keyword>   And
%token  <keyword>   Or
%token  <keyword>   Not
%token  <keyword>   Eq
%token  <keyword>   Ne
%token  <keyword>   Lt
%token  <keyword>   Gt
%token  <keyword>   Lte
%token  <keyword>   Gte
%token  <keyword>   Contains
%token  <keyword>   Root
%token  <keyword>   DocId
%token  <keyword>   CtlDocId
%token  <keyword>   CtlCreator
%token  <keyword>   CtlCreated
%token  <keyword>   CtlValStatus
%token  <keyword>   CtlValDate
%token  <keyword>   CtlApproved
%token  <keyword>   CtlDocType
%token  <keyword>   CtlTitle
%token  <keyword>   CtlModified
%token  <keyword>   CtlModifier
%token  <str>       AttrName
%token  <str>       RValue
%type   <node>      Filter
%type   <node>      Assertion
%type   <node>      Comparison
%type   <op>        ComparisonOp
%type   <ctl>       CtlPath
%left               Or
%left               And

%%
             
          Query : Root DocId {}
                | Root Filter DocId 
                { 
                    cdr::QueryParam* qp;
                    qp = static_cast<cdr::QueryParam*>(parserParam);
                    qp->query->setTree($2);
                }
         Filter : '[' Assertion ']' { $$ = $2; }
      Assertion : Comparison { $$ = $1; }
                | Not '(' Assertion ')' { $$ = new cdr::QueryNode($3); }
                | '(' Assertion ')' { $$ = $2; }
                | Assertion And Assertion 
                {
                    $$ = new cdr::QueryNode($1, cdr::QueryNode::AND, $3);
                }
                | Assertion Or Assertion
                {
                      $$ = new cdr::QueryNode($1, cdr::QueryNode::OR, $3);
                }
     Comparison : AttrName ComparisonOp RValue 
                {
                    $$ = new cdr::QueryNode($1, $2, $3);
                }
                | CtlPath ComparisonOp RValue 
                {
                    $$ = new cdr::QueryNode($1, $2, $3);
                }
   ComparisonOp : Eq           { $$ = cdr::QueryNode::EQ;         }
                | Ne           { $$ = cdr::QueryNode::NE;         }
                | Gt           { $$ = cdr::QueryNode::GT;         }
                | Lt           { $$ = cdr::QueryNode::LT;         }
                | Gte          { $$ = cdr::QueryNode::GTE;        }
                | Lte          { $$ = cdr::QueryNode::LTE;        }
                | Contains     { $$ = cdr::QueryNode::CONTAINS;   }
        CtlPath : CtlDocId     { $$ = cdr::QueryNode::DOC_ID;     }
                | CtlCreator   { $$ = cdr::QueryNode::CREATOR;    }
                | CtlCreated   { $$ = cdr::QueryNode::CREATED;    }
                | CtlValStatus { $$ = cdr::QueryNode::VAL_STATUS; }
                | CtlValDate   { $$ = cdr::QueryNode::VAL_DATE;   }
                | CtlApproved  { $$ = cdr::QueryNode::APPROVED;   }
                | CtlDocType   { $$ = cdr::QueryNode::DOC_TYPE;   }
                | CtlTitle     { $$ = cdr::QueryNode::TITLE;      }
                | CtlModified  { $$ = cdr::QueryNode::MODIFIED;   }
                | CtlModifier  { $$ = cdr::QueryNode::MODIFIER;   }

%%

int yylex(void* parm1, void* parm2)
{
    YYSTYPE* yylval = const_cast<YYSTYPE*>(parm1);
    cdr::ParserInput& pi = *const_cast<cdr::ParserInput*>(parm2);
    for (;;) {
        wchar_t c = *pi;
        switch (c) {
            case 0:
                return pi.setLastTok(0);
            case '[': case ']': case '(': case ')': 
                ++pi;
                return pi.setLastTok(c);
            case ' ': case '\t': case '\r': case '\n': 
                ++pi;
                continue;
            case '\'': case '"':
                ++pi;
                yylval->str = pi.extractString(c);
                return pi.setLastTok(RValue);
        }
        if (pi.matches(L"not(")) {
            pi += 3; // Keep '(' for later.
            return pi.setLastTok(Not);
        }
        static struct { const wchar_t* str; int tok; } tokmap[] = {
            { L"and",               And           },
            { L"or",                Or            },
            { L"=",                 Eq            },
            { L"eq",                Eq            },
            { L"!=",                Ne            },
            { L"ne",                Ne            },
            { L"lt",                Lt            },
            { L"gt",                Gt            },
            { L"lte",               Lte           },
            { L"gte",               Gte           },
            { L"contains",          Contains      },
            { L"//CdrDoc",          Root          },
            { L"/CdrCtl/DocId",     DocId         },
            { L"CdrCtl/DocId",      CtlDocId      },
            { L"CdrCtl/Creator",    CtlCreator    },
            { L"CdrCtl/Created",    CtlCreated    },
            { L"CdrCtl/ValStatus",  CtlValStatus  },
            { L"CdrCtl/ValDate",    CtlValDate    },
            { L"CdrCtl/Approved",   CtlApproved   },
            { L"CdrCtl/DocType",    CtlDocType    },
            { L"CdrCtl/Title",      CtlTitle      },
            { L"CdrCtl/Modified",   CtlModified   },
            { L"CdrCtl/Modifier",   CtlModifier   }
        };
        for (int i = 0; i < sizeof tokmap / sizeof *tokmap; ++i) {
            if (pi.matches(tokmap[i].str)) {
                pi += wcslen(tokmap[i].str);
                return pi.setLastTok(tokmap[i].tok);
            }
        }
        if (pi.matches(L"CdrAttr/")) {
            pi += 8;
            size_t len = 0;
            const wchar_t* p = pi.getRemainingBuf();
            while (p[len]) {
                unsigned c = static_cast<unsigned>(p[len]);
                int      i = static_cast<int>(c);
                if (c > 127)
                    break;
                if (len == 0 && !isalpha(i))
                    break;
                if (!isalpha(i) && !isdigit(i) && !wcschr(L"-_.:", p[len]))
                    break;
                ++len;
            }
            yylval->str = pi.extractString(len);
            return pi.setLastTok(AttrName);
        }
        return pi.setLastTok(0);
    }
    return pi.setLastTok(0);
}

void yyerror(char* s)
{
    std::cerr << "yyerror: " << s << std::endl;
    throw cdr::Exception(L"Parse error", cdr::String(s));
}


#ifdef TESTMAIN

main()
{
    cdr::String line;
    while (std::getline(std::wcin, line)) {
        try {
            cdr::ParserInput input(line);
            cdr::Query query;
            cdr::QueryParam qp(&query, &input);
            std::wcerr << L"***************************************\n";
            std::wcerr << line << L"\n";
            std::wcerr << L"***************************************\n";
            yyparse(static_cast<void*>(&qp));
            std::wcerr << L"---------------------------------------\n";
            std::wcerr << query.getSql() << L"\n";
        }
        catch (cdr::Exception e) {
            std::wcerr << L"Caught CDR Exception: " 
                       << e.getString() 
                       << std::endl;
        }
    }
    return 0;
}

#endif
