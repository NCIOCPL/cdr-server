#pragma warning(disable : 4786) // While including iostream
#include <iostream> // DEBUG

#include <string>
#include "CdrDbConnection.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrLink.h"
#include "CdrLinkProcs.h"

/*
 * CdrLinkProcs.cpp
 *
 * Custom processing routines for specialized link checking.
 * See CdrLinkProcs.h.
 *
 *                                          Alan Meyer  January, 2001
 *
 * $Id: CdrLinkProcs.cpp,v 1.2 2001-04-13 16:23:04 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/04/06 00:07:48  ameyer
 * Initial revision
 *
 */

// Prototypes
static bool LinkTargetContainsCheck (cdr::db::Connection&,
                      cdr::link::CdrLink*, cdr::String&);
static std::string parseTag (const char **);
static LinkChkRelator parseRelator (const char **);
static std::string parseValue (const char **);
static LinkChkBoolOp parseBoolOp (const char **);
static LinkChkRelation *parseRelation (const char **);
static LinkChkPair *parseRule (const char **);

/*
 * Check for, resolve, and execute custom processing routines for a link.
 * See CdrLink.h for description.
 */

int cdr::link::customLinkCheck (
    cdr::db::Connection& conn,
    cdr::link::CdrLink*  link
) {
    std::string qry;        // For database SQL string
    int         errCount;   // Total errors for link


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
    while (prop_rs.next()) {

        // Need custom validation code for each property
        cdr::String prop_name  = prop_rs.getString (1);
        cdr::String prop_value = prop_rs.getString (2);

        // Each property must have a custom routine associated with it
        // We add the calls here, as required
        if (prop_name == L"LinkTargetContains")
            errCount += LinkTargetContainsCheck (conn, link, prop_value)
                     ? 0 : 1;

        else
            throw cdr::Exception (L"Unknown link check custom routine: "
                                  + prop_name);
    }

    return errCount;
}


/********************************************************************
 * Link target checking
 *
 *  The following collection of classes is used in creating customized
 *  link target checking instructions.
 **********************************************************************/

const static char *LinkTargMutex = "LinkTargetMutex";

/**
 * Top level routine to find out if a target document contains one or
 * more fields matching a particular custom link check rule.
 *
 * The routine maintains a static vector of parse tree objects.  When
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

static bool LinkTargetContainsCheck (
    cdr::db::Connection& dbConn,
    cdr::link::CdrLink*  link,
    cdr::String&         rule
) {
    // We allow this many rules
    // I don't use an expandable structure like vector or list because
    //   I want to find out if there is a runaway failure, compiling
    //   the same rule multiple times.
    const int maxRules = 50;

    // Static vector of parsed rule objects
    static LinkChkTargContains *s_parsedRule[maxRules];
    static int                 s_parsedCount = 0;

    // Named mutex used for synchronizing access to vector of
    // LinkChkTargContains objects
    static HANDLE s_LinkContainsMutex = 0;

    // Pointer to single parse tree we'll use
    LinkChkTargContains *ruleTree = 0;

    // Loop counter.  MSVC++ complains if this is defined in the loop
    // Non-standard MSVC extension is responsible
    int i;


    // If this is the first thread in the current process, create a mutex
    //   to control the static vector of rule parse trees
    if (s_LinkContainsMutex == 0)
        s_LinkContainsMutex = CreateMutex (0, false, LinkTargMutex);

    // Lock and search vector of parse trees to see if we can find
    //   an existing parse of the rule we want
    if (s_LinkContainsMutex != 0) {

        DWORD stat = WaitForSingleObject (s_LinkContainsMutex, 5000);

        switch (stat) {

            case WAIT_OBJECT_0:
            case WAIT_ABANDONED:
                // Have exclusive ownership of the parse tree vector
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
                    ruleTree = new LinkChkTargContains (rule);

                    // Add it to the array
                    s_parsedRule[s_parsedCount++] = ruleTree;
                }

                // Release mutex
                if (!ReleaseMutex (s_LinkContainsMutex)) {
                    throw cdr::Exception (
                        L"Can't release Link Procs mutex.  Error="
                        + cdr::String::toString (GetLastError ()));
                }

                break;

            case WAIT_TIMEOUT:
                throw cdr::Exception (
                 L"Timeout waiting for mutex for link proc parse tree object");
                break;

            default:
                throw cdr::Exception (
              L"Unknown mutex error on link proc parse trees - can't happen!");
        }
    }
    else {
        throw cdr::Exception (
            L"Can't create link proc parse trees mutex.  Error=" +
              cdr::String::toString (GetLastError ()));
    }

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

LinkChkTargContains::LinkChkTargContains (cdr::String linkRule)
{
    // Save full copy of the rule
    ruleString = linkRule;

    // Parse the rule and save the parse tree
    // If parse fails, an exception is thrown
    const char *ruleStrz = ruleString.toUtf8().c_str();
    treeTop = parseRule (&ruleStrz);
}


/**
 * Skip whitespace in a string.
 *
 * @param  stringpp     Ptr to ptr to cstring, will update to point past
 *                      whitespace.
 */

