#pragma warning(disable : 4786) // While including iostream
#include <iostream> // DEBUG

// #include <Windows.h> // if we need security descriptor
#include <process.h>    // For _getpid()
#include <string>
#include <vector>
#include "CdrDbConnection.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrLink.h"
#include "CdrLinkProcs.h"
#include "CdrLock.h"

/*
 * CdrLinkProcs.cpp
 *
 * Custom processing routines for specialized link checking.
 * See CdrLinkProcs.h.
 *
 *                                          Alan Meyer  January, 2001
 *
 * $Id$
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.21  2006/10/04 03:46:45  ameyer
 * Fixed double negation bug in handling "<>".
 * Added support for AND NOT operator.
 *
 * Revision 1.20  2006/05/17 01:52:29  ameyer
 * Added support for custom picklist processing on top of custom link
 * validation.
 *
 * Revision 1.19  2005/07/28 21:03:30  ameyer
 * Attempting to fix fumbled cvs command that added experimental version
 * 1.18 by mistake.
 *
 * Revision 1.17  2004/03/06 17:09:01  bkline
 * Removed extraneous ' character from dynamically built SQL queries.
 *
 * Revision 1.16  2004/02/10 22:15:39  ameyer
 * Added capability to check for the existence or non-existence of an
 * element in a link target.  Required changes to the validation and
 * picklist generation code.
 *
 * Revision 1.15  2003/04/15 22:30:34  ameyer
 * Modified parseTag to accept attribute paths, e.g., /A/B/@cdr:ref
 * Modified skipSpace to also skip CrLf characters.
 *
 * Revision 1.14  2002/05/09 21:39:29  ameyer
 * Fixed bug in the new code.
 *
 * Revision 1.13  2002/05/09 19:42:47  ameyer
 * Revised customLinkCheck() to free up db connection before passing it
 * down to lower level custom routines.
 *
 * Revision 1.12  2002/05/08 20:34:26  pzhang
 * Added implementation of makeQueries and getSearchLinksRespWithProp.
 *
 * Revision 1.11  2002/05/07 19:31:29  ameyer
 * Fixed bug in parenthesis handling in parseRule.
 * Added Peter's code to support "AND" and "OR" in addition to "&&" and "||".
 *
 * Revision 1.10  2002/04/09 22:36:36  bkline
 * Fixed reversed-logic error in test for conformance to a custom rule.
 *
 * Revision 1.9  2002/03/25 21:33:50  bkline
 * Fixed an uninitialized pointer bug in parseRule().
 *
 * Revision 1.8  2002/03/25 20:19:18  bkline
 * Fixed off-by-one loop error in code to parse tag in property string.
 *
 * Revision 1.7  2001/11/09 22:23:56  bkline
 * Fixed some bugs.
 *
 * Revision 1.6  2001/11/08 23:00:04  bkline
 * Tightened up the mutex handling.
 *
 * Revision 1.5  2001/11/08 19:02:41  ameyer
 * Added comment about a problem.  See XXXX.
 *
 * Revision 1.4  2001/09/28 01:20:43  ameyer
 * Completed first implementation of getLinkTargetRestrictions.
 *
 * Revision 1.3  2001/09/25 14:55:07  ameyer
 * Many new features and functions related to link target checking.
 *
 * Revision 1.2  2001/04/13 16:23:04  ameyer
 * Changed database structure for link properties table, required a
 * change to the select statement.
 *
 * Revision 1.1  2001/04/06 00:07:48  ameyer
 * Initial revision
 *
 */

// Prototypes
static bool linkTargetContainsCheck (cdr::db::Connection&,
                      cdr::link::CdrLink*, cdr::String&);
static std::string parseTag (const char **);
static cdr::link::LinkChkRelator parseRelator (const char **);
static std::string parseValue (const char **);
static cdr::link::LinkChkBoolOp parseBoolOp (const char **);
static cdr::link::LinkChkRelation *parseRelation (const char **);
static cdr::link::LinkChkPair *parseRule (const char **);
static void freePropVector (std::vector<struct PropList *> &);

/*
 * Check for, resolve, and execute custom processing routines for a link.
 * See CdrLink.h for description.
 */

// Information from the link property tables
// Must gather it in memory so we can re-use the DBMS connection
//   on each one
struct PropList {
    cdr::String name;       // Name of the property
    cdr::String value;      // Value associated with it
};

