/**
 * Definitions of validation control class.
 *
 * Objects of these classes are primarily used in CdrValidateDoc.cpp.
 * However, access to some complex validation information is also required
 * in CdrDoc.cpp.
 *
 * The code standing behind these classes is all defined in
 * CdrValidateDoc.cpp.
 *
 * $Id$
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2008/05/23 04:34:58  ameyer
 * Added some needed accessors.
 * Made a couple more functions const.
 *
 * Revision 1.3  2008/05/23 03:01:23  ameyer
 * Added controls for marking the type and severity level of errors.
 *
 * Revision 1.2  2008/04/10 20:09:41  ameyer
 * Renamed cdr:eid to cdr-eid to workaround XMetal namespace bug.
 *
 * Revision 1.1  2008/03/25 23:41:15  ameyer
 * Header for validation controls.
 *
 */

#ifndef CDR_VALIDATION_CTL_
#define CDR_VALIDATION_CTL_

#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * A validation error has no context info, i.e., we don't know what
     * element the error occurred in.
     */
    const cdr::String NO_ERROR_CONTEXT = L"";

    /**
     * Values distinguishing different kinds of errors.
     */
    enum ErrType {
        ETYPE_VALIDATION,
        ETYPE_OTHER
    };

    /**
     * Values distinguishing severity levels of errors.
     */
    enum ErrLevel {
        ELVL_INFO,
        ELVL_WARNING,
        ELVL_ERROR,
        ELVL_FATAL
    };

    /**
     * An internal representation of one single validation error.
     */
    class ValidationError {
        public:
            /**
             * Default constructor.
             */
            ValidationError () {};

            /**
             * Constructor.
             *
             *  @param ctxt     Context within which the error occurred.
             *  @param msg      Error message for users.
             *  @param errorId  Value of cdr-eid to force to be used.
             */
            ValidationError (cdr::String& msg, cdr::String& errorId);

            /**
             * Access the msssage.
             */
            cdr::String getMessage() { return errMsg; }

            /**
             * Create an XML string representing the error, with or
             * without cdr:eref location information.
             *
             *  @param includeLocator   True = include a cdr:eref identifying
             *                          the cdr-eid for the error's context.
             */
            cdr::String toXmlString (bool includeLocator);

            /**
             * Set the error type.
             *
             *  @param type             New type.
             */
            void setErrorType (ErrType type) { errType = type; }

            /**
             * Get error type.
             *
             *  @return                 Type.
             */
            ErrType getErrorType() const { return errType; }

            /**
             * Set the error severity level.
             *
             *  @param level            New severity level.
             */
            void setErrorLevel (ErrLevel level) { errLevel = level; }

            /**
             * Get error severity level.
             *
             *  @return                 Level.
             */
            ErrLevel getErrorLevel() const { return errLevel; }

            /**
             * Has a context node been set for this element?
             *
             * @return          True = contextNode has been set.
             */
            bool hasContext();

        private:
            // cdr-eid error attribute for element with this error
            // NOTE: This attribute should have been named "cdr:eid",
            //    however a bug in XMetal's XPath implementation made
            //    it impossible to locate attributes with namespaces
            //    using XPath expressions.  The bug has been reported
            //    the XMetal vendor, but we have decided to implement
            //    a workaround by using the non-namespaced attribute
            //    name "cdr-eid".
            cdr::String errIdStr;

            // Categories of errors
            ErrType errType;

            // Severity levels
            ErrLevel errLevel;

            // Human readable error message
            cdr::String errMsg;
    };

    /**
     * Information pertaining to one validation process for one CDR
     * document.
     *
     * It holds both the control information that says how validation
     * is done, and the errors resulting from validation.
     */
    class ValidationControl {
        public:
            /**
             * Constructor.
             */
            ValidationControl();

            /**
             * Set the current element context.
             *
             * The element context is the value of the cdr-eid attribute
             * found in the element that had the error.  As we traverse the
             * dom tree we remember the current node by saving this value
             * in the ValidationControl object.  The next error to be
             * added, which may come from a routine that doesn't have
             * access to the dom node, picks up this value as the current
             * context.
             *
             * A validation program uses this to record where it is in
             * the document when testing validation rules.  Our XMetal
             * client can use these to position a cursor on the element
             * with the error.
             *
             * Sometimes errors pertain to a document as a whole.  In
             * that case NO_ERROR_CONTEXT is stored in the context.
             *
             *  @param ctxtNode     Context DOM node.  ""=No element context.
             */
            void setElementContext (const cdr::dom::Element& ctxtNode);

            /**
             * Declare an error.
             *
             * The message and current context will be remembered.
             *
             * The errorId parameter shows the value of the cdr-eid to use
             * in reporting this error.  Normally, the program declaring
             * an error doesn't know this value, we use the currentCtxt
             * to find the attribute.
             *
             * However, sometimes, specifically in the custom rule XSLT
             * validation in CdrXsd, we have no direct access to the DOM
             * node, but do know the value of the associated cdr-eid.  So
             * that module uses the second parameter here to force the
             * reported cdr:eref to have the value it wants.
             *
             *  @param msg          Error message.
             *  @param errorId      Value of cdr-eid to force to use if
             *                      there is no current context.
             */
            void addError (cdr::String msg,
                           cdr::String errorId=NO_ERROR_CONTEXT);

            /**
             * Modify the type of the last error.
             *
             * Used in the relatively less common cases where an error
             * is not a validation error, but something else.
             *
             * Operates on the last ValidationError in the errVector
             *
             *  @param type         One of the enumerated types.
             */
            void setLastErrorType (ErrType type);

            /**
             * Modify the error level of the last error.
             *
             * Operates on the last ValidationError in the errVector
             *
             *  @param level        One of the enumerated types.
             */
            void setLastErrorLevel (ErrLevel level);

            /**
             * Cumulate all of the errors in a source ValidationControl
             * into this one.
             *
             *  @param errSrc       Copy errors from here.
             *
             *  @param msg          Error message.
             */
            void cumulateErrors (ValidationControl& errSrc);

            /**
             * Construct a string of serialized XML containing all of
             * the errors found for the document.
             *
             * If the object is usingErrorIds, error location cdr:erefs
             * will be included.  Else not.
             *
             * The function can pack errors that are stored in the error
             * vector and also errors stored in a separately passed
             * StringList of error messages.  This is because some errors can
             * occur inside document validation and some can occur outside it.
             *
             *  @param errList      List of error messages that do not
             *                      reference elements.
             *                      0 or empty list means there are none.
             *
             *  @return             If errors exist:
             *                          Serial XML with error info.
             *                      Else:
             *                          Empty string (L"").
             *
             *  @throws             cdr::Exception if locators requested but
             *                      they weren't requested when this
             *                      ValidationControl was constructed.
             */
            cdr::String getErrorXml (StringList& errList);

            /**
             * Get all the error messages in the object in the form
             * of a StringList.
             *
             *  @param msgList      Append them to this list.
             */
            void getErrorMsgs (StringList& msgList);

            /**
             * Setter for withLocators, called before validation if need.
             *
             *  @param locators     True = use cdr-eid/ref error locators.
             */
            void setLocators (bool locators);

            /**
             * Get the total number of errors.
             *
             * Returns the number of items in the errVector.  Errors
             * created by other older mechanisms aren't counted.
             *
             *  @return             Count of errors.
             */
            int getErrorCount () const;

            /**
             * Get the number of ValidationErrors with a specific
             * error level.
             *
             *  @param level        Level to count.
             *  @return             Count.
             */
            int getLevelCount (ErrLevel level) const;

            /**
             * Find out whether we are tracking locations or not in this
             * object.
             *
             *  @return             True = we are tracking locators.
             */
            bool hasLocators() const;

            /**
             * Has this object set the current context yet?
             *
             * @return              True = Context was set.
             */
            bool hasContext() const;

        private:
            // Accumlator for all the errors found
            std::vector<ValidationError> errVector;

            // Current element context for the next error we find
            // We might need a stack of these, but we'll try to get
            //   away without that for now
            cdr::String currentCtxt;

            // True = we're using error IDs.  Other software must have
            //   generated the ids into the XML.
            // Problems will occur if that hasn't happened.
            bool usingErrorIds;
    };

} // namespace cdr

#endif // CDR_VALIDATION_CTL_
