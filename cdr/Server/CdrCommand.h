/*
 * $Id: CdrCommand.h,v 1.19 2001-05-21 20:29:31 bkline Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.18  2001/05/17 17:39:09  ameyer
 * Add link administration transaction handlers.
 *
 * Revision 1.17  2001/04/13 12:19:16  bkline
 * Added listActions(), getAction(), addAction(), repAction(), and
 * delAction().
 *
 * Revision 1.16  2001/04/08 22:48:24  bkline
 * Added getTree() function.
 *
 * Revision 1.15  2001/04/05 19:50:56  ameyer
 * Added reIndexDoc.
 *
 * Revision 1.14  2001/01/17 21:51:28  bkline
 * Replaced getSchema with get/add/mod/delDocType.
 *
 * Revision 1.13  2000/12/28 13:30:36  bkline
 * Added getSchema function.
 *
 * Revision 1.12  2000/10/23 14:54:45  mruben
 * added commands for version control
 *
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
     * Retrieves a list of the actions subject to access control in the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listActions(Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves an existing action from the CDR authentication module.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getAction(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn);

    /**
     * Adds a new action to the CDR authentication module.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addAction(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn);

    /**
     * Modifies an existing action in the CDR authentication module.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String repAction(Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn);

    /**
     * Delets an existing action from the CDR authentication module.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delAction(Session&          session,
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
     * Returns a list of available query term rules.  Rules cannot be created
     * through the CDR command interface.  They are inserted by the programmer
     * implementing the custom software behind the rule.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listQueryTermRules(Session&          session,
                                     const dom::Node&  node,
                                     db::Connection&   conn);

    /**
     * Returns a list of CDR query term definitions.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listQueryTermDefs(Session&          session,
                                    const dom::Node&  node,
                                    db::Connection&   conn);

    /**
     * Adds a new query term definition.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addQueryTermDef(Session&          session,
                                  const dom::Node&  node,
                                  db::Connection&   conn);

    /**
     * Deletes an existing query term definition.  There is no command for
     * modifying an existing query term definition.  Instead a command is
     * issued to delete a definition by identifying its path and rule (if
     * present) and then a second command to add the new definition is
     * submitted.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delQueryTermDef(Session&          session,
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
     * Re-index a document by updating the query_term table.
     * Used if indexing requirements change.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String reIndexDoc (Session&          session,
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
     * Add or modify a link type definition in the relational tables
     * controlling link definitions and behavior.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String putLinkType (Session&          session,
                               const dom::Node&  node,
                               db::Connection&   conn);

    /**
     * Retrieve all information pertaining to a link type in the
     * relational tables controlling link definitions and behavior.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getLinkType (Session&          session,
                               const dom::Node&  node,
                               db::Connection&   conn);

    /**
     * Retrieve a list of names of defined link type names and comments.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listLinkTypes (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Retrieve a list of defined types of properties that can be
     * applied to link types.
     *
     * Types declared in the link_prop_types table correspond to custom
     * validation processing routines for links.  It makes no sense to
     * define one without writing the code to perform the corresponding
     * validation.  Therefore there is no external CdrCommand to create
     * these, only this command to retrieve them so that users using the
     * administrative interface can assign existing property types to
     * link types.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listLinkProps (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Provides a list of schema documents currently stored in the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listSchemaDocs(Session&          session,
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
     * Load the row from the doc_type table for the requested document type.
     * Send back the schema and other related document type information.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getDocType (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Adds a new document type to the system.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addDocType (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Modifies an existing document type in the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String modDocType (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Removes a document type from the CDR system.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delDocType (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Retrieves parents and children of a node in the terminology tree
     * formed by the documents of type Term in the CDR repository.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getTree    (Session&          session,
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