int cdr::link::customLinkCheck (
    cdr::db::Connection& conn,
    cdr::link::CdrLink*  link
) {
    std::string qry;        // For database SQL string
    int         errCount;   // Total errors for link

    std::vector<struct PropList *> propVector;
    std::vector<struct PropList *>::const_iterator i ;

    // Search the link properties table for all custom actions to
    //   take for the link type of the passed link
    // Implementation note:
    //   We could cache the information for each link type, but there is
    //     a danger that someone could modify a table without bouncing
    //     the server to flush the cache.
    //   We won't do that here, but if link checking turns out to be
    //     a significant problem, this is a possible place to look
    //     for optimization.
    qry = "SELECT t.name, p.value "
          "FROM   link_prop_type t, link_properties p "
          "WHERE  link_id = ? AND t.id = p.property_id";

    cdr::db::PreparedStatement prop_sel = conn.prepareStatement (qry);
    prop_sel.setInt (1, link->getType());
    cdr::db::ResultSet prop_rs = prop_sel.executeQuery();

    // No errors yet
    errCount = 0;

    // If any custom properties found, process each one
    try {
        while (prop_rs.next()) {

            // Need custom validation code for each property
            struct PropList *pl = new struct PropList;
            pl->name  = prop_rs.getString (1);
            pl->value = prop_rs.getString (2);
            propVector.push_back (pl);
        }

        // Done with this statement
        // Close it so lower level funcs can use connection
        prop_sel.close();

        if (!propVector.empty()) {
            // Each property must have a custom routine associated with it
            // We add the calls here, as required
            i = propVector.begin();
            while (i != propVector.end()) {

                // All supported custom property checks are listed here
                if ((*i)->name == L"LinkTargetContains")
                  errCount += linkTargetContainsCheck (conn, link, (*i)->value)
                             ? 0 : 1;
                // Add any new properties to check here
                // if ...

                // Else property isn't known to this software
                else
                    throw cdr::Exception (
                        L"Unknown link check custom routine: " + (*i)->name);
                ++i;
            }
        }
    }
    // Free objects in vector
    catch (...) {
        freePropVector (propVector);
        throw;
    }
    freePropVector (propVector);

    return errCount;
}

/**
 * Subroutine of customLinkCheck() to free allocated memory.
 */

static void freePropVector (
    std::vector<struct PropList *> &propVector
) {
    if (!propVector.empty()) {
        std::vector<struct PropList *>::const_iterator i = propVector.end();
        do {
            --i;
            delete *i;
        } while (i != propVector.begin());
    }
}


/********************************************************************
 * Link target checking
 *
 *  The following collection of classes is used in creating customized
 *  link target checking instructions.
 *
 *  A fair amount of common code is used both for validating complex
 *  link target checking (does the target of a link contain appropriate
 *  fields for this kind of link) and for pick list creation (create a
 *  list of potential target links which have appropriate fields for
 *  this kind of link.)
 **********************************************************************/

// Mutex name to prevent concurrent, conflicting updates
//   of our saved rule array.
static char s_LinkTargetMutexName[32];

/**
 * Find or create a parse tree for a link target contains rule.
 *
 * The routine maintains a static array of parse tree objects.  When
 * asked to check a certain rule it first looks to see if it already has
 * a parse of the rule.  If so, it uses that parse.  Else it parses
 * the rule.
 *
 * @param  rule         Pointer to rule to check
 *
 * @return              Top of the parse tree.
 * @throws CdrException If rule cannot be parsed.
 */
