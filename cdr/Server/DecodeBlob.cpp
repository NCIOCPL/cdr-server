/*
 * $Id $
 *
 * Standalone blob decoder used for testing new cdr::Blob constructor.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrBlob.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

main(int ac, char** av)
{
    if (ac != 3) {
        std::cerr << "usage: decodeBlob input-filename output-filename\n";
        return 1;
    }
    FILE* fp = fopen(av[1], "rb");
    if (!fp) {
        perror(av[1]);
        return 1;
    }
    fseek(fp, 0L, SEEK_END);
    size_t len = ftell(fp);
    rewind(fp);
    char* buf = (char*)calloc(1, len + 1);
    fread(buf, 1, len, fp);
    fclose(fp);
    cdr::String encodedString(buf);
    cdr::Blob blob(encodedString);
    fp = fopen(av[2], "wb");
    if (!fp) {
        perror(av[2]);
        return 1;
    }
    fwrite(blob.data(), 1, blob.size(), fp);
    return 0;
}
