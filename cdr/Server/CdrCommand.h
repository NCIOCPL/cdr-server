/*
 * $Id: CdrCommand.h,v 1.6 2000-05-03 18:15:55 bkline Exp $
 *
 * Interface for CDR command handlers.
 *
 * $Log: not supported by cvs2svn $
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
    extern String logon      (Session&, const dom::Node&, db::Connection&);

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
    extern String logoff     (Session&, const dom::Node&, db::Connection&);

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
    extern String checkAuth  (Session&, const dom::Node&, db::Connection&);

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
    extern String addGrp     (Session&, const dom::Node&, db::Connection&);

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
    extern String getGrp     (Session&, const dom::Node&, db::Connection&);

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
    extern String modGrp     (Session&, const dom::Node&, db::Connection&);

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
    extern String delGrp     (Session&, const dom::Node&, db::Connection&);

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
    extern String listGrps   (Session&, const dom::Node&, db::Connection&);

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
    extern String addUsr     (Session&, const dom::Node&, db::Connection&);

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
    extern String getUsr     (Session&, const dom::Node&, db::Connection&);

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
    extern String modUsr     (Session&, const dom::Node&, db::Connection&);

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
    extern String delUsr     (Session&, const dom::Node&, db::Connection&);

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
    extern String listUsrs   (Session&, const dom::Node&, db::Connection&);

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
    extern String addDoc     (Session&, const dom::Node&, db::Connection&);

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
    extern String repDoc     (Session&, const dom::Node&, db::Connection&);

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
    extern String delDoc     (Session&, const dom::Node&, db::Connection&);

    /**
     * Examines the specified document to determine whether it meets the
     * Schema and Link validation requirements.  The document can be submitted
     * as part of the command, or can be referenced by it document ID.
     *
     *  @param      session     contains information about the current user.
     *  @param      node        contains the XML for the command.
     *  @param      conn        reference to the connection object for the
     *                          CDR database.
     *  @return                 String object containing the XML for the
     *                          command response.
     *  @exception  cdr::Exception if a database or processing error occurs.
     */
    extern String validateDoc(Session&, const dom::Node&, db::Connection&);

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
    extern String search     (Session&, const dom::Node&, db::Connection&);

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
    extern String getDoc     (Session&, const dom::Node&, db::Connection&);

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
    extern String checkOut   (Session&, const dom::Node&, db::Connection&);

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
    extern String checkIn    (Session&, const dom::Node&, db::Connection&);

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
    extern String report     (Session&, const dom::Node&, db::Connection&);

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
    extern String filter     (Session&, const dom::Node&, db::Connection&);

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
    extern String getLinks   (Session&, const dom::Node&, db::Connection&);

}

#endif