static cdr::link::LinkChkTargContains *findOrMakeRuleTree (
    cdr::String& rule
) {
    // Create mutex name, process specific
    // It's protecting data stored per process, not per machine
    if (*s_LinkTargetMutexName == 0)
        sprintf (s_LinkTargetMutexName, "LinkTargetMutex%d", _getpid());

    // We allow this many rules
    // I don't use an expandable structure like vector or list because
    //   I want to find out if there is a runaway failure, compiling
    //   the same rule multiple times.
    // If we ever exceed this number legitimately, then we must recompile
    const int maxRules = 50;

    // Static array of parsed rule objects
    static cdr::link::LinkChkTargContains *s_parsedRule[maxRules];
    static int s_parsedCount = 0;

    // Named mutex used for synchronizing access to array of
    // LinkChkTargContains objects
    static HANDLE s_LinkContainsMutex = 0;

    // Pointer to single parse tree we'll use
    cdr::link::LinkChkTargContains *ruleTree = 0;

    // Loop counter.  MSVC++ complains if this is defined in the loop
    // Non-standard MSVC extension is responsible
    int i;

    // Windows requires a security descriptor
    // This is what we have to do if we wanted a mutex that worked
    //   across processes.
    // 0 as security descriptor doesn't work right - causing failure
    //   to create or acquire the mutex
    // SECURITY_DESCRIPTOR sd;
    // InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    // SECURITY_ATTRIBUTES sa = {sizeof sa, &sd, FALSE};
    //  s_LinkContainsMutex = CreateMutex (&sa, FALSE, s_LinkTargMutex);

    // If this is the first thread in the current process, create a mutex
    //   to control the static array of rule parse trees
    if (s_LinkContainsMutex == 0)
        s_LinkContainsMutex = CreateMutex (0, FALSE, s_LinkTargetMutexName);

    // Lock and search array of parse trees to see if we can find
    //   an existing parse of the rule we want
    if (s_LinkContainsMutex != 0) {

        cdr::Lock lock(s_LinkContainsMutex, 5000);
        if (lock.m) {

            // Have exclusive ownership of the parse tree array
            // Search it for an existing parse of the passed rule
            for (i=0; i<s_parsedCount; i++) {
                if (s_parsedRule[i]->ruleCompare (rule)) {

                    // Found it
                    ruleTree = s_parsedRule[i];
                    break;
                }
            }

            // If we didn't find it, we have to create it
            // This shouldn't happen very often at all
            if (ruleTree == 0) {

                // Need room in the array
                if (s_parsedCount >= maxRules)
                    throw cdr::Exception (
                        L"Too many link proc rules, bug?");

                // Parse this rule
                // Throws exception if parse fails
                ruleTree = new cdr::link::LinkChkTargContains (rule);

                // Add it to the array
                s_parsedRule[s_parsedCount++] = ruleTree;
            }
        }
        else
            throw cdr::Exception (
                 L"Timeout waiting for mutex for link proc parse tree object");
    }
    else {
        throw cdr::Exception (
            L"Can't create link proc parse trees mutex.  Error=" +
              cdr::String::toString (GetLastError ()));
    }

    return ruleTree;
}

/**
 * Top level routine to find out if a target document contains one or
 * more fields matching a particular custom link check rule.
 *
 * The routine maintains a static array of parse tree objects.  When
 * asked to check a certain rule it first looks to see if it already has
 * a parse of the rule.  If so, it uses that parse.  Else it parses
 * the rule.
 *
 * @param  dbConn       Reference to database connection.
 * @param  link         Link object to check for conformance to rule.
 * @param  rule         Pointer to rule to check
 *
 * @return              True=Link passes rule, else false.
 * @throws CdrException if any system failure occurs.
 */
static bool linkTargetContainsCheck (
    cdr::db::Connection& dbConn,
    cdr::link::CdrLink*  link,
    cdr::String&         rule
) {
    // Find or create a parse of the rule
    cdr::link::LinkChkTargContains *ruleTree = findOrMakeRuleTree (rule);

    // Evaluate the rule
    if (!ruleTree->ruleEval (dbConn, link->getTrgId())) {

        // Failed. Compose error message
        link->addLinkErr (L"Failed link target rule: " + rule);

        // Return error count
        return 1;
    }

    // Success
    return 0;
}

/*
 * Constructor for LinkChkTargContains.
 */
cdr::link::LinkChkTargContains::LinkChkTargContains (cdr::String linkRule)
{
    // Save full copy of the rule
    ruleString = linkRule;

    // Parse the rule and save the parse tree
    // If parse fails, an exception is thrown
    // Microsoft C++ library bug workaround.  Destructor for string
    // returned by toUtf8() kicks in too soon.
    // const char *ruleStrz = ruleString.toUtf8().c_str(); -> fails!
    const std::string bugWorkaround = ruleString.toUtf8();
    const char* ruleStrz = bugWorkaround.c_str();
    treeTop = parseRule (&ruleStrz);
}


/**
 * Skip whitespace in a string.
 *
 * @param  stringpp     Ptr to ptr to cstring, will update to point past
 *                      whitespace.
 */

static void skipSpace (const char **stringpp) {
    while (**stringpp == ' ' || **stringpp == '\t' ||
           **stringpp == '\r' || **stringpp == '\n')
        ++*stringpp;
}


