#include <cstring>
#include <iostream>
#include <ctime>
#include "CdrString.h"

main()
{
    wchar_t chars[0xFFFD];
    for (size_t i = 0; i < 0xFFFD; ++i)
        chars[i] = i + 1;
    bool passed;
    time_t now = time(0);
    for (int j = 0; j < 1000; ++j) {
        cdr::String cdrString(chars, 0xFFFD);
        std::string s = cdrString.toUtf8();
        cdr::String newString(s);
        if (memcmp(cdrString.c_str(), newString.c_str(), 0xFFFD << 1))
            passed = false;
        else
            passed = true;
    }
    time_t delta = time(0) - now;
    if (passed)
        std::cout << "PASSED\n";
    else
        std::cout << "FAILED\n";
    std::cout << "Elapsed time: " << delta << " seconds\n";
    return 0;
}
