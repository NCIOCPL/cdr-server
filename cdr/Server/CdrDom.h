/*
 * $Id: CdrDom.h,v 1.1 2000-04-11 14:17:50 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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

namespace cdr {
    namespace dom {

        // Map back to names in DOM standard.
        typedef ::DOM_NamedNodeMap  NamedNodeMap;
        typedef ::DOM_Node          Node;
        typedef ::DOM_Element       Element;
        typedef ::DOM_Document      Document;
        typedef ::XMLException      DOMException;

        // Wrap Parser class, which is not part of standard.
        class Parser : public ::DOMParser {
        public:
            void parse(const std::string& xml) throw(cdr::dom::DOMException);
        private:
            static bool initialized;
            static bool doInit() throw(cdr::dom::DOMException) { 
                XMLPlatformUtils::Initialize(); return true; 
            }
        };
    }
}

#endif