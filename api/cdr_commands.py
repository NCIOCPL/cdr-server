"""
HTTPS interface for handling CDR client-server requests

Pulled out separate from the main CGI script for pre-compilation.
"""

import base64
import datetime
import os
import sys
import threading
from lxml import etree
from cdrapi import db
from cdrapi.users import Session
from cdrapi.settings import Tier
from cdrapi.docs import Doc, Doctype, FilterSet, LinkType, GlossaryTermName
from cdrapi.reports import Report
from cdrapi.searches import QueryTermDef, Search
from cdrapi.publishing import Job


class CommandSet:
    """
    XML wrappers for the CDR client commands

    For documentation of each command, consult the comments in the modules
    in the cdrapi package.
    """

    PARSER = etree.XMLParser(strip_cdata=False, huge_tree=True)
    COMMANDS = dict(
        CdrAddAction="_add_action",
        CdrAddDoc="_put_doc",
        CdrAddDocType="_put_doctype",
        CdrAddExternalMapping="_add_external_mapping",
        CdrAddFilterSet="_put_filter_set",
        CdrAddGrp="_add_group",
        CdrAddLinkType="_put_link_type",
        CdrAddQueryTermDef="_add_query_term_def",
        CdrAddUsr="_put_user",
        CdrCanDo="_can_do",
        CdrCheckAuth="_check_auth",
        CdrCheckIn="_check_in",
        CdrCheckOut="_check_out",
        CdrCreateLabel="_create_label",
        CdrDelAction="_del_action",
        CdrDelDoc="_del_doc",
        CdrDelDocType="_del_doctype",
        CdrDeleteLabel="_del_label",
        CdrDelFilterSet="_del_filter_set",
        CdrDelGrp="_del_group",
        CdrDelLinkType="_del_linktype",
        CdrDelQueryTermDef="_del_query_term_def",
        CdrDelUsr="_del_user",
        CdrDupSession="_dup_session",
        CdrFilter="_filter_doc",
        CdrGetAction="_get_action",
        CdrGetCssFiles="_get_css_files",
        CdrGetDoc="_get_doc",
        CdrGetDocType="_get_doctype",
        CdrGetFilters="_get_filters",
        CdrGetFilterSet="_get_filter_set",
        CdrGetFilterSets="_get_filter_sets",
        CdrGetGlossaryMap="_get_glossary_map",
        CdrGetGrp="_get_group",
        CdrGetLinks="_get_links",
        CdrGetLinkType="_get_link_type",
        CdrGetSpanishGlossaryMap="_get_glossary_map",
        CdrGetTree="_get_tree",
        CdrGetUsr="_get_user",
        CdrLabelDocument="_label_doc_version",
        CdrLastVersions="_last_versions",
        CdrListActions="_list_actions",
        CdrListDocTypes="_list_doctypes",
        CdrListGrps="_list_groups",
        CdrListLinkProps="_list_linkprops",
        CdrListLinkTypes="_list_linktypes",
        CdrListQueryTermDefs="_list_query_term_defs",
        CdrListQueryTermRules="_list_query_term_rules",
        CdrListSchemaDocs="_list_schema_docs",
        CdrListUsrs="_list_users",
        CdrListVersions="_list_versions",
        CdrLogClientEvent="_log_client_event",
        CdrLogoff="_logoff",
        CdrMailerCleanup="_mailer_cleanup",
        CdrModDocType="_put_doctype",
        CdrModGrp="_mod_group",
        CdrModLinkType="_put_link_type",
        CdrModUsr="_put_user",
        CdrPasteLink="_paste_link",
        CdrPublish="_publish",
        CdrReindexDoc="_reindex_doc",
        CdrRepAction="_mod_action",
        CdrRepDoc="_put_doc",
        CdrRepFilterSet="_put_filter_set",
        CdrReport="_report",
        CdrSaveClientTraceLog="_save_client_trace_log",
        CdrSearch="_search",
        CdrSearchLinks="_search_links",
        CdrSetCtl="_set_ctl",
        CdrSetDocStatus="_set_doc_status",
        CdrUnlabelDocument="_unlabel_doc_version",
        CdrUpdateTitle="_update_title",
        CdrValidateDoc="_validate_doc",
    )

    def __init__(self):
        """
        Capture the environment and catch the incoming command set

        The XML wrapper API is set up to handler multiple commands
        within a single HTTPS request. In actual practice, each
        command is (almost?) always sent separately.
        """

        self.tier = Tier()
        self.level = os.environ.get("HTTP_X_LOGGING_LEVEL", "INFO")
        self.logger = self.tier.get_logger("https_api", level=self.level)
        self.start = datetime.datetime.now()
        self.session = None
        self.client = os.environ.get("REMOTE_ADDR")
        self.root = self.__load_commands()

    def get_responses(self):
        """
        Process the commands in the set and assemble the responses

        Return:
          `CdrResponseSet` block `etree._Element` object
        """

        responses = []
        for node in self.root.findall("*"):
            if node.tag == "SessionId":
                try:
                    name = node.text.strip()
                    self.session = Session(name)
                except Exception as e:
                    self.logger.warning(str(e))
                    return self.__wrap_responses(self.__wrap_error(e))
            elif node.tag == "CdrCommand":
                try:
                    responses.append(self.__process_command(node))
                except Exception:
                    self.logger.exception("{} failure".format(node.tag))
            else:
                self.logger.warning("unexpected element %r", node.tag)
        return self.__wrap_responses(*responses)

    # ------------------------------------------------------------------
    # HELPER METHODS START HERE.
    # ------------------------------------------------------------------

    def __load_commands(self):
        """
        Read the request from the web server

        Return:
          parsed `CdrCommandSet` element block
        """

        content_length = os.environ.get("CONTENT_LENGTH")
        if content_length:
            request = sys.stdin.buffer.read(int(content_length))
        else:
            request = sys.stdin.buffer.read()
        self.logger.info("%s bytes from %s", len(request), self.client)
        thread_id = threading.current_thread().ident
        values = os.getpid(), thread_id, request.decode("utf-8")
        args = "process_id, thread_id, received, request", "?, ?, GETDATE(), ?"
        cmd = "INSERT INTO api_request ({}) VALUES ({})".format(*args)
        try:
            conn = db.connect()
            cursor = conn.cursor()
            cursor.execute(cmd, values)
            conn.commit()
        except Exception:
            self.logger.exception("Failure recording command set")
        root = etree.fromstring(request, parser=self.PARSER)
        if root.tag != "CdrCommandSet":
            raise Exception("not a CDR command set")
        return root

    def __process_command(self, node):
        """
        Pass off the work for a CDR command to its handler

        Exceptions are caught here and return in an `Errors` element.
        Attributes for Status, Elapsed, and (optionally) CmdId are addded
        to the returned element.

        Pass:
          node - parsed `CdrCommand` element

        Return:
          `CdrResponse` element as `etree._Element` object
        """

        start = datetime.datetime.now()
        response = etree.Element("CdrResponse")
        command_id = node.get("CmdId")
        if command_id:
            response.set("CmdId", command_id)
        try:
            if not self.session:
                raise Exception("Missing session ID")
            child = node.find("*")
            if child is None:
                raise Exception("Missing specific command element")
            self.logger.info(child.tag)
            handler = self.COMMANDS.get(child.tag)
            if handler is None:
                raise Exception("Unknown command: {}".format(child.tag))
            response.append(getattr(self, handler)(child))
            response.set("Status", "success")
        except Exception as e:
            self.logger.exception("{} failure".format(node.tag))
            response.append(self.__wrap_error(e))
            response.set("Status", "failure")
        elapsed = (datetime.datetime.now() - start).total_seconds()
        response.set("Elapsed", "{:f}".format(elapsed))
        return response

    def __wrap_error(self, error):
        """
        Enclose an error string in a structure expected by the client

        Pass:
          string describing a failure condition

        Return:
          `Errors` element as `etree._Element` object
        """

        errors = etree.Element("Errors")
        etree.SubElement(errors, "Err").text = str(error)
        return errors

    def __wrap_responses(self, *responses):
        """
        Enclose the individual response elements in a single element

        Adds a time stamp attribute to the wrapper element.

        Pass:
          sequence of `etree._Element` nodes for he command responses

        Return:
          `CdrResponseSet` block `etree._Element` object
        """

        response_set = etree.Element("CdrResponseSet")
        response_set.set("Time", self.start.strftime("%Y-%m-%dT%H:%M:%S.%f"))
        for response in responses:
            response_set.append(response)
        serialized_response = etree.tostring(response_set, encoding="utf-8")
        self.logger.info("returning %d bytes", len(serialized_response))
        return serialized_response

    @staticmethod
    def get_node_text(node, default=None):
        """
        Assemble the concatenated text nodes for an element of the document.

        Note that the call to node.itertext() must include the wildcard
        string argument to specify that we want to avoid recursing into
        nodes which are not elements. Otherwise we will get the content
        of processing instructions, and how ugly would that be?!?

        Pass:
            node - element node from an XML document parsed by the lxml package
            default - what to return if the node is None

        Return:
            default if node is None; otherwise concatenated string node
            descendants
        """

        if node is None:
            return default
        return "".join(node.itertext("*"))

    # ------------------------------------------------------------------
    # MAPPED COMMAND METHODS START HERE.
    # ------------------------------------------------------------------

    def _add_action(self, node):
        name = flag = comment = None
        for child in node.findall("*"):
            if child.tag == "Name":
                name = child.text.strip()
            elif child.tag == "DoctypeSpecific":
                flag = child.text
            elif child.tag == "Comment":
                comment = child.text
        action = Session.Action(name, flag, comment)
        action.add(self.session)
        return etree.Element(node.tag + "Resp")

    def _add_external_mapping(self, node):
        usage = cdr_id = value = None
        opts = dict(bogus="N", mappable="Y")
        for child in node.findall("*"):
            if child.tag == "Usage":
                usage = self.get_node_text(child)
            elif child.tag == "CdrId":
                cdr_id = self.get_node_text(child)
            elif child.tag == "Value":
                value = self.get_node_text(child)
            elif child.tag == "Bogus":
                opts["bogus"] = self.get_node_text(child)
            elif child.tag == "Mappable":
                opts["mappable"] = self.get_node_text(child)
        doc = Doc(self.session, id=cdr_id)
        mapping_id = str(doc.add_external_mapping(usage, value, **opts))
        return etree.Element(node.tag + "Resp", MappingId=mapping_id)

    def _add_group(self, node):
        opts = dict(name=None, comment=None, users=[], actions={})
        for child in node.findall("*"):
            if child.tag == "GrpName":
                opts["name"] = child.text.strip()
            elif child.tag == "Comment":
                opts["comment"] = child.text
            elif child.tag == "UserName":
                opts["users"].append(child.text)
            elif child.tag == "Auth":
                action = self.get_node_text(child.find("Action"), "").strip()
                doctype = self.get_node_text(child.find("DocType"), "").strip()
                if not action:
                    raise Exception("Missing Action element in auth")
                if action not in opts["actions"]:
                    opts["actions"][action] = []
                opts["actions"][action].append(doctype)
        group = Session.Group(**opts)
        group.add(self.session)
        return etree.Element(node.tag + "Resp")

    def _add_query_term_def(self, node):
        path = self.get_node_text(node.find("Path"))
        rule = self.get_node_text(node.find("Rule")) or None
        QueryTermDef(self.session, path, rule).add()
        return etree.Element(node.tag + "Resp")

    def _can_do(self, node):
        action = doc_type = None
        for child in node.findall("*"):
            if child.tag == "Action":
                action = child.text.strip()
            elif child.tag == "DocType":
                doc_type = child.text.strip()
        if not action:
            raise Exception("No action specified to check")
        response = etree.Element(node.tag + "Resp")
        response.text = "Y" if self.session.can_do(action, doc_type) else "N"
        return response

    def _check_auth(self, node):
        pairs = []
        for wrapper in node.findall("Auth"):
            action = self.get_node_text(wrapper.find("Action"))
            doctype = self.get_node_text(wrapper.find("DocType"))
            pairs.append((action, doctype))
        response = etree.Element(node.tag + "Resp")
        permissions = self.session.check_permissions(pairs)
        for action in sorted(permissions):
            added = False
            for doctype in sorted(permissions[action]):
                if doctype and doctype.strip():
                    wrapper = etree.SubElement(response, "Auth")
                    etree.SubElement(wrapper, "Action").text = action
                    etree.SubElement(wrapper, "DocType").text = doctype
                    added = True
            if not added:
                wrapper = etree.SubElement(response, "Auth")
                etree.SubElement(wrapper, "Action").text = action
        return response

    def _check_in(self, node):
        opts = {
            "abandon": node.get("Abandon", "N") == "Y",
            "force": node.get("ForceCheckIn", "N") == "Y",
            "publishable": node.get("Publishable", "N") == "Y",
            "comment": self.get_node_text(node.find("Comment"))
        }
        doc_id = self.get_node_text(node.find("DocumentId"))
        doc = Doc(self.session, id=doc_id)
        doc.check_in(**opts)
        response = etree.Element(node.tag + "Resp")
        version = etree.SubElement(response, "Version")
        if doc.last_version:
            version.text = str(doc.last_version)
        return response

    def _check_out(self, node):
        opts = {
            "force": node.get("ForceCheckOut", "N") == "Y",
            "comment": self.get_node_text(node.find("Comment"))
        }
        doc_id = self.get_node_text(node.find("DocumentId"))
        doc = Doc(self.session, id=doc_id)
        doc.check_out(**opts)
        response = etree.Element(node.tag + "Resp")
        version = etree.SubElement(response, "Version")
        if doc.last_version:
            version.text = str(doc.last_version)
        return response

    def _create_label(self, node):
        name = self.get_node_text(node.find("Name"))
        comment = self.get_node_text(node.find("Comment"))
        Doc.create_label(self.session, name, comment=comment)
        return etree.Element(node.tag + "Resp")

    def _del_action(self, node):
        name = self.get_node_text(node.find("Name"), "").strip()
        if not name:
            raise Exception("Missing action name")
        self.session.get_action(name).delete(self.session)
        return etree.Element(node.tag + "Resp")

    def _del_doc(self, node):
        opts = {}
        doc = None
        for child in node.findall("*"):
            if child.tag == "DocId":
                doc = Doc(self.session, id=child.text)
            elif child.tag == "Validate":
                opts["validate"] = child.text == "Y"
            elif child.tag == "Reason":
                opts["reason"] = child.text
        if doc is None:
            raise Exception("Missing document ID")
        doc.delete(**opts)
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "DocId").text = doc.cdr_id
        if doc.errors:
            response.append(doc.errors_node)
        return response

    def _del_doctype(self, node):
        name = node.get("Type")
        if not name:
            raise Exception("Missing doctype name")
        Doctype(self.session, name=name).delete()
        return etree.Element(node.tag + "Resp")

    def _del_filter_set(self, node):
        name = self.get_node_text(node.find("FilterSetName"))
        filter_set = FilterSet(self.session, name=name)
        filter_set.delete()
        return etree.Element(node.tag + "Resp")

    def _del_group(self, node):
        name = self.get_node_text(node.find("GrpName"), "").strip()
        if not name:
            raise Exception("Missing group name")
        self.session.get_group(name).delete(self.session)
        return etree.Element(node.tag + "Resp")

    def _del_label(self, node):
        name = self.get_node_text(node.find("Name"))
        Doc.delete_label(self.session, name)
        return etree.Element(node.tag + "Resp")

    def _del_linktype(self, node):
        name = self.get_node_text(node.find("Name"))
        LinkType(self.session, name=name).delete()
        return etree.Element(node.tag + "Resp")

    def _del_query_term_def(self, node):
        path = self.get_node_text(node.find("Path"))
        rule = self.get_node_text(node.find("Rule")) or None
        QueryTermDef(self.session, path, rule).delete()
        return etree.Element(node.tag + "Resp")

    def _del_user(self, node):
        name = self.get_node_text(node.find("UserName"))
        Session.User(self.session, name=name).delete()
        return etree.Element(node.tag + "Resp")

    def _dup_session(self, node):
        session = self.session.duplicate()
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "SessionId").text = self.session.name
        etree.SubElement(response, "NewSessionId").text = session.name
        return response

    def _filter_doc(self, node):
        output = node.get("Output") != "N"
        opts = {
            "output": output,
            "ctl": node.get("ctl") == "y",
            "version": node.get("FilterVersion"),
            "date": node.get("FilterCutoff")
        }
        doc = None
        filters = []
        parms = {}
        for child in node.findall("*"):
            if child.tag == "Document":
                href = child.get("href")
                if href:
                    ver = child.get("version")
                    date = child.get("maxDate")
                    doc = Doc(self.session, id=href, version=ver, before=date)
                else:
                    doc = Doc(self.session, xml=self.get_node_text(child))
            elif child.tag == "Filter":
                href = child.get("href")
                name = child.get("Name")
                if href:
                    filters.append(href)
                elif name:
                    filters.append("name:{}".format(name))
                else:
                    opts["filter"] = self.get_node_text(child)
                    raise Exception("filter without ID or name")
            elif child.tag == "FilterSet":
                filters.append("set:{}".format(child.get("Name")))
            elif child.tag == "Parms":
                for parm in child.findall("Parm"):
                    name = self.get_node_text(parm.find("Name"), "")
                    value = self.get_node_text(parm.find("Value"), "")
                    parms[name] = value
        if not doc:
            raise Exception("nothing to filter")
        if parms:
            opts["parms"] = parms
        result = doc.filter(*filters, **opts)
        response = etree.Element(node.tag + "Resp")
        if output:
            doc = str(result.result_tree)
            etree.SubElement(response, "Document").text = etree.CDATA(doc)
        if result.messages:
            messages = etree.SubElement(response, "Messages")
            for message in result.messages:
                etree.SubElement(messages, "message").text = message
        return response

    def _get_action(self, node):
        name = self.get_node_text(node.find("Name"), "").strip()
        if not name:
            raise Exception("Missing action name")
        action = self.session.get_action(name)
        if not action:
            raise Exception("Action not found: {}".format(name))
        flag = action.doctype_specific
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "Name").text = action.name
        etree.SubElement(response, "DoctypeSpecific").text = flag
        etree.SubElement(response, "Comment").text = action.comment or ""
        return response

    def _get_css_files(self, node):
        files = Doctype.get_css_files(self.session)
        response = etree.Element(node.tag + "Resp")
        for name in sorted(files):
            data = base64.encodebytes(files[name]).decode("ascii")
            wrapper = etree.SubElement(response, "File")
            etree.SubElement(wrapper, "Name").text = name
            etree.SubElement(wrapper, "Data").text = data
        return response

    def _get_doc(self, node):
        doc_id = self.get_node_text(node.find("DocId"))
        version = self.get_node_text(node.find("DocVersion"))
        denormalize = self.get_node_text(node.find("DenormalizeLinks"), "Y")
        lock = self.get_node_text(node.find("Lock")) == "Y"
        doc = Doc(self.session, id=doc_id, version=version)
        if lock:
            doc.check_out()
        opts = {
            "get_xml": node.get("includeXml", "Y") == "Y",
            "get_blob": node.get("includeBlob", "Y") == "Y",
            "denormalize": denormalize != "N"
        }
        response = etree.Element(node.tag + "Resp")
        response.append(doc.legacy_doc(**opts))
        return response

    def _get_doctype(self, node):
        name = node.get("Type")
        include_values = node.get("GetEnumValues") == "Y"
        include_dtd = node.get("OmitDtd") != "Y"
        self.logger.info("_get_doctype(): name=%s", name)
        doctype = Doctype(self.session, name=name)
        response = etree.Element(node.tag + "Resp")
        response.set("Type", doctype.name)
        response.set("Format", doctype.format)
        response.set("Versioning", doctype.versioning)
        response.set("Created", str(doctype.created))
        response.set("SchemaMod", str(doctype.schema_mod))
        response.set("Active", doctype.active)
        if include_dtd:
            dtd = etree.CDATA(doctype.dtd)
            etree.SubElement(response, "DocDtd").text = dtd
        etree.SubElement(response, "DocSchema").text = doctype.schema
        if include_values:
            vv_lists = doctype.vv_lists
            for set_name in sorted(vv_lists):
                wrapper = etree.SubElement(response, "EnumSet", Node=set_name)
                for value in vv_lists[set_name]:
                    etree.SubElement(wrapper, "ValidValue").text = value
        linking_elements = doctype.linking_elements
        if linking_elements:
            wrapper = etree.SubElement(response, "LinkingElements")
            for name in sorted(linking_elements):
                etree.SubElement(wrapper, "LinkingElement").text = name
        if doctype.comment is not None:
            etree.SubElement(response, "Comment").text = doctype.comment
        return response

    def _get_filters(self, node):
        response = etree.Element(node.tag + "Resp")
        for doc in FilterSet.get_filters(self.session):
            node = etree.SubElement(response, "Filter", DocId=doc.cdr_id)
            node.text = doc.title
        return response

    def _get_filter_set(self, node):
        name = self.get_node_text(node.find("FilterSetName"))
        filter_set = FilterSet(self.session, name=name)
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "FilterSetName").text = filter_set.name
        if filter_set.description is not None:
            child = etree.SubElement(response, "FilterSetDescription")
            child.text = filter_set.description
        if filter_set.notes is not None:
            child = etree.SubElement(response, "FilterSetNotes")
            child.text = filter_set.notes
        for m in filter_set.members:
            if isinstance(m, Doc):
                child = etree.SubElement(response, "Filter", DocId=m.cdr_id)
                child.text = m.title
            else:
                set_id = str(m.id)
                child = etree.SubElement(response, "FilterSet", SetId=set_id)
                child.text = m.name
        return response

    def _get_filter_sets(self, node):
        response = etree.Element(node.tag + "Resp")
        for id, name in FilterSet.get_filter_sets(self.session):
            etree.SubElement(response, "FilterSet", SetId=str(id)).text = name
        return response

    def _get_glossary_map(self, node):
        lang = "en" if node.tag == "CdrGetGlossaryMap" else "es"
        dictionary = self.get_node_text(node.find("Dictionary"), "").strip()
        audience = self.get_node_text(node.find("Audience"), "").strip()
        response = etree.Element(node.tag + "Resp")
        opts = dict(language=lang, dictionary=dictionary, audience=audience)
        for doc in GlossaryTermName.get_mappings(self.session, **opts):
            wrapper = etree.SubElement(response, "Term", id=str(doc.id))
            etree.SubElement(wrapper, "Name").text = doc.name
            for phrase in doc.phrases:
                etree.SubElement(wrapper, "Phrase").text = phrase
        return response

    def _get_group(self, node):
        name = self.get_node_text(node.find("GrpName"), "").strip()
        if not name:
            raise Exception("Missing group name")
        group = self.session.get_group(name)
        if not group:
            raise Exception("Group not found: {}".format(name))
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "GrpName").text = group.name
        etree.SubElement(response, "GrpId").text = str(group.id)
        for action in sorted(group.actions):
            doctypes = group.actions[action] or [""]
            for doctype in doctypes:
                auth = etree.SubElement(response, "Auth")
                etree.SubElement(auth, "Action").text = action
                if doctype:
                    etree.SubElement(auth, "DocType").text = doctype
        for user in group.users:
            etree.SubElement(response, "UserName").text = user
        if group.comment:
            etree.SubElement(response, "Comment").text = group.comment
        return response

    def _get_link_type(self, node):
        name = self.get_node_text(node.find("Name"), "").strip()
        if not name:
            raise Exception("Missing link type name")
        linktype = LinkType(self.session, name=name)
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "Name").text = linktype.name
        etree.SubElement(response, "LinkChkType").text = linktype.chk_type
        if linktype.comment is not None:
            comment = linktype.comment
            etree.SubElement(response, "LinkTypeComment").text = comment
        sources = linktype.sources or []
        properties = linktype.properties or []
        for source in sources:
            wrapper = etree.SubElement(response, "LinkSource")
            etree.SubElement(wrapper, "SrcDocType").text = source.doctype.name
            etree.SubElement(wrapper, "SrcField").text = source.element
            for target in linktype.targets.values():
                etree.SubElement(response, "TargetDocType").text = target.name
        for prop in properties:
            wrapper = etree.SubElement(response, "LinkProperties")
            etree.SubElement(wrapper, "LinkProperty").text = prop.name
            etree.SubElement(wrapper, "PropertyValue").text = prop.value
            etree.SubElement(wrapper, "PropertyComment").text = prop.comment
        return response

    def _get_links(self, node):
        doc_id = self.get_node_text(node.find("DocId"))
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "DocID").text = doc_id
        wrapper = etree.SubElement(response, "LnkList")
        links = Doc(self.session, id=doc_id).link_report()
        etree.SubElement(wrapper, "LnkCount").text = str(len(links))
        for link in links:
            etree.SubElement(wrapper, "LnkItem").text = link
        return response

    def _get_user(self, node):
        name = self.get_node_text(node.find("UserName"))
        user = Session.User(self.session, name=name)
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "UserName").text = user.name
        etree.SubElement(response, "AuthenticationMode").text = user.authmode
        if user.fullname is not None:
            etree.SubElement(response, "FullName").text = user.fullname
        if user.office is not None:
            etree.SubElement(response, "Office").text = user.office
        if user.email is not None:
            etree.SubElement(response, "Email").text = user.email
        if user.phone is not None:
            etree.SubElement(response, "Phone").text = user.phone
        if user.comment is not None:
            etree.SubElement(response, "Comment").text = user.comment
        for group in (user.groups or []):
            etree.SubElement(response, "GrpName").text = group
        return response

    def _get_tree(self, node):
        response = etree.Element(node.tag + "Resp")
        doc_id = self.get_node_text(node.find("DocId"))
        depth = int(self.get_node_text(node.find("ChildDepth")))
        tree = Doc(self.session, id=doc_id).get_tree(depth)
        pairs = etree.SubElement(response, "Pairs")
        for relationship in tree.relationships:
            wrapper = etree.SubElement(pairs, "Pair")
            etree.SubElement(wrapper, "Child").text = str(relationship.child)
            etree.SubElement(wrapper, "Parent").text = str(relationship.parent)
        terms = etree.SubElement(response, "Terms")
        for term_id in tree.names:
            wrapper = etree.SubElement(terms, "Term")
            etree.SubElement(wrapper, "Id").text = str(term_id)
            etree.SubElement(wrapper, "Name").text = tree.names[term_id]
        return response

    def _label_doc_version(self, node):
        doc_id = self.get_node_text(node.find("DocumentId"))
        version = self.get_node_text(node.find("DocumentVersion"))
        label = self.get_node_text(node.find("LabelName"))
        Doc(self.session, id=doc_id, version=version).label(label)
        return etree.Element(node.tag + "Resp")

    def _last_versions(self, node):
        response = etree.Element(node.tag + "Resp")
        doc_id = self.get_node_text(node.find("DocId"))
        doc = Doc(self.session, id=doc_id)
        last_version = str(doc.last_version or -1)
        last_pub_ver = str(doc.last_publishable_version or -1)
        changed = doc.has_unversioned_changes and "Y" or "N"
        etree.SubElement(response, "LastVersionNum").text = last_version
        etree.SubElement(response, "LastPubVersionNum").text = last_pub_ver
        etree.SubElement(response, "IsChanged").text = changed
        return response

    def _list_actions(self, node):
        response = etree.Element(node.tag + "Resp")
        for action in self.session.list_actions():
            flag = action.doctype_specific
            action_node = etree.SubElement(response, "Action")
            etree.SubElement(action_node, "Name").text = action.name
            etree.SubElement(action_node, "NeedDoctype").text = flag
        return response

    def _list_doctypes(self, node):
        response = etree.Element(node.tag + "Resp")
        for name in Doctype.list_doc_types(self.session):
            etree.SubElement(response, "DocType").text = name
        return response

    def _list_groups(self, node):
        response = etree.Element(node.tag + "Resp")
        for name in self.session.list_groups():
            etree.SubElement(response, "GrpName").text = name
        return response

    def _list_linkprops(self, node):
        response = etree.Element(node.tag + "Resp")
        for prop_type in LinkType.get_property_types(self.session):
            wrapper = etree.SubElement(response, "LinkProperty")
            etree.SubElement(wrapper, "Name").text = prop_type.name
            if prop_type.comment is not None:
                etree.SubElement(wrapper, "Comment").text = prop_type.comment
        return response

    def _list_linktypes(self, node):
        response = etree.Element(node.tag + "Resp")
        for name in LinkType.get_linktype_names(self.session):
            etree.SubElement(response, "Name").text = name
        return response

    def _list_query_term_defs(self, node):
        response = etree.Element(node.tag + "Resp")
        for definition in QueryTermDef.get_definitions(self.session):
            wrapper = etree.SubElement(response, "Definition")
            etree.SubElement(wrapper, "Path").text = definition.path
            if definition.rule is not None:
                etree.SubElement(wrapper, "Rule").text = definition.rule
        return response

    def _list_query_term_rules(self, node):
        response = etree.Element(node.tag + "Resp")
        for rule in QueryTermDef.get_rules(self.session):
            etree.SubElement(response, "Rule").text = rule
        return response

    def _list_schema_docs(self, node):
        response = etree.Element(node.tag + "Resp")
        for title in Doctype.list_schema_docs(self.session):
            etree.SubElement(response, "DocTitle").text = title
        return response

    def _list_users(self, node):
        response = etree.Element(node.tag + "Resp")
        for name in self.session.list_users():
            etree.SubElement(response, "UserName").text = name
        return response

    def _list_versions(self, node):
        response = etree.Element(node.tag + "Resp")
        doc_id = self.get_node_text(node.find("DocId"))
        limit = self.get_node_text(node.find("NumVersions"))
        if limit is not None:
            limit = int(limit)
            if limit < 0:
                limit = None
        versions = Doc(self.session, id=doc_id).list_versions(limit)
        for version in versions:
            wrapper = etree.SubElement(response, "Version")
            etree.SubElement(wrapper, "Num").text = str(version.number)
            etree.SubElement(wrapper, "Date").text = str(version.saved)
            if version.comment is not None:
                etree.SubElement(wrapper, "Comment").text = version.comment
        return response

    def _log_client_event(self, node):
        response = etree.Element(node.tag + "Resp")
        description = self.get_node_text(node.find("EventDescription"))
        self.session.log_client_event(description)
        return response

    def _logoff(self, node):
        self.session.logout()
        return etree.Element(node.tag + "Resp")

    def _mailer_cleanup(self, node):
        response = etree.Element(node.tag + "Resp")
        report = Doc.delete_failed_mailers(self.session)
        for doc_id in report.deleted:
            doc = Doc(self.session, id=doc_id)
            etree.SubElement(response, "DeletedDoc").text = doc.cdr_id
        if report.errors:
            wrapper = etree.SubElement(response, "Errors")
            for error in report.errors:
                etree.SubElement(wrapper, "Err").text = str(error)
        return response

    def _mod_action(self, node):
        name = self.get_node_text(node.find("Name"))
        new_name = self.get_node_text(node.find("NewName"))
        flag = self.get_node_text(node.find("DoctypeSpecific"))
        comment = self.get_node_text(node.find("Comment"))
        action = self.session.get_action(name)
        if new_name:
            action.name = new_name
        action.doctype_specific = flag
        action.comment = comment
        action.modify(self.session)
        return etree.Element(node.tag + "Resp")

    def _mod_group(self, node):
        name = new_name = comment = None
        users = []
        actions = {}
        for child in node.findall("*"):
            if child.tag == "GrpName":
                name = child.text.strip()
            elif child.tag == "NewGrpName":
                new_name = child.text.strip()
            elif child.tag == "Comment":
                comment = child.text
            elif child.tag == "UserName":
                users.append(child.text)
            elif child.tag == "Auth":
                action = self.get_node_text(child.find("Action"), "").strip()
                doctype = self.get_node_text(child.find("DocType"), "").strip()
                if not action:
                    raise Exception("Missing Action element in auth")
                if action not in actions:
                    actions[action] = []
                actions[action].append(doctype)
        if not name:
            raise Exception("Missing group name")
        group = self.session.get_group(name)
        if new_name:
            group.name = new_name
        group.comment = comment
        group.users = users
        group.actions = actions
        group.modify(self.session)
        return etree.Element(node.tag + "Resp")

    def _paste_link(self, node):
        source_type = self.get_node_text(node.find("SourceDocType"))
        element = self.get_node_text(node.find("SourceElementType"))
        target = self.get_node_text(node.find("TargetDocId"))
        doctype = Doctype(self.session, name=source_type)
        message = "Link from {} elements of {} documents"
        tail = " not permitted"
        link_type = LinkType.lookup(self.session, doctype, element)
        if link_type is None:
            raise Exception(message.format(element, source_type) + tail)
        doc = Doc(self.session, id=target)
        if doc.doctype.id not in link_type.targets:
            message += " to document {}" + tail
            raise Exception(message.format(element, source_type, doc.cdr_id))
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "DenormalizedContent").text = doc.title
        return response

    def _publish(self, node):
        opts = dict(
            system=self.get_node_text(node.find("PubSystem")),
            subsystem=self.get_node_text(node.find("PubSubset")),
            email=self.get_node_text(node.find("Email")),
            no_output=self.get_node_text(node.find("NoOutput")) == "Y",
            permissive=self.get_node_text(node.find("AllowNonPub")) == "Y",
            force=self.get_node_text(node.find("AllowInActive")) == "Y"
        )
        parms = dict()
        for wrapper in node.findall("Parms/Parm"):
            name = self.get_node_text(wrapper.find("Name"))
            value = self.get_node_text(wrapper.find("Value"))
            parms[name] = value
        opts["parms"] = parms
        docs = []
        for doc in node.findall("DocList/Doc"):
            doc_id = doc.get("Id")
            version = doc.get("Version")
            docs.append(Doc(self.session, id=doc_id, version=version))
        opts["docs"] = docs
        job_id = Job(self.session, **opts).create()
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "JobId").text = str(job_id)
        return response

    def _put_doc(self, node):
        new = node.tag == "CdrAddDoc"
        opts = dict()
        doc_opts = dict()
        doc_node = None
        echo = False
        for child in node.findall("*"):
            if child.tag == "CheckIn":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["unlock"] = True
            elif child.tag == "Version":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["version"] = True
                    if child.get("Publishable", "").upper() == "Y":
                        opts["publishable"] = True
            elif child.tag == "Validate":
                if self.get_node_text(child, "").upper() == "Y":
                    val_types = child.get("ValidationTypes", "").lower()
                    if val_types:
                        opts["val_types"] = val_types.split()
                    else:
                        opts["val_types"] = "schema", "links"
                    if child.get("ErrorLocators", "").upper() == "Y":
                        opts["locators"] = True
            elif child.tag == "SetLinks":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["set_links"] = True
            elif child.tag == "Echo":
                if self.get_node_text(child, "").upper() == "Y":
                    echo = True
            elif child.tag == "DelAllBlobVersions":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["del_blobs"] = True
            elif child.tag == "Reason":
                opts["reason"] = self.get_node_text(child)
            elif child.tag == "CdrDoc":
                doc_node = child
        if doc_node is None:
            raise Exception("put_doc() missing CdrDoc element")
        doc_opts = dict(doctype=doc_node.get("Type"))
        if doc_node.get("Id"):
            doc_opts["id"] = doc_node.get("Id")
        for child in doc_node.findall("*"):
            if child.tag == "CdrDocCtl":
                for ctl_node in child.findall("*"):
                    if ctl_node.tag == "DocId" and not doc_opts.get("id"):
                        doc_opts["id"] = self.get_node_text(ctl_node)
                    elif ctl_node.tag == "DocTitle":
                        opts["title"] = self.get_node_text(ctl_node)
                    elif ctl_node.tag == "DocComment":
                        opts["comment"] = self.get_node_text(ctl_node)
                    elif ctl_node.tag == "DocActiveStatus":
                        opts["active_status"] = self.get_node_text(ctl_node)
                    elif ctl_node.tag == "DocNeedsReview":
                        needs_review = self.get_node_text(ctl_node, "N")
                        if needs_review.upper() == "Y":
                            opts["needs_review"] = True
            elif child.tag == "CdrDocXml":
                doc_opts["xml"] = self.get_node_text(child)
            elif child.tag == "CdrDocBlob":
                blob_base64_chars = self.get_node_text(child).encode("ascii")
                doc_opts["blob"] = base64.decodebytes(blob_base64_chars)
        if new and doc_opts.get("id"):
            raise Exception("can't add a document which already has an ID")
        if not new and not doc_opts.get("id"):
            raise Exception("CdrRepDoc missing document ID")
        doc = Doc(self.session, **doc_opts)
        doc.save(**opts)
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "DocId").text = doc.cdr_id
        doc_included = False
        legacy_opts = dict(get_xml=True, brief=True, denormalize=True)
        if doc.errors_node is not None:
            response.append(doc.errors_node)
            if doc.is_content_type and opts.get("locators"):
                legacy_opts["locators"] = True
                response.append(doc.legacy_doc(**legacy_opts))
                doc_included = True
        if echo and not doc_included:
            response.append(doc.legacy_doc(**legacy_opts))
        return response

    def _put_doctype(self, node):
        opts = {
            "name": node.get("Type"),
            "format": node.get("Format") or "xml",
            "versioning": node.get("Versioning") or "Y",
            "active": node.get("Active") or "Y"
        }
        for child in node.findall("*"):
            if child.tag == "DocSchema":
                opts["schema"] = self.get_node_text(child)
            elif child.tag == "Comment":
                opts["comment"] = self.get_node_text(child)
        doctype = Doctype(self.session, **opts)
        doctype.save()
        return etree.Element(node.tag + "Resp")

    def _put_filter_set(self, node):
        new = node.tag == "CdrAddFilterSet"
        opts = dict(name=None, description=None, notes=None, members=[])
        for child in node:
            if child.tag == "FilterSetName":
                opts["name"] = self.get_node_text(child)
            elif child.tag == "FilterSetDescription":
                opts["description"] = self.get_node_text(child)
            elif child.tag == "FilterSetNotes":
                opts["notes"] = self.get_node_text(child)
            elif child.tag == "Filter":
                member = Doc(self.session, id=child.get("DocId"))
                opts["members"].append(member)
            elif child.tag == "FilterSet":
                member = FilterSet(self.session, id=child.get("SetId"))
                opts["members"].append(member)
        filter_set = FilterSet(self.session, **opts)
        if new and filter_set.id:
            message = "Filter set {!r} already exists".format(filter_set.name)
            raise Exception(message)
        if not new and not filter_set.id:
            message = "Filter set {!r} not found".format(filter_set.name)
            raise Exception(message)
        member_count = filter_set.save()
        name = node.tag + "Resp"
        return etree.Element(name, TotalFilters=str(member_count))

    def _put_link_type(self, node):
        opts = dict(
            sources=[],
            targets={},
            properties=[],
            chk_type=self.get_node_text(node.find("LinkChkType")),
            comment=self.get_node_text(node.find("Comment"))
        )
        name = self.get_node_text(node.find("Name"))
        if node.tag == "CdrModLinkType":
            linktype_id = LinkType(self.session, name=name).id
            if linktype_id is None:
                raise Exception("Can't find link type {}".format(name))
            opts["id"] = linktype_id
            new_name = self.get_node_text(node.find("NewName"))
            opts["name"] = new_name if new_name else name
        else:
            opts["name"] = name
        for wrapper in node.findall("LinkSource"):
            doctype_name = self.get_node_text(wrapper.find("SrcDocType"))
            element = self.get_node_text(wrapper.find("SrcField"))
            doctype = Doctype(self.session, name=doctype_name)
            opts["sources"].append(LinkType.LinkSource(doctype, element))
        for child in node.findall("TargetDocType"):
            doctype_name = self.get_node_text(child)
            doctype = Doctype(self.session, name=doctype_name)
            opts["targets"][doctype.id] = doctype
        message = "Property type {!r} not supported"
        for wrapper in node.findall("LinkProperties"):
            name = self.get_node_text(wrapper.find("LinkProperty"))
            value = self.get_node_text(wrapper.find("PropertyValue"))
            comment = self.get_node_text(wrapper.find("Comment"))
            try:
                cls = getattr(LinkType, name)
                property = cls(self.session, name, value, comment)
                if not isinstance(property, LinkType.Property):
                    raise Exception(message.format(name))
                opts["properties"].append(property)
            except Exception:
                raise Exception(message.format(name))
        linktype = LinkType(self.session, **opts)
        linktype.save()
        return etree.Element(node.tag + "Resp")

    def _put_user(self, node):
        opts = dict(
            name=self.get_node_text(node.find("UserName")),
            fullname=self.get_node_text(node.find("FullName")),
            office=self.get_node_text(node.find("Office")),
            email=self.get_node_text(node.find("Email")),
            phone=self.get_node_text(node.find("Phone")),
            comment=self.get_node_text(node.find("Comment")),
            authmode=self.get_node_text(node.find("AuthenticationMode")),
            groups=[]
        )
        if node.tag == "CdrModUsr":
            opts["id"] = Session.User(self.session, name=opts["name"]).id
            new_name = self.get_node_text(node.find("NewName"))
            if new_name:
                opts["name"] = new_name
        for group in node.findall("GrpName"):
            opts["groups"].append(self.get_node_text(group))
        password = self.get_node_text(node.find("Password"))
        Session.User(self.session, **opts).save(password)
        return etree.Element(node.tag + "Resp")

    def _reindex_doc(self, node):
        doc_id = self.get_node_text(node.find("DocId"))
        Doc(self.session, id=doc_id).reindex()
        return etree.Element(node.tag + "Resp")

    def _report(self, node):
        name = self.get_node_text(node.find("ReportName"))
        params = {}
        for param in node.findall("ReportParams/ReportParam"):
            params[param.get("Name")] = param.get("Value")
        response = etree.Element(node.tag + "Resp")
        response.append(Report(self.session, name, **params).run())
        return response

    def _save_client_trace_log(self, node):
        log_data = self.get_node_text(node.find("LogData"))
        opts = {}
        opts["user"] = self.get_node_text(node.find("User"))
        opts["session"] = self.get_node_text(node.find("Session"))
        log_id = self.session.save_client_trace_log(log_data, **opts)
        response = etree.Element(node.tag + "Resp")
        etree.SubElement(response, "LogId").text = str(log_id)
        return response

    def _search(self, node):
        query = node.find("Query")
        if query is None:
            raise Exception("Missing required Query")
        limit = query.get("MaxDocs")
        if limit:
            limit = int(limit)
        doctypes = [self.get_node_text(n) for n in query.findall("DocType")]
        tests = [self.get_node_text(n) for n in query.findall("Test")]
        search = Search(self.session, *tests, doctypes=doctypes, limit=limit)
        docs = search.run()
        response = etree.Element(node.tag + "Resp")
        results = etree.SubElement(response, "QueryResults")
        for doc in docs:
            wrapper = etree.SubElement(results, "QueryResult")
            etree.SubElement(wrapper, "DocId").text = doc.cdr_id
            etree.SubElement(wrapper, "DocType").text = doc.doctype.name
            etree.SubElement(wrapper, "DocTitle").text = doc.title
        return response

    def _search_links(self, node):
        limit = node.get("MaxDocs")
        limit = int(limit) if limit else None
        pattern = self.get_node_text(node.find("TargetTitlePattern"))
        element = self.get_node_text(node.find("SourceElementType"))
        type_name = self.get_node_text(node.find("SourceDocType"))
        doc_type = Doctype(self.session, name=type_name)
        link_type = LinkType.lookup(self.session, doc_type, element)
        if link_type is None:
            raise Exception("No links permitted from this element")
        response = etree.Element(node.tag + "Resp")
        results = etree.SubElement(response, "QueryResults")
        opts = dict(pattern=pattern, limit=limit)
        for doc in link_type.search(**opts):
            wrapper = etree.SubElement(results, "QueryResult")
            etree.SubElement(wrapper, "DocId").text = doc.cdr_id
            etree.SubElement(wrapper, "DocTitle").text = doc.title
        return response

    def _set_ctl(self, node):
        action = self.get_node_text(node.find("Ctl/Action"))
        group = self.get_node_text(node.find("Ctl/Group"))
        name = self.get_node_text(node.find("Ctl/Key"))
        if action == "Create":
            value = self.get_node_text(node.find("Ctl/Value"))
            comment = self.get_node_text(node.find("Ctl/Comment"))
            args = self.session, group, name, value
            opts = dict(comment=comment)
            Tier.set_control_value(*args, **opts)
        elif action == "Inactivate":
            Tier.inactivate_control_value(self.session, group, name)
        elif action != "Install":
            raise Exception("Invalid action {!r}".format(action))
        return etree.Element(node.tag + "Resp")

    def _set_doc_status(self, node):
        doc_id = self.get_node_text(node.find("DocId"))
        status = self.get_node_text(node.find("NewStatus"))
        comment = self.get_node_text(node.find("Comment"))
        Doc(self.session, id=doc_id).set_status(status, comment=comment)
        return etree.Element(node.tag + "Resp")

    def _unlabel_doc_version(self, node):
        doc_id = self.get_node_text(node.find("DocumentId"))
        label = self.get_node_text(node.find("LabelName"))
        Doc(self.session, id=doc_id).unlabel(label)
        return etree.Element(node.tag + "Resp")

    def _update_title(self, node):
        doc_id = self.get_node_text(node.find("DocId"))
        result = Doc(self.session, id=doc_id).update_title()
        response = etree.Element(node.tag + "Resp")
        response.text = "changed" if result else "unchanged"
        return response

    def _validate_doc(self, node):
        doctype = node.get("DocType")
        types = node.get("ValidationTypes", "").lower().split()
        opts = {
            "store": "never",
            "types": types or ("schema", "links"),
            "locators": node.get("ErrorLocators") in ("Y", "y")
        }
        xml = doc_id = None
        for child in node:
            if child.tag == "CdrDoc":
                doc_id = self.get_node_text(child.find("CdrDocCtl/DocId"))
                xml = self.get_node_text(child.find("CdrDocXml"))
                level = child.get("RevisionFilterLevel")
                if level and level.isdigit():
                    opts["level"] = int(level)
            elif child.tag == "DocId":
                doc_id = self.get_node_text(child)
                if not child.get("ValidateOnly") != "Y":
                    opts["store"] = "always"
        if not doctype:
            raise Exception("Missing required DocType element")
        if xml:
            doc = Doc(self.session, xml=xml, doctype=doctype, id=doc_id)
        elif doc_id:
            doc = Doc(self.session, id=doc_id)
            if doc.doctype.name != doctype:
                raise Exception("DocType mismatch")
        else:
            raise Exception("Must specify DocId or CdrDoc element")
        doc.validate(**opts)
        return doc.legacy_validation_response(opts["locators"])