static void skipSpace (const char **stringpp) {
    while (**stringpp == ' ' || **stringpp == '\t')
        ++*stringpp;
}


/**
 * Get a tag from a Tag Rel Value string.
 *
 * @param  stringpp     Ptr to ptr to string.  Update to point past tag.
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
        for (i='A'; i<'Z'; i++)
            s_nmchars[i] = true;
        for (i='a'; i<'z'; i++)
            s_nmchars[i] = true;
        for (i='0'; i<'9'; i++)
            s_nmchars[i] = true;
        s_nmchars['.'] = true;
        s_nmchars['-'] = true;
        s_nmchars['_'] = true;
        s_nmchars['/'] = true;  // Not a tag char, but legal separator
    }

    // Initialize from passed ptr to ptr
    p = startp = *stringpp;

    // A tag must start with an alphabetic
    if (!isalpha(*p++))
        throw cdr::Exception (L"Link property validation expression string "
                              L"does not contain valid XML tag");

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

static LinkChkRelator parseRelator (const char **stringpp)
{
    // Do simple, dumb comparisons to try to match against known relators
    const char *p = *stringpp;
    if (strncmp (p, "==", 2)) {
        *stringpp += 2;
        return relEqual;
    }
    if (strncmp (p, "!=", 2)) {
        *stringpp += 2;
        return relNotEqual;
    }

    // No match where one was required
    throw cdr::Exception (L"System Error: Link check expression "
                          L"missing relational operator");
}


/**
 * Get a quote delimited value from a Tag Rel Value string.
 *
 * @param  stringpp     Ptr to ptr to string.  Update to point past value.
 *
 * @return              Relator constant.
 * @throws              CdrException if no quote delimited string found.
 */

