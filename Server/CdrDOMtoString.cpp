/*
 * $Id$
 *
 * Generate CdrString containing representation of a DOM node
 *
 * Based on code distributed with the XML4C DOM parser
 *
 * $Log: not supported by cvs2svn $
 *
 */

#include "CdrDom.h"
#include "CdrException.h"
#include "CdrString.h"

namespace
{
  using namespace cdr;
  
  /***************************************************************************/
  /* produce cdr::String representation of a DOM node                        */
  /***************************************************************************/
  String outputContent(const String &node)
  {
    String result;
    result.reserve(node.length());
    
    int length = node.length();
    const XMLCh* chars = node.c_str();
          
    int index;
    for (index = 0; index < length; index++)
    {
      switch (chars[index])
      {
        case chAmpersand :
          result += L"&amp;";
          break;
                  
        case chOpenAngle :
          result += L"&lt;";
          break;
                  
        case chCloseAngle:
          result += L"&gt;";
          break;
                  
        case chDoubleQuote :
          result += L"&quot;";
          break;
                  
        default:
          // If it is none of the special characters, print it as such
          result += node.substr(index, 1);
          break;
      }
    }
  
    return result;
  }
}

namespace cdr
{
  String DOMtoString(const dom::Node& node)
  {
    String result;
    
    // Get the name and value out for convenience
    String nodeName(node.getNodeName());
    String nodeValue(node.getNodeValue());
    
    switch (node.getNodeType())
    {
      case dom::Node::TEXT_NODE:
      {
        result += outputContent(nodeValue);
        break;
      }
  
      case dom::Node::PROCESSING_INSTRUCTION_NODE :
      {
        result = "<?"
               + nodeName
               + L' '
               + nodeValue
               + "?>";
        break;
      }
  
      case dom::Node::DOCUMENT_NODE :
      {
        result = "<?xml version='1.0' encoding='ISO-8859-1' ?>\n";
        dom::Node child = node.getFirstChild();
        while (child != 0)
        {
          result += DOMtoString(child) + L'\n';
          child = child.getNextSibling();
        }
        break;
      }
  
      case dom::Node::ELEMENT_NODE :
      {
        // Output the element start tag.
        result = L'<' + nodeName;
  
        // Output any attributes on this element
        dom::NamedNodeMap attributes = node.getAttributes();
        int attrCount = attributes.getLength();
        for (int i = 0; i < attrCount; i++)
        {
          dom::Node attribute = attributes.item(i);
  
          result += L' ' + attribute.getNodeName()
                  + " = \"";
          //  Note that "<" must be escaped in attribute values.
          result += outputContent(attribute.getNodeValue());
          result += '"';
        }
  
        //
        //  Test for the presence of children, which includes both
        //  text content and nested elements.
        //
        dom::Node child = node.getFirstChild();
        if (child != 0)
        {
          // There are children. Close start-tag, and output children.
          result += L'>';
          while( child != 0)
          {
            result += DOMtoString(child);
            child = child.getNextSibling();
          }
  
          // Done with children.  Output the end tag.
          result += "</" + nodeName + L'>';
        }
        else
        {
          //
          //  There were no children.  Output the short form close of the
          //  element start tag, making it an empty-element tag.
          //
          result += "/>";
        }
        break;
      }
  
      case dom::Node::ENTITY_REFERENCE_NODE:
      {
        dom::Node child;
        for (child = node.getFirstChild();
             child != 0;
             child = child.getNextSibling())
          result += DOMtoString(child);
        break;
      }
  
      case dom::Node::CDATA_SECTION_NODE:
      {
        result = "<![CDATA[" + nodeValue + "]]>";
        break;
      }
  
      case dom::Node::COMMENT_NODE:
      {
        result = "<!--" + nodeValue + "-->";
        break;
      }
  
      default:
        throw Exception(L"Invalid DOM in CdrDOMtoString");
    }
    return result;
  }
}
