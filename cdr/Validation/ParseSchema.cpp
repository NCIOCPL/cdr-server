/*
 * $Id: ParseSchema.cpp,v 1.2 2000-04-11 21:24:09 bkline Exp $
 *
 * Prototype for CDR schema parser.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/11 17:57:44  bkline
 * Initial revision
 *
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
static std::string  readSchemaFile(const char*);
static void         writeDtd(const cdr::xsd::Schema&);
static void         writeElement(const cdr::xsd::Schema&, cdr::xsd::Element&);

/**
 * Parses schema document, extracts document type information, and
 * writes equivalent DTD to standard output in UTF-8.  Reads schema
 * from file named on command line (default is ProtocolSchema.xml).
 */
int main(int ac, char** av)
{
    try {
        const char* name = ac > 1 ? av[1] : "ProtocolSchema.xml";
        std::string xmlString = readSchemaFile(name);
        cdr::dom::Parser parser;
        parser.parse(xmlString);
        cdr::dom::Document document = parser.getDocument();
        cdr::dom::Element docElement = document.getDocumentElement();
        cdr::xsd::Schema schema(docElement);
        writeDtd(schema);
    }

    catch (const cdr::Exception& cdrEx) {
        std::wcerr << L"CdrException: " << cdrEx.getString() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const cdr::dom::DOMException& ex) {
        std::wcerr << cdr::String(ex.getMessage()) << std::endl;
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
    is.seekg(0, std::ios::end);
    size_t n = is.tellg();
    is.seekg(0, std::ios::beg);
    char* p = new char[n];
    is.read(p, n);
    std::string s(p, n);
    delete [] p;
    return s;
}

/**
 * Writes the DTD equivalent of the schema to standard output.
 */
void writeDtd(const cdr::xsd::Schema& schema)
{
    std::cout << "<?xml version='1.0' encoding='UTF-8'?>\n";
    writeElement(schema, schema.getTopElement());
}

/**
 * Recursively writes an element description for the DTD to standard
 * output.  Element object cannot be const because it needs to lookup
 * and store its type information.
 */
void writeElement(const cdr::xsd::Schema& schema, cdr::xsd::Element& elem)
{
    // Find out what type we are (from the type name).
    const cdr::xsd::Type *type = elem.getType(schema);
    if (!type)
        throw cdr::Exception(L"Missing type for element", elem.getName());

    // Start the element declaration.
    std::cout << "<!ELEMENT " << elem.getName().toUtf8() << " ";

    // Use RTTI to determine whether this is a simple or complex type.
    const cdr::xsd::SimpleType *sType = 
        dynamic_cast<const cdr::xsd::SimpleType*>(type);
    const cdr::xsd::ComplexType *cType = 
        dynamic_cast<const cdr::xsd::ComplexType*>(type);
    if (sType)
        std::cout << "(#PCDATA)>\n";
    else {

        // Handle the content.
        int eCounter = 0;
        cdr::xsd::ElemEnum elems = cType->getElements();
        switch (cType->getContentType()) {
        case cdr::xsd::ComplexType::EMPTY:
            if (cType->getElemCount() != 0)
                throw 
                    cdr::Exception(L"Elements not allowed for EMPTY content");
            std::cout << "EMPTY>\n";
            break;
        case cdr::xsd::ComplexType::TEXT_ONLY:
            if (cType->getElemCount() != 0)
                throw cdr::Exception(L"Elements not allowed for TEXT_ONLY "
                                     L"content");
            std::cout << "(#PCDATA)>\n";
            break;
        case cdr::xsd::ComplexType::MIXED:
            std::cout << "(#PCDATA";
            while (elems != cType->getElemEnd()) {
                cdr::xsd::Element* e = *elems;
                std::cout << " | " << e->getName().toUtf8();
                ++elems;
                ++eCounter;
            }
            if (eCounter > 0)
                std::cout << ")*>\n";
            else
                std::cout << ")>\n";
            break;
        case cdr::xsd::ComplexType::ELEMENT_ONLY:
            if (cType->getElemCount() < 1)
                throw cdr::Exception(L"Elements required for ELEMENT_ONLY "
                                     L"content");
            while (elems != cType->getElemEnd()) {
                cdr::xsd::Element* e = *elems;
                int minOccs = e->getMinOccs();
                int maxOccs = e->getMaxOccs();
                std::cout << (eCounter ? ", " : "(") << e->getName().toUtf8();
                char *reps = "";
                if (minOccs < 1)
                    reps = maxOccs == 1 ? "?" : "*";
                else if (maxOccs > 1)
                    reps = "+";
                if (*reps)
                    std::cout << reps;
                ++elems;
                ++eCounter;
            }
            std::cout << ")>\n";
            break;
        }

        // Handle the attributes, if any.
        if (cType->getAttrCount() > 0) {
            std::cout << "<!ATTLIST " << elem.getName().toUtf8();
            cdr::xsd::AttrEnum attrs = cType->getAttributes();
            while (attrs != cType->getAttrEnd()) {
                cdr::xsd::Attribute* a = *attrs++;
                std::cout << ' ' << a->getName().toUtf8() << ' ';
                const cdr::xsd::SimpleType* aType = 
                    dynamic_cast<const cdr::xsd::SimpleType*>
                    (a->getType(schema));
                if (!aType)
                    throw(L"Missing type for attribute", a->getName());
                const cdr::StringSet& enums = aType->getEnumSet();
                if (enums.size() > 0) {
                    std::wcerr << L"enums.size(): " << enums.size() << std::endl;
                    char sep = '(';
                    cdr::StringSet::const_iterator i = enums.begin();
                    while (i != enums.end()) {
                        std::wcerr << L"enumerated value: " << *i << std::endl;
                        std::cout << sep << (*i++).toUtf8();
                        sep = '|';
                    }
                    std::cout << ')';
                }
                else
                    std::cout << "CDATA";
                if (a->isOptional())
                    std::cout << " #IMPLIED";
                else
                    std::cout << " #REQUIRED";
            }
            std::cout << ">\n";
        }

        // Declare any child elements which have not already been declared.
        static cdr::StringSet declaredElems;
        elems = cType->getElements();
        while (elems != cType->getElemEnd()) {
            cdr::xsd::Element* e = *elems++;
            if (declaredElems.find(e->getName()) == declaredElems.end()) {
                declaredElems.insert(e->getName());
                writeElement(schema, *e);
            }
        }
    }
}
