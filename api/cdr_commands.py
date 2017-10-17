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
from cdrapi.docs import Doc


try:
    basestring
except:
    basestring = unicode = str

class CommandSet:
    COMMANDS = dict(
        CdrLogoff="logoff",
        CdrDupSession="dup_session",
        CdrCanDo="can_do",
        CdrAddGrp="add_group",
        CdrModGrp="mod_group",
        CdrGetGrp="get_group",
        CdrDelGrp="del_group",
        CdrListGrps="list_groups",
        CdrAddAction="add_action",
        CdrRepAction="mod_action",
        CdrGetAction="get_action",
        CdrDelAction="del_action",
        CdrListActions="list_actions",
        CdrAddDoc="add_doc",
        CdrRepDoc="rep_doc",
        CdrDelDoc="del_doc",
        CdrGetDoc="get_doc",
        CdrFilter="filter_doc",
    )
    def __init__(self):
        self.tier = Tier()
        self.level = os.environ.get("HTTP_X_LOGGING_LEVEL", "INFO")
        self.logger = self.tier.get_logger("https_api", level=self.level)
        self.start = datetime.datetime.now()
        self.session = None
        self.client = os.environ.get("REMOTE_ADDR")
        self.root = self.load_commands()
    def load_commands(self):
        content_length = os.environ.get("CONTENT_LENGTH")
        if content_length:
            request = sys.stdin.buffer.read(int(content_length))
        else:
            request = sys.stdin.buffer.read()
        self.logger.info("%s bytes from %s", len(request), self.client)
        root = etree.fromstring(request)
        if root.tag != "CdrCommandSet":
            raise Exception("not a CDR command set")
        return root
    def get_responses(self):
        responses = []
        for node in self.root.findall("*"):
            if node.tag == "SessionId":
                try:
                    name = node.text.strip()
                    self.session = Session(name)
                except Exception as e:
                    self.logger.warning(str(e))
                    return self.wrap_responses(self.wrap_error(e))
            elif node.tag == "CdrCommand":
                responses.append(self.process_command(node))
            else:
                self.logger.warning("unexpected element {!r}", node.tag)
        return self.wrap_responses(*responses)
    def process_command(self, node):
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
            self.logger.error(str(e))
            response.append(self.wrap_error(e))
            response.set("Status", "failure")
        elapsed = (datetime.datetime.now() - start).total_seconds()
        response.set("Elapsed", "{:f}".format(elapsed))
        return response
    def wrap_responses(self, *responses):
        response_set = etree.Element("CdrResponseSet")
        response_set.set("Time", self.start.strftime("%Y-%m-%dT%H:%M:%S.%f"))
        for response in responses:
            response_set.append(response)
        return etree.tostring(response_set, encoding="utf-8")

    def wrap_error(self, error):
        errors = etree.Element("Errors")
        etree.SubElement(errors, "Err").text = str(error)
        return errors

    def logoff(self, node):
        self.session.logout()
        return etree.Element("CdrLogoffResp")

    def dup_session(self, node):
        session = self.session.duplicate()
        response = etree.Element("CdrDupSessionResp")
        etree.SubElement(response, "SessionId").text = self.session.name
        etree.SubElement(response, "NewSessionId").text = session.name
        return response

    def can_do(self, node):
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

    def add_group(self, node):
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

    def mod_group(self, node):
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

    def get_group(self, node):
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

    def del_group(self, node):
        name = self.get_node_text(node.find("GrpName"), "").strip()
        if not name:
            raise Exception("Missing group name")
        self.session.get_group(name).delete(self.session)
        return etree.Element("CdrDelGrpResp")

    def list_groups(self, node):
        response = etree.Element("CdrListGrpsResp")
        for name in self.session.list_groups():
            etree.SubElement(response, "GrpName").text = name
        return response

    def list_actions(self, node):
        response = etree.Element("CdrListActionsResp")
        for action in self.session.list_actions():
            flag = action.doctype_specific
            action_node = etree.SubElement(response, "Action")
            etree.SubElement(action_node, "Name").text = action.name
            etree.SubElement(action_node, "NeedDoctype").text = flag
        return response

    def add_action(self, node):
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

    def get_action(self, node):
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

    def mod_action(self, node):
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

    def del_action(self, node):
        name = self.get_node_text(node.find("Name"), "").strip()
        if not name:
            raise Exception("Missing action name")
        self.session.get_action(name).delete(self.session)
        return etree.Element("CdrDelActionResp")

    def get_doc(self, node):
        doc_id = self.get_node_text(node.find("DocId"))
        version = self.get_node_text(node.find("DocVersion"))
        denormalize = self.get_node_text(node.find("DenormalizeLinks"), "Y")
        lock = self.get_node_text(node.find("Lock")) == "Y"
        doc = Doc(self.session, id=doc_id, version=version)
        if lock:
            doc.lock()
        opts = {
            "get_xml": node.get("includeXml", "Y") == "Y",
            "get_blob": node.get("includeBlob", "Y") == "Y",
            "denormalize": denormalize != "N"
        }
        response = etree.Element("CdrGetDocResp")
        response.append(doc.legacy_doc(**opts))
        return response

    def add_doc(self, node):
        return self.__put_doc(node, new=True)
    def rep_doc(self, node):
        return self.__put_doc(node, new=False)
    def __put_doc(self, node, new=False):
        opts = {"validate": []}
        doc_node = None
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
                    for val_type in ("schema", "links"):
                        if val_type in val_types:
                            opts["validate"].append(val_type)
                    if child.get("ErrorLocators", "").upper() == "Y":
                        opts["locators"] = True
            elif child.tag == "SetLinks":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["set_links"] = True
            elif child.tag == "Echo":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["echo"] = True
            elif child.tag == "DelAllBlobVersions":
                if self.get_node_text(child, "").upper() == "Y":
                    opts["del_blobs"] = True
            elif child.tag == "Reason":
                opts["reason"] = self.get_node_text(child)
            elif child.tag == "CdrDoc":
                doc_node = child
        if doc_node is None:
            raise Exception("put_doc() missing CdrDoc element")
        opts["doctype"] = node.get("Type")
        opts["filter_level"] = node.get("RevisionFilterLevel")
        doc_id = node.get("Id")
        for child in doc_node.findall("*"):
            if child.tag == "CdrDocCtl":
                for ctl_node in child.findall("*"):
                    if ctl_node.tag == "DocId" and not doc_id:
                        doc_id = self.get_node_text(ctl_node)
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
                opts["xml"] = self.get_node_text(child)
            elif child.tag == "CdrDocBlob":
                opts["blob"] = base64.decodestring(self.get_node_text(child))
        if new and doc_id:
            raise Exception("can't add a document which already has an ID")
        if not new and not doc_id:
            raise Exception("CdrRepDoc missing document ID")
        doc = Doc(self.session, id=doc_id)
        doc.save(**opts)
        name = "CdrAddDocResp" if new else "CdrRepDocResp"
        response = etree.Element(name)
        etree.SubElement(response, "DocId").text = doc.cdr_id
        if doc.validation_errors:
            response.append(doc.validation_errors)
            if doc.is_content_type() and opts["locators"]:
                response.append(doc.legacy_doc(get_xml=True, brief=True))
        return response
    def filter_doc(self, node):
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
            doc = unicode(result.doc)
            etree.SubElement(response, "Document").text = etree.CDATA(doc)
        if result.messages:
            messages = etree.SubElement(response, "Messages")
            for message in result.messages:
                etree.SubElement(messages, "message").text = message
        return response

    @staticmethod
    def get_node_text(node, default=None):
        if node is None:
            return default
        return "".join(node.itertext("*"))
