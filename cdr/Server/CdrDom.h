/*
 * $Id: CdrDom.h,v 1.3 2000-04-22 15:38:53 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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

#include "CdrString.h"

namespace cdr {
    namespace dom {

        // Map back to names in DOM standard.
        typedef ::DOM_NamedNodeMap  NamedNodeMap;
        typedef ::DOM_Node          Node;
        typedef ::DOM_Element       Element;
        typedef ::DOM_Document      Document;
        typedef ::XMLException      DOMException;

        /**
         * Wrap Parser class, which is not part of standard.
         */
        class Parser : public ::DOMParser {
        public:
            void parse(const std::string& xml) throw(cdr::dom::DOMException);
        private:
            static bool initialized;
            static bool doInit() throw(cdr::dom::DOMException) { 
                XMLPlatformUtils::Initialize(); return true; 
            }
        };

        /**
         * Convenience method, not part of standard DOM interface
         * (though perhaps it should be), for extracting the text
         * content from an element.
         */
        extern cdr::String getTextContent(const Node&);
    }
}

#endif
