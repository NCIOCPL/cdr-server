"""
HTTPS interface for handling CDR client-server requests

Pulled out separate from the main CGI script for pre-compilation.
"""

import base64
import datetime
import os
import sys
from lxml import etree
from cdrapi.users import Session
from cdrapi.settings import Tier
from cdrapi.docs import Doc, Doctype


try:
    basestring
except:
    basestring = unicode = str


class CommandSet:
    PARSER = etree.XMLParser(strip_cdata=False)
    COMMANDS = dict(
        CdrAddAction="_add_action",
        CdrAddDoc="_add_doc",
        CdrAddDocType="_add_doctype",
        CdrAddGrp="_add_group",
        CdrCanDo="_can_do",
        CdrDelAction="_del_action",
        CdrDelDoc="_del_doc",
        CdrDelDocType="_del_doctype",
        CdrDelGrp="_del_group",
        CdrDupSession="_dup_session",
        CdrFilter="_filter_doc",
        CdrGetAction="_get_action",
        CdrGetCssFiles="_get_css_files",
        CdrGetDoc="_get_doc",
        CdrGetDocType="_get_doctype",
        CdrGetGrp="_get_group",
        CdrListDocTypes="_list_doctypes",
        CdrListSchemaDocs="_list_schema_docs",
        CdrListGrps="_list_groups",
        CdrLogoff="_logoff",
        CdrListActions="_list_actions",
        CdrModDocType="_mod_doctype",
        CdrModGrp="_mod_group",
        CdrRepAction="_mod_action",
        CdrRepDoc="_rep_doc",
        CdrValidateDoc="_validate_doc",
    )

    def __init__(self):
        self.tier = Tier()
        self.level = os.environ.get("HTTP_X_LOGGING_LEVEL", "INFO")
        self.logger = self.tier.get_logger("https_api", level=self.level)
        self.start = datetime.datetime.now()
        self.session = None
        self.client = os.environ.get("REMOTE_ADDR")
        self.root = self.__load_commands()

    def get_responses(self):
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
                except:
                    self.logger.exception("{} failure".format(node.tag))
            else:
                self.logger.warning("unexpected element %r", node.tag)
        return self.__wrap_responses(*responses)


    #------------------------------------------------------------------
    # MAPPED COMMAND METHODS START HERE.
    #------------------------------------------------------------------

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
        return etree.Element("CdrAddActionResp")

    def _add_doc(self, node):
        try:
            return self.__put_doc(node, new=True)
        except:
            self.logger.exception("_add_doc()")
            raise

    def _add_doctype(self, node):
        return self.__put_doctype(node, new=True)

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
        return etree.Element("CdrAddGrpResp")

    def _can_do(self, node):
        action = doc_type = None
        for child in node.findall("*"):
            if child.tag == "Action":
                action = child.text.strip()
            elif child.tag == "DocType":
                doc_type = child.text.strip()
        if not action:
            raise Exception("No action specified to check")
        response = etree.Element("CdrCanDoResp")
        response.text = "Y" if self.session.can_do(action, doc_type) else "N"
        return response

    def _del_action(self, node):
        name = self.get_node_text(node.find("Name"), "").strip()
        if not name:
            raise Exception("Missing action name")
        self.session.get_action(name).delete(self.session)
        return etree.Element("CdrDelActionResp")

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
        response = etree.Element("CdrDelDocResp")
        etree.SubElement(response, "DocId").text = doc.cdr_id
        if doc.errors:
            response.append(doc.errors_node)
        return response

    def _del_doctype(self, node):
        name = node.get("Type")
        if not name:
            raise Exception("Missing doctype name")
        Doctype(self.session, name=name).delete()
        return etree.Element("CdrDelDocTypeResp")

    def _del_group(self, node):
        name = self.get_node_text(node.find("GrpName"), "").strip()
        if not name:
            raise Exception("Missing group name")
        self.session.get_group(name).delete(self.session)
        return etree.Element("CdrDelGrpResp")

    def _dup_session(self, node):
        session = self.session.duplicate()
        response = etree.Element("CdrDupSessionResp")
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
                    doc = Doc(self.session, xml=self.get_node_text(doc_node))
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
            elif child.tag == "Parm":
                name = self.get_node_text(child.find("Name"), "")
                value = self.get_node_text(child.find("Value"), "")
                parms[name] = value
        if not doc:
            raise Exception("nothing to filter")
        result = doc.filter(*filters, **opts)
        response = etree.Element("CdrFilterResp")
        if output:
            doc = unicode(result.result_tree)
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
        response = etree.Element("CdrGetActionResp")
        etree.SubElement(response, "Name").text = action.name
        etree.SubElement(response, "DoctypeSpecific").text = flag
        etree.SubElement(response, "Comment").text = action.comment or ""
        return response

    def _get_css_files(self, node):
        files = Doctype.get_css_files(self.session)
        response = etree.Element("CdrGetCssFiles")
        for name in sorted(files):
            wrapper = etree.SubElement(response, "File")
            etree.SubElement(wrapper, "Name").text = name
            etree.SubElement(wrapper, "Data").text = files[name]
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
        response = etree.Element("CdrGetDocResp")
        response.append(doc.legacy_doc(**opts))
        return response

    def _get_doctype(self, node):
        name = node.get("Type")
        include_values = node.get("GetEnumValues") == "Y"
        include_dtd = node.get("OmitDtd") != "Y"
        self.logger.info("_get_doctype(): name=%s", name)
        doctype = Doctype(self.session, name=name)
        response = etree.Element("CdrGetDocTypeResp")
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

    def _get_group(self, node):
        name = self.get_node_text(node.find("GrpName"), "").strip()
        if not name:
            raise Exception("Missing group name")
        group = self.session.get_group(name)
        if not group:
            raise Exception("Group not found: {}".format(name))
        response = etree.Element("CdrGetGrpResp")
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

    def _list_actions(self, node):
        response = etree.Element("CdrListActionsResp")
        for action in self.session.list_actions():
            flag = action.doctype_specific
            action_node = etree.SubElement(response, "Action")
            etree.SubElement(action_node, "Name").text = action.name
            etree.SubElement(action_node, "NeedDoctype").text = flag
        return response

    def _list_doctypes(self, node):
        response = etree.Element("CdrListDocTypesResp")
        for name in Doctype.list_doc_types(self.session):
            etree.SubElement(response, "DocType").text = name
        return response

    def _list_groups(self, node):
        response = etree.Element("CdrListGrpsResp")
        for name in self.session.list_groups():
            etree.SubElement(response, "GrpName").text = name
        return response

    def _list_schema_docs(self, node):
        response = etree.Element("CdrListSchemaDocsResp")
        for title in Doctype.list_schema_docs(self.session):
            etree.SubElement(response, "DocTitle").text = title
        return response

    def _logoff(self, node):
        self.session.logout()
        return etree.Element("CdrLogoffResp")

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
        return etree.Element("CdrRepActionResp")

    def _mod_doctype(self, node):
        return self.__put_doctype(node, new=False)

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
        return etree.Element("CdrModGrpResp")

    def _rep_doc(self, node):
        return self.__put_doc(node, new=False)

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
                xml = self.get_node_text(child.find("CdrDocXml"))
            elif child.tag == "DocId":
                doc_id = self.get_node_text(child)
                if not child.get("ValidateOnly") != "Y":
                    opts["store"] = "always"
        if not doctype:
            raise Exception("Missing required DocType element")
        if not xml and not doc_id:
            raise Exception("Must specify DocId or CdrDoc element")
        if xml:
            doc = Doc(self.session, xml=xml, doctype=doctype)
        elif doc_id:
            doc = Doc(self.session, id=doc_id)
            if doc.doctype.name != doctype:
                raise Exception("DocType mismatch")
        else:
            raise Exception("Both DocId and CdrDoc specified")
        doc.validate(**opts)
        return doc.legacy_validation_response(opts["locators"])


    #------------------------------------------------------------------
    # HELPER METHODS START HERE.
    #------------------------------------------------------------------

    def __put_doc(self, node, new=False):
        opts = dict()
        doc_opts = dict()
        doc_node = None
        echo = locators = False
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
                        locators = opts["locators"] = True
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
                blob = base64.decodestring(self.get_node_text(child))
                doc_opts["blob"] = blob
        if new and doc_opts.get("id"):
            raise Exception("can't add a document which already has an ID")
        if not new and not doc_opts.get("id"):
            raise Exception("CdrRepDoc missing document ID")
        doc = Doc(self.session, **doc_opts)
        doc.save(**opts)
        name = "CdrAddDocResp" if new else "CdrRepDocResp"
        response = etree.Element(name)
        etree.SubElement(response, "DocId").text = doc.cdr_id
        if doc.errors_node:
            response.append(doc.errors_node)
            if doc.is_content_type and opts.get("locators"):
                response.append(doc.legacy_doc(get_xml=True, brief=True))
        return response

    def __put_doctype(self, node, new=False):
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
        name = "CdrAddDocTypeResp" if new else "CdrModDocTypeResp"
        return etree.Element(name)

    def __load_commands(self):
        content_length = os.environ.get("CONTENT_LENGTH")
        if content_length:
            request = sys.stdin.buffer.read(int(content_length))
        else:
            request = sys.stdin.buffer.read()
        self.logger.info("%s bytes from %s", len(request), self.client)
        root = etree.fromstring(request, parser=self.PARSER)
        if root.tag != "CdrCommandSet":
            raise Exception("not a CDR command set")
        return root

    def __process_command(self, node):
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
            handler = self.COMMANDS.get(child.tag)
            if handler is None:
                raise Exception("Unknown command: {}".format(child.tag))
            response.append(getattr(self, handler)(child))
            response.set("Status", "success")
            self.logger.info(child.tag)
        except Exception as e:
            self.logger.exception("{} failure".format(node.tag))
            response.append(self.__wrap_error(e))
            response.set("Status", "failure")
        elapsed = (datetime.datetime.now() - start).total_seconds()
        response.set("Elapsed", "{:f}".format(elapsed))
        return response

    def __wrap_error(self, error):
        errors = etree.Element("Errors")
        etree.SubElement(errors, "Err").text = str(error)
        return errors

    def __wrap_responses(self, *responses):
        response_set = etree.Element("CdrResponseSet")
        response_set.set("Time", self.start.strftime("%Y-%m-%dT%H:%M:%S.%f"))
        for response in responses:
            response_set.append(response)
        return etree.tostring(response_set, encoding="utf-8")

    @staticmethod
    def get_node_text(node, default=None):
        if node is None:
            return default
        return "".join(node.itertext("*"))