/**
 * Get a tag from a Tag Rel Value string.
 *
 * @param  stringpp     Ptr to ptr to null terminated string.
 *                      Update to point past tag.
 *
 * @return              Tag string for search of query_term table.
 * @throws              CdrException if no valid tag found.
 */

static std::string parseTag (const char **stringpp)
{

    const char *startp, // Pointer to start of name
         *p;            // Pointer to each char we're examining
    int  i;             // Loop counter


    // Longest tag we allow, more is almost certainly an error
    const size_t MAXTAG=512;


    // Table of characters which can be in a tag, initialized first time
    static bool s_nmchars[256];
    static bool s_first_time = true;


    // Initialize table of legal tag characters
    // This list doesn't include Unicode chars, we'll keep it simple and sane
    if (s_first_time) {
        // Legal chars in element ids
        for (i='A'; i<='Z'; i++)
            s_nmchars[i] = true;
        for (i='a'; i<='z'; i++)
            s_nmchars[i] = true;
        for (i='0'; i<='9'; i++)
            s_nmchars[i] = true;
        s_nmchars['.'] = true;
        s_nmchars['-'] = true;
        s_nmchars['_'] = true;
        // Next two are for attribute values
        s_nmchars['@'] = true;
        s_nmchars[':'] = true;
        // Not a tag char, but legal separator
        s_nmchars['/'] = true;

        s_first_time = false;
    }

    // Initialize from passed ptr to ptr
    p = startp = *stringpp;

    // Continue until we run out of tag characters
    // Note that string terminator ends check because s_nmchars[0]==false
    while (s_nmchars[*p])
        ++p;

    // Update pointer into string
    *stringpp = p;

    // Construct an std::string with the tag in it
    size_t taglen = p - startp;
    if (taglen > MAXTAG)
        throw cdr::Exception (L"Link property validation string tag has "
                              L"excessive length");
    char tagbuf[MAXTAG+1];
    memcpy (tagbuf, startp, taglen);
    tagbuf[taglen] = '\0';

    // Return the tag
    return (std::string (tagbuf));
}


/**
 * Get a tag/value relator from a Tag Rel Value string.
 *
 * @param  stringpp     Ptr to ptr to string.  Update to point past relator.
 *
 * @return              Relator constant.
 * @throws              CdrException if no valid relator found.
 */

static cdr::link::LinkChkRelator parseRelator (const char **stringpp)
{
    // Do simple, dumb comparisons to try to match against known relators
    const char *p = *stringpp;
    if (!strncmp (p, "==", 2)) {
        *stringpp += 2;
        return cdr::link::relEqual;
    }
    else if (!strncmp (p, "!=", 2)) {
        *stringpp += 2;
        return cdr::link::relNotEqual;
    }
    else if (!strncmp (p, "+=", 2)) {
        *stringpp += 2;
        return cdr::link::pickListEqual;
    }
    else if (!strncmp (p, "-=", 2)) {
        *stringpp += 2;
        return cdr::link::pickListNotEqual;
    }

    // No match where one was required
    throw cdr::Exception (L"System Error: Link check expression '" +
                          cdr::String (p) +
                          L"' missing relational operator");
}


/**
 * Get a quote delimited value from a Tag Rel Value string.
 *
 * @param  stringpp     Ptr to ptr to null terminated string.
 *                      Update to point past quote delimited value.
 *
 * @return              Relator constant.
 * @throws              CdrException if no quote delimited string found.
 */

static std::string parseValue (const char **stringpp) {
    const int MAXVAL=256;       // Max size of a value is less
    char buf[MAXVAL],           // Create string here
         *bp,                   // Ptr into buf
         *endbp;                // Ptr after end of buf
    const char *p = *stringpp;  // Ptr chars in stringpp

    // Must be a leading quote
    if (*p++ != '"')
        throw cdr::Exception (L"System Error: Link check expression "
                              L"requires quoted string value to check");

    // Copy chars until final quote
    bp    = buf;
    endbp = bp + MAXVAL-1;
    while (*p && bp < endbp) {

        // Backslash escapes the next char, whatever it is
        // Intended for escaping quote, also needed for backslash
        if (*p == '\\') {
            ++p;
            *bp++ = *p++;
        }

        // Quote not preceded by backslash ends the string
        else if (*p == '"')
            break;

        // Else it's an ordinary string character
        else
            *bp++ = *p++;
    }

    // Off the end with no trailing quote?
    if (!*p)
        throw cdr::Exception (L"System Error: Link check expression "
                              L"missing trailing quote");

    // Too long?  Can't really happen unless we redefine the table columns
    if (bp >= endbp)
        throw cdr::Exception (L"System Error: Link check expression "
                              L"has too long value");

    // Tell caller where we're pointing now - after trailing quote
    *stringpp = p + 1;

    // Construct and return value string, sans quotes
    *bp = '\0';
    return (std::string (buf));
}


