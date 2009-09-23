import sys, xml.parsers.expat
class Elem:
    def __init__(self, name):
        self.name = name
        self.empty = 1
        self.tagClosed = 0
class Doc:
    def __init__(self):
        self.elems = []
        self.data = '<?xml version="1.0" encoding="UTF-8" ?>\n\n'
    def append(self, data):
        self.data += data
    def popElem(self):
        elem = self.elems[-1]
        self.elems = self.elems[:-1]
        return elem
    def pushElem(self, elem):
        if not self.elems: 
            self.data += "\n"
        elif not self.lastTagClosed():
            self.closeParentStartTag(len(self.elems) == 1 and "\n\n" or "\n")
        self.elems.append(elem)
    def closeParentStartTag(self, newLines = ""):
        if self.elems: 
            lastElem = self.elems[-1]
            lastElem.tagClose = 1
            if lastElem.empty:
                lastElem.empty = 0
                self.data += ">%s" % (newLines)
           
        self.followingComment = 0
    def getIndent(self):
        return 2 * len(self.elems)
    def curElem(self):
        if not self.elems: return None
        return self.elems[-1].name
    def lastTagClosed(self):
        if not self.elems: return 1
        return self.elems[-1].tagClosed
 
def attrSorter(a, b):
    for val in ['name','type','minOccurs','maxOccurs','value']:
        if a == val: return -1
        elif b == val: return 1
    return cmp(a, b)

doc = Doc()

def start_element(name, attrs):
    indent = doc.getIndent()
    doc.pushElem(Elem(name))
    padding = 20 - indent - len(name) - 1
    linesep = ""
    start = "%s<%s" % (indent * ' ', name)
    doc.append(start)
    attrKeys = attrs.keys()
    attrKeys.sort(attrSorter)
    for attr in attrKeys:
        output = "%s%s %-15s = '%s'" % (linesep, padding* ' ', attr, 
                                        attrs[attr])
        doc.append(output)
        padding = 20
        linesep = "\n"

def end_element(name):
    curElem = doc.curElem()
    elem = doc.popElem()
    if elem.name != name: 
        throw ("mismatched end tag %s with start tag %s" % (name, elem.name))
    if elem.empty:
        doc.append("/>\n")
    elif curElem == 'documentation':
        doc.append("</%s>\n" % name)
    else:
        doc.append("%s</%s>\n" % (len(doc.elems) * ' ' * 2, name))
    if len(doc.elems) == 1: doc.append("\n")
def char_data(data):
    if doc.curElem() == 'documentation':
        doc.closeParentStartTag()
        doc.append(data)
def comment(data):
    if not doc.lastTagClosed():
        doc.closeParentStartTag("\n")
    doc.append('<!--%s-->\n' % data)

p = xml.parsers.expat.ParserCreate()

p.StartElementHandler = start_element
p.EndElementHandler = end_element
p.CharacterDataHandler = char_data
p.CommentHandler = comment

p.Parse(sys.stdin.read())
sys.stdout.write(doc.data)
