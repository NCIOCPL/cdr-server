/*
 * $Id: tCdrRegEx.cpp,v 1.1 2000-05-03 15:09:48 bkline Exp $
 *
 * Test driver for cdr::RegEx class.
 *
 * $Log: not supported by cvs2svn $
 */

#include <iostream>
#include "CdrRegEx.h"
#include "CdrException.h"

#define RE_PAT_DOC_ID       L"^CDR\\d{10}$"
#define RE_PAT_YEAR_MONTH   L"^\\d\\d\\d\\d-\\d\\d"
#define RE_PAT_DAY          L"-\\d\\d"
#define RE_PAT_DATE         RE_PAT_YEAR_MONTH RE_PAT_DAY L"$"
#define RE_PAT_TIME_PART    L"\\d\\d:\\d\\d((:\\d\\d)(\\.\\d+)?)?" \
                            L"( ?[-+]\\d{1,2}:\\d{2})?$"
#define RE_PAT_TIME         L"^" RE_PAT_TIME_PART
#define RE_PAT_TIME_INSTANT RE_PAT_YEAR_MONTH RE_PAT_DAY L"T" RE_PAT_TIME_PART
#define RE_PAT_BINARY       L"^([0-9a-fA-F]{2}|\\s)*$"
#define RE_PAT_NUMBER       L"^\\d+(\\.\\d*)?|\\d*\\.\\d+$"
#define RE_PAT_INTEGER      L"^\\d+$"
#define RE_PAT_URI          L"^http://work.this.out.later.com$"

main()
{
    struct Test {
        const wchar_t*  pattern;
        const wchar_t*  string;
        bool            expectedResult;
    };
    static Test tests[] = {
        { RE_PAT_DOC_ID,        L"CDR0014355892",                   true  }, 
        { RE_PAT_DOC_ID,        L"CDR0014355892a",                  false }, 
        { RE_PAT_DOC_ID,        L"CDR001435892",                    false }, 
        { RE_PAT_DOC_ID,        L"CDR001435x892",                   false }, 
        { RE_PAT_DATE,          L"2000-02-14",                      true  }, 
        { RE_PAT_DATE,          L"2000-2-4",                        false }, 
        { RE_PAT_TIME_INSTANT,  L"2000-02-04T13:24:18.007-05:00",   true  }, 
        { RE_PAT_BINARY,        L"DEAD1234",                        true  }, 
        { RE_PAT_NUMBER,        L"12345.67",                        true  }, 
        { RE_PAT_INTEGER,       L"12345678",                        true  }, 
        { RE_PAT_URI,           L"@@@@@@@@",                        false }, 
        { L")",                 L"SHOULD TRIGGER EXCEPTION",        false },
    };
    int nTests = 0, nFailures = 0, nExceptions = 0, nSuccesses = 0;

    cdr::RegEx re;
    for (size_t i = 0; i < sizeof tests / sizeof *tests; ++i) {
    
        ++nTests;
        try {

            std::wcout << L"        PATTERN: " 
                       << tests[i].pattern 
                       << std::endl;
            std::wcout << L"         STRING: " 
                       << tests[i].string  
                       << std::endl;
            std::wcout << L"EXPECTED RESULT: " 
                       << std::boolalpha 
                       << tests[i].expectedResult
                       << std::endl;
            re.set(tests[i].pattern);
            bool outcome = re.match(tests[i].string);
            std::wcout << L"  ACTUAL RESULT: " 
                       << std::boolalpha 
                       << tests[i].expectedResult
                       << std::endl;
            if (outcome != tests[i].expectedResult) {
                ++nFailures;
                std::wcout << L"************   FAILED TEST   *************\n";
            }
            else
                ++nSuccesses;
        }
        catch (cdr::Exception& e) {
            ++nExceptions;
            std::wcout << L"Caught exception: " << e.getString() << L"\n";
        }
    }
    std::wcout << L"\n------------------------------------------------\n\n";
    std::wcout << L"         TOTAL TESTS = " << nTests << std::endl;
    std::wcout << L"       NUMBER PASSED = " << nSuccesses << std::endl;
    std::wcout << L"       NUMBER FAILED = " << nFailures << std::endl;
    std::wcout << L"NUMBER OF EXCEPTIONS = " << nExceptions << std::endl;
    std::wcout << L" EXPECTED EXCEPTIONS = 1\n";
    if (nFailures == 0 && nExceptions == 1)
        std::wcout << L"\n***********  SUCCESS  ***********\n";
    else if (nSuccesses == 0)
        std::wcout << L"\n***********  FAILURE  ***********\n";
    else
        std::wcout << L"\n***********  PARTIAL SUCCESS  ***********\n";
    std::wcout << L"\n------------------------------------------------\n\n";
    return 0;
}
