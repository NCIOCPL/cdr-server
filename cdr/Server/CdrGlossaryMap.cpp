/*
 * $Id: CdrGlossaryMap.cpp,v 1.3 2004-12-21 19:30:24 bkline Exp $
 *
 * Returns a document identifying which glossary terms should be used
 * for marking up phrases found in a CDR document.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2004/09/09 18:43:26  bkline
 * Added Name element for each term.
 *
 * Revision 1.1  2004/07/08 00:32:38  bkline
 * Added CdrGetGlossaryMap command; added cdr.lib to 'make clean' target.
 *
 */

#include <set>
#include <map>
#include <vector>
#include <sstream>
#include <wchar.h>

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"


static void mapPreferredTerms(cdr::StringSet& phrases, 
                              std::map<int, cdr::StringList>& mappings,
                              std::map<int, cdr::String>& names,
                              cdr::db::Connection& conn);
static void mapExternalPhrases(cdr::StringSet& phrases, 
                              std::map<int, cdr::StringList>& mappings,
                              cdr::db::Connection& conn);
static void addPhrase(cdr::StringSet& phrases,
                      std::map<int, cdr::StringList>& mappings,
                      int id,
                      const cdr::String& phrase);
static cdr::String normalizePhrase(const cdr::String& phrase);

/**
 * Returns a document identifying which glossary terms should be used
 * for marking up phrases found in a CDR document.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getGlossaryMap(cdr::Session&,
                                const cdr::dom::Node&,
                                cdr::db::Connection& connection)
{
    // Start the response document.
    std::wostringstream response;
    response << L"<CdrGetGlossaryMapResp>";

    // Remember phrases we've already taken care of.
    StringSet phrases;

    // Collect the phrases which belong to each term.
    std::map<int, StringList> mappings;
    std::map<int, String>     names;

    // Find the term's preferred terms first (these take precedence).
    mapPreferredTerms(phrases, mappings, names, connection);

    // Collect all the other phrases we can map.
    mapExternalPhrases(phrases, mappings, connection);

    // Fill out the response document.
    std::map<int, StringList>::const_iterator term = mappings.begin();
    while (term != mappings.end()) {
        String name = L"[No Term Name Found]";
        if (names.find(term->first) != names.end())
            name = names[term->first];
        response << L"<Term id='" << term->first << L"'>";
        response << L"<Name>" << cdr::entConvert(name) << L"</Name>";
        StringList::const_iterator phrase = term->second.begin();
        while (phrase != term->second.end()) {
            response << L"<Phrase>" << cdr::entConvert(*phrase)
                     << L"</Phrase>";
            ++phrase;
        }
        response << L"</Term>";
        ++term;
    }

    // Wrap it up and ship it out.
    response << L"</CdrGetGlossaryMapResp>";
    return response.str();
}

/*
 * Get the preferred names for the glossary terms.
 */
void mapPreferredTerms(cdr::StringSet& phrases, 
                       std::map<int, cdr::StringList>& mappings,
                       std::map<int, cdr::String>& names,
                       cdr::db::Connection& conn)
{
    cdr::db::Statement stmt = conn.createStatement();
    cdr::db::ResultSet rs = stmt.executeQuery(
        "SELECT n.doc_id, n.value                   "
        "  FROM query_term n                        "
        "  JOIN query_term s                        "
        "    ON s.doc_id = n.doc_id                 "
        " WHERE n.path = '/GlossaryTerm/TermName'   "
        "   AND s.path = '/GlossaryTerm/TermStatus' "
        "   AND s.value <> 'Rejected'               ");
    while (rs.next()) {
        int         id   = rs.getInt(1);
        cdr::String name = rs.getString(2);
        names[id]        = name;
        addPhrase(phrases, mappings, id, name);
    }
    stmt.close();
}

/*
 * Get the alternate phrases from the external_map table.
 */
void mapExternalPhrases(cdr::StringSet& phrases, 
                        std::map<int, cdr::StringList>& mappings,
                        cdr::db::Connection& conn)
{
    cdr::db::Statement stmt = conn.createStatement();
    cdr::db::ResultSet rs = stmt.executeQuery(
        "SELECT m.doc_id, m.value                   "
        "  FROM external_map m                      "
        "  JOIN external_map_usage u                "
        "    ON u.id   = m.usage                    "
        "  JOIN query_term s                        "
        "    ON s.doc_id = m.doc_id                 "
        " WHERE u.name = 'GlossaryTerm Phrases'     "
        "   AND s.path = '/GlossaryTerm/TermStatus' "
        "   AND s.value <> 'Rejected'               ");
    while (rs.next()) {
        int         id   = rs.getInt(1);
        cdr::String name = rs.getString(2);
        addPhrase(phrases, mappings, id, name);
    }
    stmt.close();
}

/*
 * Add a phrase to our mapping object if we haven't already handled the
 * phrase.
 */
void addPhrase(cdr::StringSet& phrases,
               std::map<int, cdr::StringList>& mappings,
               int id,
               const cdr::String& phrase)
{
    // Check to see if we have already seen this phrase.
    cdr::String normalizedPhrase = normalizePhrase(phrase);
    if (phrases.find(normalizedPhrase) != phrases.end()) 
        return;

    // Plug the phrase into the mapping table.
    if (mappings.find(id) == mappings.end())
        mappings[id] = cdr::StringList(); //stringList;
    mappings[id].push_back(normalizedPhrase);

    // Remember that we already have this phrase.
    phrases.insert(normalizedPhrase);
}

cdr::String normalizePhrase(const cdr::String& phrase)
{
    cdr::String p = phrase;
    size_t i = 0;
    bool justSawSpace = false;
    while (i < p.size()) {
        if (wcschr(L"'\".,?!:;()[]{}<>\x201C\x201D", p[i]))
            p.erase(i, 1);
        else if (wcschr(L"\n\r\t -", p[i])) {
            if (justSawSpace || i == 0 || i == p.size() - 1)
                p.erase(i, 1);
            else {
                p[i++] = ' ';
                justSawSpace = true;
            }
        }
        else {
            p[i] = towupper(p[i]);
            ++i;
            justSawSpace = false;
        }
    }
    return p;
}
