/*
 * $Id: CdrSearch.h,v 1.4 2000-04-26 01:37:28 bkline Exp $
 *
 * Interface for CDR search implementation.  Used by implementation of search
 * command and by the query parser.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.2  2000/04/22 18:01:26  bkline
 * Fleshed out documentation comments.
 *
 * Revision 1.1  2000/04/21 14:00:26  bkline
 * Initial revision
 */

#ifndef CDR_SEARCH_
#define CDR_SEARCH_

#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * The <code>QueryNode</code> class represents a node in a CDR query tree.
     * A node can be a simple assertion (such as a test of a control value --
     * e.g., the date the document was created -- or a named attribute), a
     * negation of such an assertion or set of assertions, or a boolean 
     * combination of two nodes.
     */
    class QueryNode {
    public:
        
        /**
         * The three types of nodes which can be found in a CDR query tree.
         */
        enum NodeType { ASSERTION, BOOLEAN, NEGATION };

        enum Delimiters { STRING_MARK = 0xE001, INT_MARK = 0xE002 };
        /**
         * An <code>OpType</code> represents either the type of an operator
         * used in a simple assertion (for example, an equality operator),
         * or the type of boolean combination of two nodes found in the query
         * (AND or OR).
         */
        enum OpType { EQ, NE, GT, LT, GTE, LTE, CONTAINS, AND, OR };

        /**
         * The current implementation of CDR searching allows two types of
         * tests, as well as combinations and/or negations of those atomic
         * tests.  One type involves the test of a named document attribute.
         * The names available for named attributes is under the control of
         * the users.  In contrast, there is a fixed set of "control"
         * value types, derived from information in the document table itself.
         * The first type of test has an <code>LValueType</code> of 
         * <code>ATTR</code>.  The other <code>LValueType</code>s in the list
         * below are available for "control" value tests.
         */
        enum LValueType {
            DOC_ID, CREATOR, CREATED, VAL_STATUS, VAL_DATE,
            APPROVED, DOC_TYPE, TITLE, MODIFIED, MODIFIER, ATTR
        };

        /**
         * Constructor for a node representing a test of a named attribute.
         */
        QueryNode(const String& s1, OpType o, const String& s2)
            : lValueType(ATTR), nodeType(ASSERTION), lValueName(s1), op(o),
              rValue(s2), left(0), right(0) {}

        /**
         * Constructor for a node representing a test of a document "control"
         * value.
         */
        QueryNode(LValueType t, OpType o, const String& s)
            : lValueType(t), op(o), nodeType(ASSERTION), rValue(s), 
              left(0), right(0) {}

        /**
         * Constructor for a node representing negation of another node.
         */
        QueryNode(QueryNode* n) : nodeType(NEGATION), left(0), right(n) {}

        /**
         * Constructor for a node representing a boolean combination of two
         * other nodes.
         */
        QueryNode(QueryNode* n1, OpType o, QueryNode* n2)
            : left(n1), op(o), right(n2), nodeType(BOOLEAN) {}

        /**
         * Access methods.
         */
        OpType      getOp()         const { return op;         }
        LValueType  getLValueType() const { return lValueType; }
        String      getLValueName() const { return lValueName; }
        String      getRValue()     const { return rValue;     }
        NodeType    getNodeType()   const { return nodeType;   }
        QueryNode*  getLeftNode()   const { return left;       }
        QueryNode*  getRightNode()  const { return right;      }

        /**
         * Recursively constructs the portion of the WHERE clause representing
         * the tests for this node in the query tree.
         */
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

    /**
     * The <code>Query</code> class represents the tree of nodes for a parsed
     * CDR query.
     */
    class Query {
    public:

        /**
         * Creates an empty tree, to be populated during parsing.
         */
        Query();

        /**
         * Cleans up resources alloced for the query tree.
         */
        ~Query() { freeNodes(tree); tree = 0; }

        /**
         * Invoked by the query parser to plug in the head of the parsed tree
         * for the CDR query.
         */
        void        setTree(QueryNode* n) { tree = n; }

        /**
         * Generates a <code>String</code> containing the SQL for the query.
         * Most of the work is handled by the recursive work of the
         * <code>getSql()</code> method of <code>QueryNode</code>.
         */
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

    /**
     * The <code>QueryParam</code> type is used to wrap up the information
     * passed to the parser and the lexical analyzer.  The address of an
     * instance of this type is passed to the parser as a void* pointer,
     * and cast back to the correct type in side the parser.  Similarly,
     * the parser passes the <code>parserInput</code> member to the lexical
     * analyzer, which in turn casts it back to a pointer to
     * <code>ParserInput</code>.
     */
    struct QueryParam {

        /**
         * Creates a wrapper for the pointers to the private information 
         * needed by the parser and lexical analyzer.
         */
        QueryParam(Query* q, void* pi) : query(q), parserInput(pi) {}
        Query*      query;
        void*       parserInput;
    };

}

/**
 * Bison-generated parser.  One of the few public names not wrapped in the
 * <code>cdr</code> namespace (which we can't do because we don't have enough
 * control over the crude methods by the parser generator to create names).
 * At least we can change the "yy" prefix so the system can have more than one
 * parser type in the same global namespace.
 */
extern int CdrSearchparse(void*);

#endif
