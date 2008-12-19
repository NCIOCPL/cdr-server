/*
 * $Id: CdrCommand.h,v 1.35 2008-12-19 17:03:46 bkline Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.34  2004/08/20 19:58:55  bkline
 * Added new CdrSetDocStatus command.
 *
 * Revision 1.33  2004/08/11 17:48:15  bkline
 * Adding new command CdrAddExternalMapping.
 *
 * Revision 1.32  2004/07/08 00:32:38  bkline
 * Added CdrGetGlossaryMap command; added cdr.lib to 'make clean' target.
 *
 * Revision 1.31  2004/05/14 02:06:16  ameyer
 * Added cacheInit command.
 *
 * Revision 1.30  2003/02/10 14:04:20  bkline
 * Added new command CdrMailerCleanup.
 *
 * Revision 1.29  2003/01/28 23:45:53  ameyer
 * Added sysValue command.
 *
 * Revision 1.28  2002/11/14 13:23:58  bkline
 * Changed CdrFilter command to use filter sets.  Added CdrDelFilterSet
 * command.
 *
 * Revision 1.27  2002/11/12 11:44:37  bkline
 * Added filter set support.
 *
 * Revision 1.26  2002/06/26 02:21:52  ameyer
 * Added lastVersions(), plus a bit of extra documentation on listVersions().
 *
 * Revision 1.25  2002/06/18 20:33:52  ameyer
 * Added CdrCanDo command.
 *
 * Revision 1.24  2002/05/03 20:35:45  bkline
 * New CdrListVersions command added.
 *
 * Revision 1.23  2002/04/04 01:05:21  bkline
 * Added cdr::publish().
 *
 * Revision 1.22  2001/10/17 13:51:59  bkline
 * Added mergeProt() function.
 *
 * Revision 1.21  2001/09/19 18:41:54  bkline
 * Added CdrPasteLink command.
 *
 * Revision 1.20  2001/06/28 17:39:32  bkline
 * Added getCssFiles().  Added optional contents_only argument to function
 * DOMtoString().
 *
 * Revision 1.19  2001/05/21 20:29:31  bkline
 * Added commands for query term definition support.
 *
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
     * Deletes an existing action from the CDR authentication module.
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
     * Performs maintenance or retrievals on the sys_value table.
     * This one function handles add, replace, delete, and get, based
     * on the name of the element passed as the top level node for the
     * command.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String sysValue (Session&          session,
                            const dom::Node&  node,
                            db::Connection&   conn);

    /**
     * Reports whether the current session has authorization to perform
     * a particular action on a particular document type.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getCanDo (Session&          session,
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
     * Lists the most recent versions for the specified document.
     *
     * Safety note:
     *   This command is safest if called on a checked out document
     *   for which the caller holds the check-out.  That prevents another
     *   thread from altering the version history while the command
     *   is executed and/or before the results are used.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String listVersions  (Session&          session,
                                 const dom::Node&  node,
                                 db::Connection&   conn);

    /**
     * Gives the version numbers of the last version and the last
     * publishable version, which may be the same, and reports
     * whether the last version is identical to the current
     * working document according to isChanged().
     *
     * Similar to listVersions, but for a different purpose.
     *
     * Safety note:
     *   This command is safest if called on a checked out document
     *   for which the caller holds the check-out.  That prevents another
     *   thread from altering the version history while the command
     *   is executed and/or before the results are used.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String lastVersions  (Session&          session,
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
     * Returns a list of the filter documents in the CDR repository.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getFilters (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Returns a list of the named filter sets in the CDR.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getFilterSets(Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn);

    /**
     * Returns the attributes and contents of a named filter set.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getFilterSet (Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn);

    /**
     * Replaces the attributes and composition of a named filter set.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String repFilterSet (Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn);

    /**
     * Creates a new named filter set, establishing its attributes and
     * composition.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addFilterSet (Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn);

    /**
     * Deletes a named CDR filter set.  Blocked if the set is used as a
     * nested member of another filter set.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String delFilterSet(Session&         session,
                               const dom::Node& node,
                               db::Connection&  conn);

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
     * Retrieves the denormalized data for a link if it is valid to paste the
     * link at the proposed location.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String pasteLink     (Session&          session,
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
     * Send the complete set of CSS stylesheets to the client.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getCssFiles(Session&          session,
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
     * Merges a ScientificProtocoInfo document into an InScopeProtocol
     * document, then deletes the ScientificProtocolInfo document.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String mergeProt  (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Creates a new CDR publishing job.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String publish    (Session&          session,
                              const dom::Node&  node,
                              db::Connection&   conn);

    /**
     * Deletes mailer tracking documents for failed mailer jobs.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String mailerCleanup(Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn);

    /**
     * Converts a DOM node to its String representation
     *
     *  @param      node        contains the XML to be converted.
     *  @param      contents_only
     *                          true if only contents should be output
     *                          i.e., no tag if argument is element
     *  @return                 String object containing the XML
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String DOMtoString(const dom::Node&, bool contents_only = false);

    /**
     * Start or stop cacheing in the CDR server to optimize publishing
     * and/or possibly other services.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        empty element with 'on' or 'off' attributes.
     *                           on="term" starts term denormalization cacheing
     *                           on="pub" starts all publication cacheing
     *                           on="all" starts all cacheing
     *                           off= ... same values.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 "</CdrCacheingResp>".
     *  @exception  cdr::Exception if invalid or missing attributes.
     */
    extern String cacheInit    (Session&          session,
                                const dom::Node&  node,
                                db::Connection&   conn);

    /**
     * Returns a document identifying which glossary terms should be used
     * for marking up phrases found in a CDR document.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String getGlossaryMap(Session&         session,
                                 const dom::Node& node,
                                 db::Connection&  conn);

    /**
     * Inserts a new row into the external mapping table.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String addExternalMapping(Session&         session,
                                     const dom::Node& node,
                                     db::Connection&  conn);

    /**
     * Changes the active_status column for a row in the all_docs table.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String setDocStatus(Session&         session,
                               const dom::Node& node,
                               db::Connection&  conn);

    /**
     * Records an event which occurred in a CDR client process.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String logClientEvent(Session&         session,
                                 const dom::Node& node,
                                 db::Connection&  conn);

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