/**
 * Get a boolean operator connecting two or more tag/rel/value strings.
 *
 * @param  stringpp     Ptr to ptr to null terminated string.
 *                      Update to point past operator.
 *
 * @return              Operator constant.
 * @throws              CdrException if no valid operator found.
 */

static cdr::link::LinkChkBoolOp parseBoolOp (const char **stringpp)
{
    // Simple comparisons
    const char *p = *stringpp;
    cdr::link::LinkChkBoolOp op;

    if (*p == '|') {
        while (*p == '|')
            ++p;
        op = cdr::link::boolOr;
    }
    else if (*p == '&') {
        while (*p == '&')
            ++p;
        op = cdr::link::boolAnd;
    }
    else if (!_strnicmp (p, "and", 3)) {
         p += 3;
         op = cdr::link::boolAnd;

         // Is it an "and not"?
         const char *q = p;
         skipSpace(&q);
         if (!_strnicmp (q, "not", 3)) {
             p = q + 3;
             op = cdr::link::boolAndNot;
         }
    }
    else if (!_strnicmp (p, "or", 2)) {
         p += 2;
         op = cdr::link::boolOr;
    }
    else
        // No match where one was required
        throw cdr::Exception (L"System Error: Link check expression "
                              L"missing boolean operator");

    // Point past operator
    *stringpp = p;

    return op;
}


/**
 * Get a complete tag/relator/value expression.
 *
 * @param  stringpp     Ptr to ptr to string.  Update to point past operator.
 *
 * @return              Ptr to new relation object.
 * @throws              CdrException if syntax or other error.
 */

static cdr::link::LinkChkRelation *parseRelation (const char **stringpp)
{
    std::string    tag;         // Tag in format used in query_term table
    std::string    value;       // Value to compare against linked doc
    cdr::link::LinkChkRelator relator; // Relationship
    bool           chkValue = true;    // True=Look at element value
                                       // False=Just look at element existence

    // Skip initial whitespace
    skipSpace (stringpp);

    // Get the three elements of a relation, ignoring any whitespace
    tag     = parseTag (stringpp);
    skipSpace (stringpp);
    relator = parseRelator (stringpp);
    skipSpace (stringpp);

    // Value may be string, or '*', indicating specific value doesn't matter
    if (**stringpp == '*') {
        chkValue = false;
        ++*stringpp;
    }
    else
        value = parseValue (stringpp);
    skipSpace (stringpp);

    // If we got this far without throwing an exception, we have all we need
    return new cdr::link::LinkChkRelation (tag, relator, value, chkValue);
}


/**
 * Parse a link check validation rule, creating an executable parse tree.
 *
 * @param  stringp      Ptr to string containing the rule.
 *
 * @return              Ptr to parse tree.
 * @throws              CdrException if syntax or other error.
 */

static cdr::link::LinkChkPair *parseRule (const char **rulepp)
{
    // Data to put in the top of the parse tree
    cdr::link::LinkChkNode   *leftNode     = 0;
    cdr::link::LinkChkNode   *rightNode    = 0;
    cdr::link::LinkChkBoolOp boolConnector = cdr::link::boolOr;

    // Recursive descent parser
    do {
        // Leading whitespace
        skipSpace (rulepp);

        // Open parens requires a descent
        if (**rulepp == '(') {
            // Skip over the paren
            ++*rulepp;

            // Go deeper to get the left node, might itself go deeper
            leftNode = parseRule (rulepp);

            // Should now see the matching right paren
            if (**rulepp != ')')
                throw cdr::Exception (L"System Error: Link check expression "
                                      L"syntax error");
            // Skip over it
            ++*rulepp;
        }
        else
            // There should be a tag/rel/value expression here
            leftNode = parseRelation (rulepp);

        skipSpace (rulepp);

        // There might only have been one expression, or there might
        //   be more than one, connected by boolean operators
        // We are done this invocation of parseRule if:
        //   We reached the end of string, ending the entire parse, or
        //   We reached a right paren, ending this subexpression
        //     In that case we'll un-recurse
        //     Caller will see the right paren and act on it
        if (**rulepp && **rulepp != ')') {

            // There has to be a connector here, if not, an excecption
            boolConnector = parseBoolOp (rulepp);

            // Right node can be anything, including a parenthesized
            //   expression, a single relation, a list of relations
            //   separated by boolean operators, or whatever.
            // Doesn't matter what it is, a recursive descent should
            //   parse it correctly if it's well formed.
            rightNode = parseRule (rulepp);
        }
        // Continue until end of rule subexpression or end of string
    } while (**rulepp && **rulepp != ')');

    skipSpace (rulepp);

    // If we got this far, no syntax errors
    return new cdr::link::LinkChkPair (leftNode, rightNode, boolConnector);
}


