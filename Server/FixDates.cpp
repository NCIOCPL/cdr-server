/*
 * $Id$
 *
 * Example of program to zap CDR XML data.  Don't try this at home, kids!
 * This one fixes the delimiter between the date and time, changing it
 * from ' ' (as used in ANSI SQL) to 'T' (as used in ISO/XML dates).
 *
 * $Log: not supported by cvs2svn $
 */

#include <iostream>
#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"

const wchar_t*  TAG         = L"<ChangeDate>";
const size_t    TAG_LEN     = wcslen(TAG);
const size_t    DELIM_POS   = wcslen(L"YYYY-MM-DD") + TAG_LEN;

static cdr::String fixString(const cdr::String s);

int main()
{
    // Stay awake!  Catch exceptions!
    try {

        // Find the Org records.
        cdr::db::Connection conn;
        std::string q1 = "SELECT d.id,"
                         "       d.xml"
                         "  FROM document d,"
                         "       doc_type t"
                         " WHERE d.doc_type = t.id"
                         "   AND t.name = 'Org'";
        cdr::db::Statement  s1 = conn.createStatement();
        cdr::db::ResultSet  rs = s1.executeQuery(q1.c_str());

        // For each record...
        while (rs.next()) {
            int         id  = rs.getInt(1);
            cdr::String xml = rs.getString(2);
            size_t      pos = xml.find(TAG);
            bool        chg = false;

            // Examine the <ChangeDate> elements
            while (pos != xml.npos) {
                size_t  end = xml.find(L"</", pos);
                std::wcerr << L"RECORD=" << id << std::endl;
                std::wcerr << L"POS=" << pos << std::endl;
                std::wcerr << L"DATE=" 
                           << xml.substr(pos + TAG_LEN, end - (pos + TAG_LEN))
                           << std::endl;
                std::wcerr << L"DELIM='" << xml[pos + DELIM_POS] << L"'\n";

                // Change space delimiter to T.
                if (xml[pos + DELIM_POS] == L' ') {
                    xml[pos + DELIM_POS] = L'T';
                    std::wcerr << L"CHANGED ' ' TO 'T'\n";
                    chg = true;
                }
                pos = xml.find(TAG, pos + TAG_LEN);
            }

            // If we fixed any, update the row in the database.
            if (chg) {
                wchar_t idString[20];
                swprintf(idString, L"%d", id);
                std::wcout << L"UPDATE document SET xml = '"
                           << fixString(xml)
                           << L"' WHERE id = "
                           << idString
                           << L"\ngo\n";
            }
        }
        s1.close();
    }
    catch (cdr::Exception e) {
        std::wcerr << L"BAD NEWS: " << e.what() << std::endl;
    }
    return 0;
}

cdr::String fixString(const cdr::String s)
{
    cdr::String fixedString = s;
    size_t pos = fixedString.find(L'\'');
    while (pos != fixedString.npos) {
        fixedString.replace(pos, 1, L"''");
        pos = fixedString.find(L'\'', pos + 2);
    }
    return fixedString;
}
