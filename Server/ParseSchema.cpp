/*
 * Prototype for CDR schema parser.  See ../Validation/Makefile.
 */

// System headers.
#include <iostream>
#include <fstream>
#include <cstdlib>

// Project headers.
#include "CdrString.h"
#include "CdrException.h"
#include "CdrXsd.h"

// Local support functions.
static std::string readSchemaFile(const char*);

// Don't really need these, except for link dependencies.
short Glbl_DEFAULT_CDR_PORT = 2019;
short Glbl_CurrentServerPort = Glbl_DEFAULT_CDR_PORT;

/**
 * Parses schema document, extracts document type information, and
 * writes equivalent DTD to standard output in UTF-8.  Reads schema
 * from file named on command line.
 */
int main(int ac, char** av)
{
    if (ac < 1) {
        std::cerr << "usage: ParseSchema schema-file\n";
        return EXIT_FAILURE;
    }

    try {
        const char* name = av[1];
        std::string xmlString = readSchemaFile(name);
        cdr::dom::Parser parser;
        parser.parse(xmlString);
        cdr::dom::Document document = parser.getDocument();
        cdr::dom::Element docElement = document.getDocumentElement();
        cdr::xsd::Schema schema(docElement);
        std::cout << schema.makeDtd(name).toUtf8();
    }

    catch (const cdr::Exception& cdrEx) {
        std::wcerr << L"CdrException: " << cdrEx.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const cdr::dom::XMLException& ex) {
        std::wcerr << cdr::String(ex.getMessage()) << std::endl;
        return EXIT_FAILURE;
    }
    catch (cdr::dom::DOMException* de) {
        std::wcerr << L"DOM Exception: " << de->code << cdr::String(L": ")
                   << cdr::String(de->msg)
                   << std::endl;
        delete de;
        return EXIT_FAILURE;
    }
    catch (const cdr::dom::SAXParseException& spe) {
        std::wcerr << L"SAX Parse Exception: " << spe.getMessage()
                   << L" at line " << spe.getLineNumber()
                   << L", column " << spe.getColumnNumber()
                   << std::endl;
        return EXIT_FAILURE;
    }
    catch (const cdr::dom::SAXException& se) {
        std::wcerr << cdr::String(se.getMessage()) << std::endl;
        return EXIT_FAILURE;
    }

    catch (const std::exception& e) {
        std::cerr << L"Exception caught: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...) {
        std::wcerr << L"Unexpected exception caught\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Loads the contents of the specified file into a string object.
 */
std::string readSchemaFile(const char* name)
{
    std::ifstream is(name, std::ios::binary);
    if (!is)
        throw cdr::Exception(L"Unable to open schema file");
    is.seekg(0, std::ios::end);
    size_t n = (size_t)is.tellg();
    is.seekg(0, std::ios::beg);
    char* p = new char[n];
    is.read(p, n);
    std::string s(p, n);
    delete [] p;
    return s;
}
