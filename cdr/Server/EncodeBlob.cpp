/*
 * $Id $
 *
 * Standalone blob encoder used for testing cdr::Blob::encode().
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
        std::cerr << "usage: encodeBlob input-filename output-filename\n";
        return 1;
    }
    FILE* fp = fopen(av[1], "rb");
    if (!fp) {
        perror(av[1]);
        return 1;
    }
    fseek(fp, 0L, SEEK_END);
    size_t len = ftell(fp);
    unsigned char* buf = (unsigned char*)malloc(len);
    rewind(fp);
    fread(buf, 1, len, fp);
    fclose(fp);
    cdr::Blob blob(buf, len);
    std::string encoded = blob.encode().toUtf8();
    fp = fopen(av[2], "wb");
    if (!fp) {
        perror(av[2]);
        return 1;
    }
    fwrite(encoded.data(), 1, encoded.size(), fp);
    return 0;
}
