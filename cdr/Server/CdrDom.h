/*
 * $Id: CdrDom.h,v 1.9 2000-10-05 21:22:29 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2000/10/04 18:21:58  bkline
 * Expanded the exception types.
 *
 * Revision 1.7  2000/05/09 21:10:39  bkline
 * Added error handler.
 *
 * Revision 1.6  2000/05/04 01:14:08  bkline
 * More ccdoc comments.
 *
 * Revision 1.5  2000/04/26 01:35:58  bkline
 * Added overloaded second parse(const cdr::String&) method.
 *
 * Revision 1.4  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.3  2000/04/22 15:38:53  bkline
 * Added more documentation comments.
 *
 * Revision 1.2  2000/04/16 19:10:30  bkline
 * Added new function getTextContent().
 *
 * Revision 1.1  2000/04/11 14:17:50  bkline
 * Initial revision
 */

#ifndef CDR_DOM_H_
#define CDR_DOM_H_

// System headers.
#include <string>

// Interface to IBM/Apache XML parsers.
#include <util/PlatformUtils.hpp>
#include <parsers/DOMParser.hpp>
#include <framework/StdInInputSource.hpp>
#include <dom/DOM_Node.hpp>
#include <dom/DOM_NamedNodeMap.hpp>
#include <sax/ErrorHandler.hpp>
#include <sax/SAXException.hpp>
#include <sax/SAXParseException.hpp>
#include <dom/DOM_DOMException.hpp>

#include "CdrString.h"

/**@#-*/

namespace cdr {
    namespace dom {

/**@#+*/

        /** @pkg cdr.dom */

        // Map back to names in DOM standard.

        /**
         * Collections of nodes that can be accessed by name.
         */
        typedef ::DOM_NamedNodeMap  NamedNodeMap;

        /**
         * Primary datatype for the DOM model.
         */
        typedef ::DOM_Node          Node;

        /**
         * Predominant node type in the DOM model.
         */
        typedef ::DOM_Element       Element;

        /**
         * Product of a DOM parsing operation.
         */
        typedef ::DOM_Document      Document;

        /**
         * Carries error information for a failure of DOM processing.
         */
        typedef ::XMLException      XMLException;
        typedef ::DOM_DOMException  DOMException;
        typedef ::SAXException      SAXException;
        typedef ::SAXParseException SAXParseException;

        /**
         * Wrap Parser class, which is not part of standard.
         */
        class Parser : public ::DOMParser {

        public:

            /**
             * Creates a new <code>Parser</code> object, with our own custom
             * error handling object plugged into it.
             */
            Parser();

            /**
             * Releases the resources allocated for our error handler.
             */
            ~Parser();

            /**
             * Parses a narrow-character string, creating a DOM tree.
             *
             *  @param  xml     reference to string containing the UTF-8
             *                  serialization of an XML document.
             *  @exception      DOMException if a parsing error is encountered.
             */
            void parse(const std::string& xml) throw(DOMException);

            /**
             * Parses a wide-character string, creating a DOM tree.
             *
             *  @param  xml     reference to string containing the UTF-16
             *                  serialization of an XML document.
             *  @exception      DOMException if a parsing error is encountered.
             */
            void parse(const cdr::String& xml) throw(DOMException);

            /**
             * Parses a document directly from a file.
             *
             *  @param  fileName    c-style (null-terminated) string
             *                      identifying name of XML file.
             *  @exception          DOMException if file not found or a
             *                      parsing error is encountered.
             */
            void parseFile(const char* fileName) throw(DOMException);

        private:

            /**
             * Address of an instance of the error handler specialized for
             * the CDR.
             */
            ErrorHandler*       errorHandler;

            /**
             * Artificial flag used to perform initialization required by the
             * xml4c package.
             */
            static bool initialized;

            /**
             * Perform initialization required by the xml4c package.
             *
             *  @exception      DOMException if a parsing error is encountered.
             */
            static bool doInit() throw(DOMException) { 
                XMLPlatformUtils::Initialize(); return true; 
            }

            /**
             * Disabled copy constructor.
             */
            Parser(const Parser&);

            /**
             * Disabled assignment operator.
             */
            Parser& operator=(const Parser&);

        };

        /**
         * Convenience method, not part of standard DOM interface
         * (though perhaps it should be), for extracting the text
         * content from an element.
         *
         *  @param  node        reference to DOM node from which text
         *                      content is to be extracted.
         */
        extern cdr::String getTextContent(const Node& node);
    }
}

#endif
