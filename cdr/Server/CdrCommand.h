/*
 * $Id: CdrCommand.h,v 1.12 2000-10-23 14:54:45 mruben Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.11  2000/10/04 18:21:06  bkline
 * Added searchLinks and listDocTypes.
 *
 * Revision 1.10  2000/05/21 00:53:13  bkline
 * Added shutdown command.
 *
 * Revision 1.9  2000/05/10 20:29:50  mruben
 * added declaration for DOMtoString
 *
 * Revision 1.8  2000/05/09 21:09:40  bkline
 * More ccdoc comments.
 *
 * Revision 1.7  2000/05/03 18:49:14  bkline
 * Added parameter names (for ccdoc).
 *
 * Revision 1.6  2000/05/03 18:15:55  bkline
 * More ccdoc comments.
 *
 * Revision 1.5  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.4  2000/04/22 15:34:56  bkline
 * Filled out documentation comments.
 *
 * Revision 1.3  2000/04/16 19:11:36  bkline
 * Added const qualifier to Node argument in Command signature.
 *
 * Revision 1.2  2000/04/15 12:05:34  bkline
 * Changed DbConnection* to DbConnection&.  Removed redundant namespace
 * qualifiers.
 *
 * Revision 1.1  2000/04/14 15:58:04  bkline
 * Initial revision
 */

#ifndef CDR_COMMAND_
#define CDR_COMMAND_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * All external commands have this signature.
     */
    typedef String (*Command)(
            Session&,           // Contains information about current user.
            const dom::Node&,   // Contains XML for the command.
            db::Connection&     // Connection to the CDR database.
    );

    /**
     * Finds the command which matches the specified name.
     */
    extern Command lookupCommand(const String&);

    /**
     * Creates a new session for the specified user.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String logon      (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Closes the current session.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String logoff     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Reports which of the specified actions are authorized for the current
     * user.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String checkAuth  (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Creates a new CDR authorization group.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addGrp     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves the permissions and members for the specified CDR group.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getGrp     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Replaces the settings for the specified group.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String modGrp     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Removes the specified group from the CDR.  Any depended actions or user
     * memberships are removed as well.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delGrp     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves a list of the CDR group names.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listGrps   (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Creates a new user account in the CDR, optionally adding the user
     * to one or more of the CDR groups.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addUsr     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves the information associated with the specified user account.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getUsr     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Updates the information associated with the specified user account.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String modUsr     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Removes the specified user account from the CDR.  Request will be
     * refused if any actions have been performed using the account.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delUsr     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves a list of the user account names currently in the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listUsrs   (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Adds a new document to the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addDoc     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Updates an existing CDR document.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String repDoc     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Marks a CDR document as deleted (does not actually destroy the data or
     * any of the version control information).
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delDoc     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Examines the specified document to determine whether it meets the
     * Schema and Link validation requirements.  The document can be submitted
     * as part of the command, or can be referenced by it document ID.
     * <P>
     * Validates the document using the following steps.
     *  <OL>
     *   <LI> Get the document to be validated (from the database or directly 
     *        from the command, depending on the variant invoked)</LI>
     *   <LI> Parse the document</LI>
     *   <LI> Extract the document type</LI>
     *   <LI> Retrieve the schema for the document type</LI>
     *   <LI> Validate the document against the schema</LI>
     *   <LI> Verify the validity of the links to and from the document</LI>
     *  </OL>
     *
     * As many errors are reported as possible (in contrast to an algorithm
     * which bails out when the first error is encountered).  Obviously, if the
     * document isn't even well-formed, there's not much else we can report,
     * however.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String validateDoc(Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves a list of document IDs for documents which match the
     * specified query.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String search     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves a list of potential link targets for a specified combination
     * of source document type and element.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String searchLinks(Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves a &lt;CdrDoc&gt; element for the specified CDR document.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getDoc     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Marks the specified document as checked out by the current user.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String checkVerOut   (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Releases the check-out status for the specified document.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String checkVerIn    (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Creates a new version label
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String createLabel   (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Deletes a version label
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String deleteLabel   (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Labels a document/version
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String labelDocument (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Removes a label from a document/version
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String unlabelDocument (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Produces a canned CDR report.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String report     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Transforms a CDR document using a filter.  The filter can be embedded
     * within the command, or referenced by document ID.  More than one filter
     * can be applied by the command, in which case the filters will be
     * applied sequentially, in the order specified.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String filter     (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Reports the links which exist in the CDR to and from the specified
     * document.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getLinks   (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Provides a list of document types currently defined for the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listDocTypes(Session&          session,
                               const dom::Node&  node,
                               db::Connection&   conn);

    /**
     * Converts a DOM node to its String representation
     *
     *  @param      node        contains the XML to be converted.
     *  @return                 String object containing the XML
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String DOMtoString(const dom::Node&);

    /**
     * Shuts down the CDR Server.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database error occurs or the user
     *                          is not authorized to shut down the server.
     */
    extern String shutdown   (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);
}

#endif
