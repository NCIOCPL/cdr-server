#include <iostream>
#include "CdrGetDoc.h"

main(int ac, char** av)
{
    try {
        cdr::db::Connection conn;
        int id = ac < 2 ? 1 : atoi(av[1]);
        wchar_t docId[32];
        swprintf(docId, L"CDR%010d", id);
        std::wcout << cdr::getDocString(docId, conn) << L"\n";
    }
    catch (cdr::Exception e) {
        std::wcerr << L"*** BAD NEWS: " << e.what() << L"\n";
    }
    return 0;
}
