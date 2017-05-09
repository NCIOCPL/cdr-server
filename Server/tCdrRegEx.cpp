/*
 * Test driver for cdr::RegEx class.
 *
 * Added an extra test 2013-05-29 to verify that if we use non-capturing
 * groups we can have constructed expressions with very large numbers of
 * OR-ed groups.
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
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
#define RE_PAT_MULTI_OR     L"abc|def|mno.*"

#define NPHRASES 10000
#define PHRASE_LEN 32

short Glbl_DEFAULT_CDR_PORT  = 2019;
short Glbl_CurrentServerPort = Glbl_DEFAULT_CDR_PORT;

int main() {

    wchar_t phrase[PHRASE_LEN + 1], target[PHRASE_LEN + 1];
    static wchar_t expression[NPHRASES * (PHRASE_LEN + 5)];
    struct Test {
        const wchar_t*  pattern;
        const wchar_t*  string;
        bool            expectedResult;
    };
    static Test tests[] = {
        { expression,           target,                             true  },
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
        { RE_PAT_MULTI_OR,      L"abc",                             true  },
        { RE_PAT_MULTI_OR,      L"def",                             true  },
        { RE_PAT_MULTI_OR,      L"mno",                             true  },
        { RE_PAT_MULTI_OR,      L"mnoand_more",                     true  },
        { RE_PAT_MULTI_OR,      L"noand_more",                      false },
        { RE_PAT_MULTI_OR,      L"abcd",                            false },
        { RE_PAT_MULTI_OR,      L"cdef",                            false },
        { RE_PAT_MULTI_OR,      L"abc|def|mno.*",                   false },
        { L")",                 L"SHOULD TRIGGER EXCEPTION",        false },
    };
    wchar_t* e = expression;
    srand((unsigned int)time(NULL));
    wchar_t* chars = L"0123456789"
                     L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                     L"abcdefghijklmnopqrstubwsyz ";
    size_t nchars = wcslen(chars);
    for (size_t n = 0; n < NPHRASES; ++n) {
        if (n > 0)
            *e++ = L'|';
        *e++ = L'(';
        *e++ = L'?';
        *e++ = L':';
        wchar_t* p = phrase;
        for (size_t i = 0; i < PHRASE_LEN; ++i) {
            wchar_t c = chars[rand() % nchars];
            *e++ = *p++ = c;
        }
        *p++ = L'\0';
        *e++ = L')';
        if (n == 500)
            wcscpy_s(target, PHRASE_LEN + 1, phrase);
    }
    *e++ = L'\0';
    int nTests = 0, nFailures = 0, nExceptions = 0, nSuccesses = 0;

    cdr::RegEx re;
    for (size_t i = 0; i < sizeof tests / sizeof *tests; ++i) {

        ++nTests;
        try {

            if (i > 0)
                std::wcout << L"--------PATTERN: "
                           << tests[i].pattern
                           << std::endl;
            else
                std::cout << "--------PATTERN: [constructed "
                          << wcslen(expression)
                          << "-character expression]"
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
                       << outcome
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
            std::wcout << L"Caught exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::wcout << L"Caught non-Cdr exception." << std::endl;
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
    std::wcout << L"\n------------------------------------------------\n"
               << std::endl;
    return 0;
}
