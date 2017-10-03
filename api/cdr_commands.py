"""
HTTPS interface for handling CDR client-server requests

Pulled out separate from the main CGI script for pre-compilation.
"""

import datetime
import os
import sys
from lxml import etree
from cdrapi.users import Session
from cdrapi.settings import Tier

class CommandSet:
    COMMANDS = dict(
        CdrCanDo="can_do",
        CdrAddGrp="add_group",
        CdrModGrp="mod_group",
        CdrGetGrp="get_group",
        CdrDelGrp="del_group",
        CdrListGrps="list_groups",
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
        opts = dict(name=None, comment=None, users=[], permissions=[])
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
                permission = Session.Group.Permission(action, doctype)
                opts["permissions"].append(permission)
        group = Session.Group(**opts)
        group.add(self.session)
        return etree.Element("CdrAddGrpResp")

    def mod_group(self, node):
        name = new_name = comment = None
        users = []
        permissions = []
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
                permissions.append(Session.Group.Permission(action, doctype))
        if not name:
            raise Exception("Missing group name")
        group = self.session.get_group(name)
        if new_name:
            group.name = new_name
        group.comment = comment
        group.users = users
        group.permissions = permissions
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
        for perm in group.permissions:
            auth = etree.SubElement(response, "Auth")
            etree.SubElement(auth, "Action").text = perm.action
            if perm.doctype:
                etree.SubElement(auth, "DocType").text = perm.doctype
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

    @staticmethod
    def get_node_text(node, default=None):
        if node is None:
            return default
        return "".join(node.itertext("*"))