static std::string parseValue (const char **stringpp)
{
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
 * @param  stringpp     Ptr to ptr to string.  Update to point past operator.
 *
 * @return              Operator constant.
 * @throws              CdrException if no valid operator found.
 */

static LinkChkBoolOp parseBoolOp (const char **stringpp)
{
    // Simple comparisons
    if (**stringpp == '|') {
        ++*stringpp;
        return boolOr;
    }
    if (**stringpp == '&') {
        ++*stringpp;
        return boolAnd;
    }

    // No match where one was required
    throw cdr::Exception (L"System Error: Link check expression "
                          L"missing boolean operator");
}


/**
 * Get a complete tag/relator/value expression.
 *
 * @param  stringpp     Ptr to ptr to string.  Update to point past operator.
 *
 * @return              Ptr to new relation object.
 * @throws              CdrException if syntax or other error.
 */

static LinkChkRelation *parseRelation (const char **stringpp)
{
    std::string    tag;         // Tag in format used in query_term table
    LinkChkRelator relator;     // Relationship
    std::string    value;       // Value to compare against linked doc

    // Skip initial whitespace
    skipSpace (stringpp);

    // Get the three elements of a relation, ignoring any whitespace
    tag     = parseTag (stringpp);
    skipSpace (stringpp);
    relator = parseRelator (stringpp);
    skipSpace (stringpp);
    value   = parseValue (stringpp);
    skipSpace (stringpp);

    // If we got this far without throwing an exception, we have all we need
    return new LinkChkRelation (tag, relator, value);
}


/**
 * Parse a link check validation rule, creating an executable parse tree.
 *
 * @param  stringp      Ptr to string containing the rule.
 *
 * @return              Ptr to parse tree.
 * @throws              CdrException if syntax or other error.
 */

static LinkChkPair *parseRule (const char **rulepp)
{
    // Data to put in the top of the parse tree
    LinkChkNode   *leftNode;
    LinkChkNode   *rightNode;
    LinkChkBoolOp boolConnector;


    // Recursive descent parser
    do {
        // Leading whitespace
        skipSpace (rulepp);

        // Open parens requires a descent
        if (**rulepp == '(') {
            // Go deeper to get the left node, might itself go deeper
            leftNode = parseRule (rulepp);
            if (**rulepp != ')')
                throw cdr::Exception (L"System Error: Link check expression "
                                      L"syntax error");
        }
        else
            // There should be a tag/rel/value expression here
            leftNode = parseRelation (rulepp);

        skipSpace (rulepp);

        // There might only have been one expression, or their might
        //   be more than one, connected by boolean operators
        // If only one, we're now at the end of the string
        if (**rulepp) {

            // There has to be a connector here, if not, an excecption
            boolConnector = parseBoolOp (rulepp);

            // Right node can be anything, including a parenthesized
            //   expression, a single relation, a list of relations
            //   separated by boolean operators, or whatever.
            // Doesn't matter what it is, a recursive descent should
            //   parse it correctly if it's well formed.
            rightNode = parseRule (rulepp);
        }
        // Continue until end of rule string
    } while (**rulepp);

    // If we got this far, no syntax errors
    return new LinkChkPair (leftNode, rightNode, boolConnector);
}


/*
 * Evaluate a single relation expression.
 */

bool LinkChkRelation::evalRelation (
    cdr::db::Connection& dbConn,
    int                  docId
) {
    // Check the query term table for this field + value + docId.
    std::string qry = "SELECT doc_id "
                      "  FROM query_term "
                      " WHERE doc_id = ? "
                      "   AND path = ?"
                      "   AND value = ?";

    cdr::db::PreparedStatement stmt = dbConn.prepareStatement (qry);
    stmt.setInt (1, docId);
    stmt.setString (2, tag);
    stmt.setString (3, value);
    cdr::db::ResultSet rs = stmt.executeQuery();

    // Are we looking for equality or inequality?
    // We don't actually need to examine anything, all we need to know is ...
    // Was there a hit?
    if (!rs.next()) {
        if (relator == relEqual)
            return true;
        return false;
    }

    // If got here, then no hit in query term table
    if (relator == relEqual)
        return false;
    return true;
}


/*
 * Evaluate a complete parse tree.
 */

bool LinkChkPair::evalRule (
    cdr::db::Connection& dbConn,
    int                  docId
) {
    bool result;     // Return code


    // Ultimately, at the leaves of the tree, all objects to evaluate are
    //   tag/rel/value relations.
    // If a particular node is a leaf, call evalRelation() to evaluate it.
    // Else call evalRule() recursively to process the subtree.
    if (lNode->getNodeType() == typePair)
        result = ((LinkChkPair *) lNode)->evalRule (dbConn, docId);
    else
        result = ((LinkChkRelation *) lNode)->evalRelation (dbConn, docId);

    // See if there is no right hand node, or if we can short circuit it
    if (!rNode)
        return result;
    if (connector == boolAnd && result == false)
        return result;
    if (connector == boolOr && result == true)
        return result;

    // Have to evaluate the right hand side of the node pair
    // Since we are here:
    //     connector == AND and result == true
    //   or
    //     connector == OR and result == false
    // In either case we can just return the result of the evaluation
    //   of the right hand node
    if (rNode->getNodeType() == typePair)
        return (((LinkChkPair *) rNode)->evalRule (dbConn, docId));
    else
        return (((LinkChkRelation *) rNode)->evalRelation (dbConn, docId));
}
