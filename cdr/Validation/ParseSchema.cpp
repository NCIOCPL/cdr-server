/*
 * $Id: ParseSchema.cpp,v 1.7 2001-04-16 18:02:29 bkline Exp $
 *
 * Prototype for CDR schema parser.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2001/04/15 13:00:43  bkline
 * Upgraded to match XML Schema 2001 release candidate.
 *
 * Revision 1.5  2001/03/21 02:47:57  bkline
 * Support for more schema features.  Better support for mixed content.
 * Moved NMTOKEN check to library code.
 *
 * Revision 1.4  2001/01/02 22:56:54  bkline
 * Fixed handling of mixed content in DTD; added NMTOKEN support.
 *
 * Revision 1.3  2000/12/21 22:25:28  bkline
 * Enhanced exception handling.
 *
 * Revision 1.2  2000/04/11 21:24:09  bkline
 * Used type information for attributes.
 *
 * Revision 1.1  2000/04/11 17:57:44  bkline
 * Initial revision
 *
 */

// System headers.
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

// Project headers.
#include "CdrString.h"
#include "CdrException.h"
#include "CdrXsd.h"

// Local support functions.
static std::string        readSchemaFile(const char*);
static bool               areNMTokens(const cdr::StringSet&);
static cdr::String        makeDtd(const cdr::xsd::Schema&, const cdr::String&);
static const wchar_t* const  
                          getCountCharacter(const cdr::xsd::CountConstrained&);
static std::wostream&     operator<<(std::wostream&, const cdr::xsd::Node*);
static void               outputMixedContent(const cdr::xsd::Node*, 
                                             cdr::StringSet&,
                                             std::wostream&);
static void               writeElement(const cdr::xsd::Schema&, 
                                       cdr::xsd::Element&,
                                       std::wostream&,
                                       bool = false);
static void               declareChildElements(const cdr::xsd::Schema&,
                                               cdr::StringSet&,
                                               const cdr::xsd::Node*,
                                               std::wostream&);

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
        std::cout << makeDtd(schema, name).toUtf8();
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
cdr::String makeDtd(const cdr::xsd::Schema& schema, const cdr::String& name)
{
    if (!&schema.getTopElement())
        throw cdr::Exception(L"No top element declared");
    time_t now = time(0);
    cdr::String when = asctime(localtime(&now));
    std::wostringstream os;
    os << L"<!--\n\n     Machine generated " << when
       << L"     From XML Schema " << name 
       << L"\n\n  -->\n\n";
    writeElement(schema, schema.getTopElement(), os, true);
    return os.str();
}

/**
 * Recursively writes an element description for the DTD to standard
 * output.  Element object cannot be const because it needs to lookup
 * and store its type information.
 */
