/*
 * $Id: CdrSearch.h,v 1.1 2000-04-21 14:00:26 bkline Exp $
 *
 * Interface for CDR search implementation.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_SEARCH_
#define CDR_SEARCH_

#include "CdrString.h"

namespace cdr {

    class QueryNode {
    public:
        enum OpType { EQ, NE, GT, LT, GTE, LTE, CONTAINS, AND, OR };
        enum LValueType {
            DOC_ID, CREATOR, CREATED, VAL_STATUS, VAL_DATE,
            APPROVED, DOC_TYPE, TITLE, MODIFIED, MODIFIER, ATTR
        };
        enum NodeType { ASSERTION, BOOLEAN, NEGATION };
        enum Delimiters { STRING_MARK = 0xE001, INT_MARK = 0xE002 };
        QueryNode(const String& s1, OpType o, const String& s2)
            : lValueType(ATTR), nodeType(ASSERTION), lValueName(s1), op(o),
              rValue(s2), left(0), right(0) {}
        QueryNode(LValueType t, OpType o, const String& s)
            : lValueType(t), op(o), nodeType(ASSERTION), rValue(s), 
              left(0), right(0) {}
        QueryNode(QueryNode* n) : nodeType(NEGATION), left(0), right(n) {}
        QueryNode(QueryNode* n1, OpType o, QueryNode* n2)
            : left(n1), op(o), right(n2), nodeType(BOOLEAN) {}
        OpType      getOp()         const { return op;         }
        LValueType  getLValueType() const { return lValueType; }
        String      getLValueName() const { return lValueName; }
        String      getRValue()     const { return rValue;     }
        NodeType    getNodeType()   const { return nodeType;   }
        QueryNode*  getLeftNode()   const { return left;       }
        QueryNode*  getRightNode()  const { return right;      }
        String      getSql()        const;
    private:
        OpType      op;
        LValueType  lValueType;
        String      lValueName;
        String      rValue;
        NodeType    nodeType;
        QueryNode*  left;
        QueryNode*  right;
        String      getOpString() const;
        String      markString(const cdr::String&) const;
        String      markInt(const cdr::String&) const;
        String      strip(const cdr::String&) const;
        QueryNode(const& QueryNode);             // Block this.
        QueryNode operator=(const& QueryNode);   // Block this.
    };

    class Query {
    public:
        Query();
        ~Query() { freeNodes(tree); tree = 0; }
        void        setTree(QueryNode* n) { tree = n; }
        String      getSql();
    private:
        QueryNode*  tree;
        void        freeNodes(QueryNode*);
        bool        hasDocTypeTest;
        bool        hasAttrTest;
        bool        hasCreatorTest;
        bool        hasModifierTest;
        void        checkTablesJoined(const QueryNode*);
        Query(const Query&);                    // Block this.
        Query operator=(const& QueryNode);      // This, too.
    };

    struct QueryParam {
        QueryParam(Query* q, void* pi) : query(q), parserInput(pi) {}
        Query*      query;
        void*       parserInput;
    };
}

// Bison-generated parser
extern int CdrSearchparse(void*);

#endif
