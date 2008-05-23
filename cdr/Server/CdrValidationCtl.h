/**
 * Definitions of validation control classes.
 *
 * Objects of these classes are primarily used in CdrValidateDoc.cpp.
 * However, access to some complex validation information is also required
 * in CdrDoc.cpp.  Therefore these classes are defined in a separate
 * header file that can be included by both.
 *
 * The code standing behind these classes is all defined in
 * CdrValidateDoc.cpp.
 *
 * $Id: CdrValidationCtl.h,v 1.3 2008-05-23 03:01:23 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
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
     * The context of a particular validation error.
     *
     * For now, the context of validation is simply the element node
     * for the element within which the error is recognized.  This may
     * be the element we were examining when we decided there was an
     * error, or it may be an ancestor of that element.
     *
     * Later on we might have more information here, which is why
     * we're creating our own object wrapper around the node.
     */
    class ValidationElementContext {
        public:
            /**
             * Default constructor.
             */
            ValidationElementContext ();

            /**
             * Constructor
             *
             *  @param cNode    The dom node for an element.
             */
            ValidationElementContext (cdr::dom::Node& cNode);

            /**
             * Accessor.
             *
             * @return          The dom node as what it really is.
             */
            cdr::dom::Node& getNode() {
                return contextNode;
            }

            /**
             * Accessor.
             *
             * @return          True = contextNode has been set.
             */
            bool contextEstablished() {
                return hasContext;
            }

            /**
             * Get the CDR error id attribute value associated with this
             * element.
             *
             *  @return         The value of this node's cdr-eid attribute.
             *  @throws         cdr::Exception if no cdr-eid attr.  The
             *                  caller should know if these exist.
             */
            cdr::String getErrorIdValue();

        private:
            // Always an element node
            cdr::dom::Element contextNode;

            // Except when there isn't one
            bool hasContext;
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
            ValidationError (ValidationElementContext& ctxt,
                             cdr::String& msg, cdr::String& errorId);

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
             * Set the error severity level.
             *
             *  @param level            New severity level.
             */
            void setErrorLevel (ErrLevel level) { errLevel = level; }

            /**
             * Has a context node been set for this element?
             *
             * @return          True = contextNode has been set.
             */
            bool hasContext() {
                return errCtxt.contextEstablished();
            }

        private:
            // The context node for the error
            ValidationElementContext errCtxt;

            // cdr-eid error attribute for element with this error
            // NOTE: This attribute should have been named "cdr:eid",
            //    however a bug in XMetal's XPath implementation made
            //    it impossible to locate attributes with namespaces
            //    using XPath expressions.  The bug has been reported
            //    the XMetal vendor, but we have decided to implement
            //    a workaround by using the non-namespaced attribute
            //    name "cdr-eid".
            cdr::String errId;

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
             * A validation program uses this to record where it is in
             * the document when testing validation rules.
             *
             * Sometimes errors pertain to a document as a whole.  In
             * that case, the element context should be set to 0.
             *
             *  @param ctxtNode     Context DOM node.  0=No element context.
             */
            void setElementContext (cdr::dom::Node& ctxtNode);

            /**
             * Declare an error.
             *
             * The message and current context will be remembered.
             *
             * The errId parameter shows the value of the cdr-eid to use
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
             *  @param errorId      Value of cdr-eid to force to be used.
             */
            void addError (cdr::String msg, cdr::String errorId=L"");

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
             *  @return             Serial XML with error info.
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
             *  @return             Count of errors.
             */
            int getErrorCount () const;

            /**
             * Find out whether we are tracking locations or not in this
             * object.
             *
             *  @return             True = we are tracking locators.
             */
            bool hasLocators();

            /**
             * Has this object set the current context yet?
             *
             * @return              True = Context was set.
             */
            bool hasContext();

        private:
            // Accumlator for all the errors found
            std::vector<ValidationError> errVector;

            // Current element context for the next error we find
            // We might need a stack of these, but we'll try to get
            //   away without that for now
            ValidationElementContext currentCtxt;

            // True = the currentCtxt has been set
            bool currentContextSet;

            // True = we're using error IDs.  Other software must have
            //   generated the ids into the XML.
            // Problems will occur if that hasn't happened.
            bool usingErrorIds;
    };

} // namespace cdr

#endif // CDR_VALIDATION_CTL_