void writeElement(const cdr::xsd::Schema& schema, cdr::xsd::Element& elem,
                  std::wostream& os, bool isTopElement)
{
    // Find out what type we are (from the type name).
    const cdr::xsd::Type *type = elem.getType(schema);
    if (!type)
        throw cdr::Exception(L"Missing type for element", elem.getName());

    // Start the element declaration.
    os << L"<!ELEMENT " << elem.getName() << L" ";

    // Use RTTI to determine whether this is a simple or complex type.
    const cdr::xsd::SimpleType *sType = 
        dynamic_cast<const cdr::xsd::SimpleType*>(type);
    const cdr::xsd::ComplexType *cType = 
        dynamic_cast<const cdr::xsd::ComplexType*>(type);
    if (sType)
        os << L"(#PCDATA)>\n";
    else {

        // Sanity check.
        if (!cType)
            throw cdr::Exception(L"Internal error",
                                 L"type must be simple or complex");
                            
        // Handle the content.
        const cdr::xsd::Node* elements = cType->getContent();
        switch (cType->getContentType()) {

        case cdr::xsd::ComplexType::EMPTY:
            if (elements)
                throw cdr::Exception(L"Elements not allowed for "
                                     L"empty content");
            os << L"EMPTY>\n";
            break;

        case cdr::xsd::ComplexType::TEXT_ONLY:

            os << L"(#PCDATA)>\n";
            break;

        case cdr::xsd::ComplexType::MIXED: {

            /*
             * From http://www.w3.org/TR/xmlschema-0/#mixedContent:
             * "Note that the mixed model in XML Schema differs fundamentally
             * from the mixed model in XML 1.0. Under the XML Schema mixed
             * model, the order and number of child elements appearing in an
             * instance must agree with the order and number of child elements
             * specified in the model. In contrast, under the XML 1.0 mixed
             * model, the order and number of child elements appearing in an
             * instance cannot be constrained. In sum, XML Schema provides
             * full schema validation of mixed models in contrast to the
             * partial schema validation provided by XML 1.0."
             */

            // For keeping track of elements which have been declared.
            cdr::StringSet mixedElements;

            os << L"(#PCDATA";
            outputMixedContent(elements, mixedElements, os);
            if (mixedElements.empty())
                throw cdr::Exception(L"Element with mixed content has no "
                                     L"child elements",  elem.getName());
            os << L")*>\n";
            break;
        }

        case cdr::xsd::ComplexType::ELEMENT_ONLY: {

            // Create a working string for the content.
            std::wostringstream contentStream;

            // Add the CdrDocCtl element for the benefit of XMetaL.
            if (isTopElement)
                contentStream << L"(CdrDocCtl,";

            // Recursively add the elements for the content model.
            contentStream << elements;

            // Close the sequence for the top element.
            if (isTopElement)
                contentStream << L")";

            // Extract the working string.
            cdr::String s = contentStream.str();

            // Sanity check.
            if (s.empty() || s == L"(CdrDocCtl,)")
                throw cdr::Exception(L"Elements required for elementOnly "
                                     L"content");

            // Add parens if needed.
            if (s[0] != L'(')
                os << L"(";
            os << s;
            if (s[0] != '(')
                os << L")";

            // Finish off the line.
            os << L">\n";
            break;
        }

        default:

            // Sanity check.
            throw cdr::Exception(L"Internal error",
                                 L"Unrecognized content model");
            break;
        }

        // Handle the attributes, if any.
        if (cType->getAttrCount() > 0) {
            os << L"<!ATTLIST " << elem.getName();
            cdr::xsd::AttrEnum attrs = cType->getAttributes();
            while (attrs != cType->getAttrEnd()) {
                cdr::xsd::Attribute* a = (attrs++)->second;
                os << L" " << a->getName() << L" ";
                const cdr::xsd::SimpleType* aType = 
                    dynamic_cast<const cdr::xsd::SimpleType*>
                    (a->getType(schema));
                if (!aType)
                    throw cdr::Exception(L"Missing type for attribute", 
                                         a->getName());

                // DTD's can't handle enumerated values which aren't NMTOKENS.
                const cdr::StringSet& enums = aType->getEnumSet();
                if (!enums.empty() && areNMTokens(enums)) {
                    const wchar_t* sep = L"(";
                    cdr::StringSet::const_iterator i = enums.begin();
                    while (i != enums.end()) {
                        os << sep << (*i++);
                        sep = L"|";
                    }
                    os << L")";
                }
                else
                    os << a->getDtdType();
                if (a->isOptional())
                    os << L" #IMPLIED";
                else
                    os << L" #REQUIRED";
            }
            os << L">\n";
        }

        // Declare any child elements which have not already been declared.
        static cdr::StringSet declaredElems;
        if (isTopElement) {
            os << L"<!ELEMENT CdrDocCtl (DocId, DocTitle)>\n";
            os << L"<!ELEMENT DocId (#PCDATA)>\n";
            os << L"<!ELEMENT DocTitle (#PCDATA)>\n";
            declaredElems.clear();
            declaredElems.insert(elem.getName());
            declaredElems.insert(L"CdrDocCtl");
            declaredElems.insert(L"DocId");
            declaredElems.insert(L"DocTitle");
        }
        declareChildElements(schema, declaredElems, elements, os);
    }
}

/**
 * Recursively write each element which can appear for mixed content type.
 *
 *  @param  node        can be element, group, choice, or sequence.
 *  @param  elements    set of elements which have already been written
 *                      for this type (so we don't repeat them).
 */
void outputMixedContent(const cdr::xsd::Node* node, cdr::StringSet& elements,
                        std::wostream& os)
{
    // Safety measure.
    if (!node)
        throw cdr::Exception(L"Internal error",
                             L"NULL node encountered for mixed content tree");

    // Is this an element node?
    const cdr::xsd::Element* e = dynamic_cast<const cdr::xsd::Element*>(node);
    if (e) {
        cdr::String name = e->getName();

        // Sanity check.
        if (name.empty())
            throw cdr::Exception(L"Missing name for element in mixed content");

        // Only output this element if we haven't already done so.
        if (elements.find(name) == elements.end()) {
            os << L"|" << name;
            elements.insert(name);
        }
        return;
    }

    // Is this node a named group?
    const cdr::xsd::Group* g = dynamic_cast<const cdr::xsd::Group*>(node);
    if (g) {
        outputMixedContent(g->getContent(), elements, os);
        return;
    }

    // Is this node a choice or sequence?
    const cdr::xsd::ChoiceOrSequence* cs =
        dynamic_cast<const cdr::xsd::ChoiceOrSequence*>(node);
    if (cs) {
        cdr::xsd::NodeEnum ne = cs->getNodes();
        while (ne != cs->getListEnd())
            outputMixedContent(*ne++, elements, os);
        return;
    }

    // Sanity check.  Should never reach this point.
    throw cdr::Exception(L"Internal error",
            L"Mixed content must be element, group, choice, or sequence");
}

/**
 * Output the element groups for elementOnly content.
 *
 *  @param  os          reference to output stream for element groups.
 *  @param  n           element, sequence, choice, or named group.
 *  @return             reference to output stream.
 */