/*
 * Evaluate a single relation expression.
 */

bool cdr::link::LinkChkRelation::evalRelation (
    cdr::db::Connection& dbConn,
    int                  docId
) {
    // These evaluations are only done in the context of validation,
    //   not picklist generation.
    // Always evaluate picklist operators as true.
    if (relator == pickListEqual || relator == pickListNotEqual)
        return true;

    // Check query terms for at least one of this field + value + docId.
    std::string qry = "SELECT TOP 1 doc_id "
                      "  FROM query_term "
                      " WHERE doc_id = ? "
                      "   AND path = ?";

    // If checking for a specific value, retrieve it too
    if (chkValue)
        qry += " AND value = ?";

    cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
    stmt.setInt (1, docId);
    stmt.setString (2, tag);
    if (chkValue)
        stmt.setString (3, value);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Are we looking for equality or inequality?
    // We don't actually need to examine anything, all we need to know is ...
    // Was there a hit?
    if (rs.next())
        return relator == cdr::link::relEqual;

    // If got here, then no hit in query term table
    else
        return relator != cdr::link::relEqual;
}


/*
 * Evaluate a complete parse tree.
 */

bool cdr::link::LinkChkPair::evalRule (
    cdr::db::Connection& dbConn,
    int                  docId
) {
    bool result;     // Return code


    // Ultimately, at the leaves of the tree, all objects to evaluate are
    //   tag/rel/value relations.
    // If a particular node is a leaf, call evalRelation() to evaluate it.
    // Else call evalRule() recursively to process the subtree.
    if (lNode->getNodeType() == typePair)
        result = ((cdr::link::LinkChkPair *) lNode)->evalRule (dbConn, docId);
    else
        result =
          ((cdr::link::LinkChkRelation *) lNode)->evalRelation (dbConn, docId);

    // See if there is no right hand node, or if we can short circuit it
    if (!rNode)
        return result;
    if (connector == cdr::link::boolAnd && result == false)
        return result;
    if (connector == cdr::link::boolAndNot && result == false)
        return result;
    if (connector == cdr::link::boolOr && result == true)
        return result;

    // Have to evaluate the right hand side of the node pair
    // Since we are here:
    //     connector == AND and result == true
    //   or
    //     connector == OR and result == false
    // In either case we can just return the result of the evaluation
    //   of the right hand node
    if (rNode->getNodeType() == typePair)
        return (((cdr::link::LinkChkPair *) rNode)->evalRule (dbConn, docId));
    else
        return (
         ((cdr::link::LinkChkRelation *) rNode)->evalRelation (dbConn, docId));
}


/**
 * Generate sub-queries for a complete parse tree.
 *
 * There are problems with SQL server executing a query containing
 *      3 query_term tables joined, although joining 2 tables seems
 *      fine. We hence drop that idea and generate this specific version
 *      of subqueries from the parse tree.
 *
 * @param  query      Ptr to string containing the query to be returned.
 *
 * @param  cdrId      A table alias followed by a column name for CDR Id.
 *
 * @throws            CdrException if syntax or other error.
 */

