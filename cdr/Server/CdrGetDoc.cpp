/*
 * $Id: CdrGetDoc.cpp,v 1.1 2000-05-03 15:18:12 bkline Exp $
 *
 * Stub version of internal document retrieval commands needed by other
 * modules.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrDom.h"
#include "CdrCommand.h"
#include "CdrException.h"
#include "CdrDbResultSet.h"
#include "CdrDbPreparedStatement.h"
#include "CdrGetDoc.h"
#include "CdrBlob.h"

// Local functions.
static cdr::String encodeBlob(const cdr::Blob& blob);

/**
 * Builds the XML string for a <CdrDoc> element for a document extracted from
 * the database.  I assume there will be another overloaded implementation
 * which takes a parameter specifying the version number.  This implementation
 * goes directly against the document table.
 */
cdr::String cdr::getDocString(
        const cdr::String&    docIdString,
        cdr::db::Connection&  conn)
{
    // Go get the document information.
    int docId = docIdString.extractDocId();
    std::string query = "          SELECT d.created,"
                        "                 c.name,"
                        "                 d.val_status,"
                        "                 d.val_date,"
                        "                 d.title,"
                        "                 d.modified,"
                        "                 m.name,"
                        "                 d.xml,"
                        "                 d.comment,"
                        "                 t.name,"
                        "                 b.data"
                        "            FROM document d"
                        " LEFT OUTER JOIN doc_blob b"
                        "              ON b.id = d.id"
                        "            JOIN doc_type t"
                        "              ON t.id = d.doc_type"
                        "            JOIN usr c"
                        "              ON c.id = d.creator"
                        " LEFT OUTER JOIN usr m"
                        "              ON m.id = d.modifier"
                        "           WHERE d.id = ?";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setInt(1, docId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unable to load document", docIdString);
    cdr::String     created   = rs.getString(1);
    cdr::String     creator   = rs.getString(2);
    cdr::String     valStatus = rs.getString(3);
    cdr::String     valDate   = rs.getString(4);
    cdr::String     title     = rs.getString(5);
    cdr::String     modified  = rs.getString(6);
    cdr::String     modifier  = rs.getString(7);
    cdr::String     xml       = rs.getString(8);
    cdr::String     comment   = rs.getString(9);
    cdr::String     docType   = rs.getString(10);
    cdr::Blob       blob      = rs.getBytes(11);
    select.close();

    // Just in case the caller sent an ID string not in canonical form.
    wchar_t canonicalIdString[25];
    swprintf(canonicalIdString, L"CDR%010d", docId);

    // Build the CdrDoc string.
    if (created.size() > 10)
        created[10] = L'T';
    cdr::String cdrDoc = cdr::String(L"<CdrDoc><CdrDocCtl><DocId>")
                       + canonicalIdString
                       + L"</DocId><DocType>"
                       + docType
                       + L"</DocType><DocCreated>"
                       + created
                       + L"</DocCreated><DocCreator>"
                       + creator
                       + L"</DocCreator><DocValStatus>"
                       + valStatus
                       + L"</DocValStatus>";
    if (!valDate.isNull()) {
        if (valDate.size() > 10)
            valDate[10] = L'T';
        cdrDoc += cdr::String(L"<DocValDate>") + valDate + L"</DocValDate>";
    }
    cdrDoc += cdr::String(L"<DocType>") + title + L"</DocType><DocTitle>"
           + title + L"</DocTitle>";
    if (!modified.isNull()) {
        if (modified.size() > 10)
            modified[10] = L'T';
        cdrDoc += cdr::String(L"<DocModified>") + modified + L"</DocModified>";
    }
    if (!modifier.isNull())
        cdrDoc += cdr::String(L"<DocModifier>") + modifier + L"</DocModifier>";
    if (!comment.isNull())
        cdrDoc += cdr::String(L"<Comment>") + comment + L"</Comment>";
    cdrDoc += L"</CdrDocCtl>";

    // Add the attributes and the main XML for the document.
    query = "   SELECT name,"
            "          val"
            "     FROM attr"
            "    WHERE id = ?"
            " ORDER BY id, name, num";
    cdr::db::PreparedStatement attrQuery = conn.prepareStatement(query);
    attrQuery.setInt(1, docId);
    cdr::db::ResultSet attrResultSet = attrQuery.executeQuery();

    cdr::String attrs = L"<CdrDocAttrs>";
    while (attrResultSet.next()) {
        cdr::String attrName  = attrResultSet.getString(1);
        cdr::String attrValue = attrResultSet.getString(2);
        attrs += cdr::String(L"<CdrAttr><AttrName>")
              +  attrName
              +  L"</AttrName><AttrVal>"
              +  attrValue
              + L"</AttrVal></CdrAttr>";
    }
    cdrDoc += attrs
           +  L"</CdrDocAttrs><CdrDocXml><![CDATA["
           +  xml
           +  L"]]></CdrDocXml>";

    // If there's a blob, add it, too.
    if (!blob.isNull())
        cdrDoc += cdr::String(L"<CdrDocBlob encoding='base64'>")
               +  encodeBlob(blob)
               +  L"</CdrDocBlob>";

    // Return the string to the caller.
    cdrDoc += L"</CdrDoc>";
    return cdrDoc;
}

cdr::String encodeBlob(const cdr::Blob& blob)
{
    wchar_t *buf = 0;
    static const wchar_t codes[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   L"abcdefghijklmnopqrstuvwsyz"
                                   L"0123456789+/";
    const wchar_t pad = L'=';
    try {
        size_t nBytes = blob.size();
        size_t nChars = (nBytes /  3 + 1) * 4   // encoding bloat
                      + (nBytes / 76 + 1) * 2;  // line breaks
        wchar_t* buf = new wchar_t[nChars];
        size_t i = 0;
        wchar_t* p = buf;
        size_t charsInLine = 0;
        while (i < nBytes) {
            switch (nBytes - i) {
            case 1:
                p[0] = codes[(blob[i + 0] >> 2)                    & 0x3F];
                p[1] = codes[(blob[i + 0] << 4)                    & 0x3F];
                p[2] = pad;
                p[3] = pad;
                break;
            case 2:
                p[0] = codes[(blob[i + 0] >> 2)                    & 0x3F];
                p[1] = codes[(blob[i + 0] << 4 | blob[i + 1] >> 4) & 0x3F];
                p[2] = codes[(blob[i + 1] << 2)                    & 0x3F];
                p[3] = pad;
                break;
            default:
                p[0] = codes[(blob[i + 0] >> 2)                    & 0x3F];
                p[1] = codes[(blob[i + 0] << 4 | blob[i + 1] >> 4) & 0x3F];
                p[2] = codes[(blob[i + 1] << 2 | blob[i + 2] >> 6) & 0x3F];
                p[3] = codes[(blob[i + 2])                         & 0x3F];
                break;
            }
            i += 3;
            p += 4;
            charsInLine += 4;
            if (charsInLine >= 76) {
                *p++ = L'\r';
                *p++ = L'\n';
                charsInLine = 0;
            }
        }

        // Release buffer and return as string
        cdr::String retval = cdr::String(buf, p - buf);
        delete [] buf;
        return retval;
    }
    catch (...) {
        delete [] buf;
        throw;
    }
}
