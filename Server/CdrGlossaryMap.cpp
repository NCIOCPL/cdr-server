/*
 * $Id: CdrGlossaryMap.cpp,v 1.5 2009-01-22 20:49:57 bkline Exp $
 *
 * Returns a document identifying which glossary terms should be used
 * for marking up phrases found in a CDR document.
 *
 * BZIssue::4704
 */

#include <set>
#include <map>
#include <vector>
#include <sstream>
#include <wchar.h>
#include <iostream>

#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"

static cdr::String PATH_EN = L"/GlossaryTermName/TermName/TermNameString";
static cdr::String PATH_ES = L"/GlossaryTermName/TranslatedName/TermNameString";

static void mapPreferredTerms(cdr::StringSet& phrases, 
                              std::map<int, cdr::StringList>& mappings,
                              std::map<int, cdr::String>& preferredNames,
                              std::set<int>& nonRejectedTerms,
                              cdr::db::Connection& conn,
                              const cdr::String& path);
static void mapExternalPhrases(cdr::StringSet& phrases, 
                               std::map<int, cdr::StringList>& mappings,
                               const std::set<int>& nonRejectedTerms,
                               cdr::db::Connection& conn,
                               const cdr::String& usage);
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
                                cdr::db::Connection& conn)
{
    // Start the response document.
    std::wostringstream response;
    response << L"<CdrGetGlossaryMapResp>";

    // Keep track of phrases we've already taken care of.
    StringSet phrases;

    // Collect the phrases which belong to each term.
    std::map<int, StringList> mappings;
    std::map<int, String>     names;

    // Find the term's preferred terms first (these take precedence).
    std::set<int> nonRejectedTerms;
    String path = PATH_EN;
    mapPreferredTerms(phrases, mappings, names, nonRejectedTerms, conn, path);

    // Collect all the other phrases we can map.
    String usage = L"GlossaryTerm Phrases";
    mapExternalPhrases(phrases, mappings, nonRejectedTerms, conn, usage);

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

/**
 * Returns a document identifying which glossary terms should be used
 * for marking up Spanish phrases found in a CDR document.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::getSpanishGlossaryMap(cdr::Session&,
                                       const cdr::dom::Node&,
                                       cdr::db::Connection& conn)
{
    // Start the response document.
    std::wostringstream response;
    response << L"<CdrGetSpanishGlossaryMapResp>";

    // Keep track of phrases we've already taken care of.
    StringSet phrases;

    // Collect the phrases which belong to each term.
    std::map<int, StringList> mappings;
    std::map<int, String>     names;

    // Find the term's preferred terms first (these take precedence).
    std::set<int> nonRejectedTerms;
    String path = PATH_ES;
    mapPreferredTerms(phrases, mappings, names, nonRejectedTerms, conn, path);

    // Collect all the other phrases we can map.
    String usage = L"Spanish GlossaryTerm Phrases";
    mapExternalPhrases(phrases, mappings, nonRejectedTerms, conn, usage);

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
    response << L"</CdrGetSpanishGlossaryMapResp>";
    return response.str();
}

/*
 * Get the preferred names for the glossary terms using the new doc type.
 */
void mapPreferredTerms(cdr::StringSet& phrases, 
                       std::map<int, cdr::StringList>& mappings,
                       std::map<int, cdr::String>& preferredNames,
                       std::set<int>& nonRejectedTerms,
                       cdr::db::Connection& conn,
                       const cdr::String& path)
{
    std::string query = 
        "         SELECT n.doc_id, n.value                           "
        "           FROM query_term n                                "
        "           JOIN query_term s                                "
        "             ON s.doc_id = n.doc_id                         ";
    if (path == PATH_EN)
        query += 
        "LEFT OUTER JOIN query_term e                                "
        "             ON e.doc_id = n.doc_id                         "
        "            AND e.path = '/GlossaryTermName/TermName'       "
        "                       + '/@ExcludeFromGlossifier'          ";
    else
        query +=
        "LEFT OUTER JOIN query_term e                                "
        "             ON e.doc_id = n.doc_id                         "
        "            AND e.path = '/GlossaryTermName/TranslatedName' "
        "                       + '/@ExcludeFromGlossifier'          "
        "            AND LEFT(n.node_loc, 4) = LEFT(e.node_loc, 4)   ";
    query +=
        "          WHERE n.path = ?                                  "
        "            AND s.path = '/GlossaryTermName/TermNameStatus' "
        "            AND s.value <> 'Rejected'                       "
        "            AND (e.value IS NULL OR e.value <> 'Yes')       ";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(query);
    stmt.setString (1, path);
    cdr::db::ResultSet rs = stmt.executeQuery();
    while (rs.next()) {
        int         id     = rs.getInt(1);
        cdr::String name   = rs.getString(2);
        preferredNames[id] = name;
        addPhrase(phrases, mappings, id, name);
        nonRejectedTerms.insert(id);
    }
    stmt.close();
}

/*
 * Get the alternate phrases from the external_map table.
 */
void mapExternalPhrases(cdr::StringSet& phrases, 
                        std::map<int, cdr::StringList>& mappings,
                        const std::set<int>& nonRejectedTerms,
                        cdr::db::Connection& conn,
                        const cdr::String& usage)
{
    std::string query = 
        "SELECT m.doc_id, m.value    "
        "  FROM external_map m       "
        "  JOIN external_map_usage u "
        "    ON u.id   = m.usage     "
        " WHERE u.name = ?           ";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(query);
    stmt.setString (1, usage);
    cdr::db::ResultSet rs = stmt.executeQuery();
    while (rs.next()) {
        int         id   = rs.getInt(1);
        cdr::String name = rs.getString(2);
        if (nonRejectedTerms.find(id) != nonRejectedTerms.end())
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
        mappings[id] = cdr::StringList();
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
        if (wcschr(L"'\".,?!:;()[]{}<>\x201C\x201D\x00A1\x00BF", p[i]))
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