void cdr::link::LinkChkNode::makeSubQueries (
    std::string& query,
    std::string& cdrid
) {
    // If we're at a leaf node (cdr::link::LinkChkRelation), generate SQL
    if (this->getNodeType() == typeRel) {

        cdr::link::LinkChkRelation *node =
                static_cast<cdr::link::LinkChkRelation*> (this);

        // This function supports picklist generation
        // Use the picklist as well as validation functions
        // If node tests for not equal or not exists, we need to
        //   be sure the value does not exist in selected docs
        if (node->getRelator() == cdr::link::relNotEqual ||
            node->getRelator() == cdr::link::pickListNotEqual)
            query += " NOT";

        // Subquery for doc ids
        query += " EXISTS (SELECT qt.doc_id FROM query_term qt "
                 " WHERE " + cdrid + " = qt.doc_id AND ";

        // Append tag to search for in query_term table
        query += "qt.path ='";
        query += node->getTag();
        query += "'";

        // If just checking for existence or non-existence of an element,
        //   we're done, else we need to check the value
        if (node->getChkValue()) {
            // Connector for value
            query += " AND ";

            // Value equals 'value'
            // Note that if it's != or -=, we handled that with NOT above
            query += "qt.value='";
            query += node->getValue();
            query += "'";
        }

        query += ")";
    }

    // Else it's an intermediate node. Recursive descent.
    else {

        // Parenthesize this node.
        query += "(";

        cdr::link::LinkChkPair *node  =
                    static_cast<cdr::link::LinkChkPair*> (this);
        cdr::link::LinkChkNode *lNode = node->getLNode();
        cdr::link::LinkChkNode *rNode = node->getRNode();

        // There's always a left node
        lNode->makeSubQueries (query, cdrid);

        // Not always a right node
        if (rNode) {
            // Output the appropriate boolean operator between nodes
            if (node->getConnector() == cdr::link::boolAnd)
                query += " AND ";
            else if (node->getConnector() == cdr::link::boolAndNot)
                query += " AND NOT ";
            else if (node->getConnector() == cdr::link::boolOr)
                query += " OR ";
            else
                throw cdr::Exception (L"Invalid connector, can't happen");
            rNode->makeSubQueries (query, cdrid);
        }

        query += ")";
    }
// DEBUG
// std::cout << "LinkProcsQry=\"" << query << "\"" << std::endl;
}

/**
 * See full comment prolog in CdrLink.h.
 */
cdr::String cdr::link::getSearchLinksRespWithProp (
        cdr::db::Connection&      conn,
        int                       link_id,
        std::vector<int>&         prop_ids,
        std::vector<cdr::String>& prop_values,
        const cdr::String&        titlePattern,
        int                       maxRows)
{
    std::string qry = "SELECT DISTINCT ";
    if (maxRows > 0) {
        char buf[40];
        sprintf(buf, "TOP %d ", maxRows);
        qry += buf;
    }
    qry += " d.id, d.title"
           "  FROM document d, link_target lt"
           " WHERE d.doc_type = lt.target_doc_type"
           "   AND d.title LIKE ?"
           "   AND lt.source_link_type = ?"
           "   AND (";

    std::string placeHolder = "";
    for (size_t i = 0; i < prop_values.size(); ++i) {
        cdr::link::LinkChkTargContains *ruleTree =
            findOrMakeRuleTree (prop_values[i]);
        cdr::link::LinkChkPair         *treeTop  =
            ruleTree->getTreeTop();
        std::string query;
        std::string cdrid = "d.id";
        treeTop->makeSubQueries (query, cdrid);
        qry += placeHolder + query ;
        placeHolder = ") AND (";
    }
    qry += ")";
    qry += " ORDER BY d.title";

// DEBUG
// cdr::log::WriteFile(L"getSearchLinksRespWithProp", cdr::String(qry));
// std::cout <<"getSearchLinksRespWithProp, qry=\"" <<qry <<"\"" <<std::endl;

    // Submit the query to the DBMS.
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setString(1, titlePattern);
    stmt.setInt(2, link_id);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Construct the response.
    cdr::String response = L"<CdrSearchLinksResp>";
    int rows = 0;
    while (rs.next()) {
        int         id      = rs.getInt(1);
        cdr::String title   = rs.getString(2);

        if (rows++ == 0)
            response += L"<QueryResults>";
        wchar_t tmp[1000];
        swprintf(tmp, L"<QueryResult><DocId>CDR%010ld</DocId>"
                      L"<DocTitle>%.500s</DocTitle>"
                      L"</QueryResult>",
                 id, cdr::entConvert(title).c_str());
        response += tmp;
    }
    if (rows > 0)
        response += L"</QueryResults></CdrSearchLinksResp>";
    else
        response += L"<QueryResults/></CdrSearchLinksResp>";

    return response;
}