std::wostream& operator<<(std::wostream& os, const cdr::xsd::Node* n)
{
    // Safety measure.
    if (!n)
        throw cdr::Exception(L"Internal error",
                             L"NULL node encountered for elementOnly content");

    // Is this node a named group.
    const cdr::xsd::Group* g = dynamic_cast<const cdr::xsd::Group*>(n);
    if (g) {
        os << g->getContent();
        return os;
    }

    // Is this an element?
    const cdr::xsd::Element* e = dynamic_cast<const cdr::xsd::Element*>(n);
    if (e) {
        cdr::String name = e->getName();

        // Sanity check.
        if (name.empty())
            throw cdr::Exception(L"Unnamed element object encountered");

        os << name << getCountCharacter(*e);
        return os;
    }

    // Is this a sequence or a choice?
    const cdr::xsd::Sequence* s = dynamic_cast<const cdr::xsd::Sequence*>(n);
    const cdr::xsd::Choice*   c = dynamic_cast<const cdr::xsd::Choice*>(n);
    if (s || c) {

        // Use a single variable to represent the choice or sequence.
        const cdr::xsd::ChoiceOrSequence* cs;
        if (s)
            cs = s;
        else
            cs = c;

        // Make sure the sequence or choice is not empty.
        if (cs->getNodeCount() < 1)
            throw cdr::Exception(L"Empty sequence or choice for "
                                 L"elementOnly content");

        // Add parentheses around multi-node list.
        if (cs->getNodeCount() > 1)
            os << L"(";

        // Walk through the nodes in the sequence or choice.
        cdr::xsd::NodeEnum e = cs->getNodes();
        const wchar_t* sep = L"";
        while (e != cs->getListEnd()) {
            os << sep << *e++;
            sep = s ? L"," : L"|";
        }

        // Add parentheses around multi-node list.
        if (cs->getNodeCount() > 1)
            os << L")";
        return os << getCountCharacter(*cs);
    }

    // Sanity check.  Should never reach this point.
    throw cdr::Exception(L"Internal error",
            L"Content must be element, group, choice, or sequence");
    return os;
}

/**
 * Determine what character, if any, should be used to indicate the number of
 * repetitions allowed for a count-constrained node.  Note that the DTD cannot
 * really represent the precision of which XML Schema is capable of
 * representing.  For example, if a schema indicates that an element or group
 * can occur between two and four times, the DTD will use "+" to indicate that
 * one or more occurrences is allowed.  No character is used when exactly one
 * occurrence is required.
 *
 *  @return             "*" if zero or more repetitions are allowed;
 *                      "?" if zero or one repetition(s) are allowed;
 *                      "+" if one or more repetitions are allowed;
 *                      otherwise "".
 */
const wchar_t* const getCountCharacter(const cdr::xsd::CountConstrained& ccNode)
{
    if (ccNode.getMinOccs() < 1)
        return ccNode.getMaxOccs() == 1 ? L"?" : L"*";
    else if (ccNode.getMaxOccs() > 1)
        return L"+";
    return L"";
}

/**
 * Recursively declare any child elements of the current node which have not
 * already been declared.
 *
 *  @param  schema              reference to schema currently being processed.
 *  @param  declaredElems       reference to set of elements which have
 *                              already been declared.
 *  @param  n                   address of current node; can be an element,
 *                              a sequence, a choice, or a named group.
 */
void declareChildElements(const cdr::xsd::Schema& schema,
                          cdr::StringSet& declaredElems, 
                          const cdr::xsd::Node* n,
                          std::wostream& os)
{
    // Safety check.
    if (!n)
        return;

    // Is this an element?
    const cdr::xsd::Element* e = dynamic_cast<const cdr::xsd::Element*>(n);
    if (e) {

        // See if we have already declared this element.
        cdr::String name = e->getName();
        if (declaredElems.find(name) != declaredElems.end())
            return;
        declaredElems.insert(name);

        // Const cast is required because elements need to resolve their type
        // name to their type when first used.
        writeElement(schema, const_cast<cdr::xsd::Element&>(*e), os);
        return;
    }

    // Is this a named group?
    const cdr::xsd::Group* g = dynamic_cast<const cdr::xsd::Group*>(n);
    if (g) {
        declareChildElements(schema, declaredElems, g->getContent(), os);
        return;
    }

    // Is this a choice or a sequence?
    const cdr::xsd::ChoiceOrSequence* cs =
        dynamic_cast<const cdr::xsd::ChoiceOrSequence*>(n);
    if (cs) {
        cdr::xsd::NodeEnum e = cs->getNodes();
        while (e != cs->getListEnd())
            declareChildElements(schema, declaredElems, *e++, os);
        return;
    }

    // Sanity check.
    if (dynamic_cast<const cdr::xsd::SimpleContent*>(n))
        throw cdr::Exception(L"Internal error",
                             L"Content must be element, group, choice, "
                             L"sequence, or simpleContent");
}

/**
 * Examines the set of enumerated values to determine whether they all match
 * the NMTOKEN production of the XML 1.0 recommendation.
 */
bool areNMTokens(const cdr::StringSet& vals)
{
    cdr::StringSet::const_iterator i = vals.begin();
    while (i != vals.end())
        if (!cdr::xsd::isNMToken(*i++))
            return false;
    return true;
}
