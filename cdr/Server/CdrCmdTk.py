#----------------------------------------------------------------------
#
#
# Test client (Python Tkinter version) for sending commands to CDR server.
#
# Usage:
#  python CdrCmdTk.py
#
# Example:
#
# A simple Tkinter GUI version primarily for learning
# One difference is that command files don't use the CDRCommandSet
# and CDRLogon because that is supplied by the program.
#
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# Import required packages.
#----------------------------------------------------------------------
import socket, string, struct, sys, re
from Tkinter import *
from tkFileDialog import *
from tkMessageBox import *


#----------------------------------------------------------------------
# The whole application
#----------------------------------------------------------------------

class App:
	def __init__(self,master):
		self.name = ''
		self.pw=''
		self.localhost = 'mmdb2'
		self.port = 2019
		self.wrap = IntVar()

#----------------------------------------------------------------------
# Set up the main dialog
#----------------------------------------------------------------------

	def showDialog(self,master):
		frame = Frame(master)
		frame.pack()

		# User name and password
		self.cdrnamel = Label(frame,text="UserName: ")
		self.cdrnamel.pack(side=LEFT)
		self.cdrname  = Entry(frame,textvariable=self.name)
		self.cdrname.pack(side=LEFT,fill=X)
		self.cdrname.focus()
		self.cdrpwl = Label(frame,text="Password: ")
		self.cdrpwl.pack(side=LEFT)
		self.cdrpw  = Entry(frame,textvariable=self.pw)
		self.cdrpw.pack(side=LEFT,fill=X)
		self.cdrpw.focus()

		# Host and port
		frame = Frame(master)
		frame.pack()
		self.cdrhostl = Label(frame,text="Host: ")
		self.cdrhostl.pack(side=LEFT)
		self.cdrhost  = Entry(frame,textvariable=self.localhost)
		self.cdrhost.insert(0,self.localhost)
		self.cdrhost.pack(side=LEFT,fill=X)

		self.cdrportl = Label(frame,text="Port: ")
		self.cdrportl.pack(side=LEFT)
		self.cdrport  = Entry(frame,textvariable=self.port)
		self.cdrport.insert(0,self.port)
		self.cdrport.pack(side=LEFT,fill=X)

		frame = Frame(master)
		frame.pack()
		self.raw = Checkbutton(frame, text="Set up CommandSet and Logon",variable=self.wrap)
		self.raw.select()
		self.raw.pack(side=LEFT)    		

		# Top window - entry of commands			
		frame = Frame(master)
		frame.pack()
		self.clabel = Label(frame,text="CDR Commands",justify=CENTER,font=("Helvetica",12))
		self.clabel.pack()

		frame = Frame(master)
		frame.pack()
 		self.fbutton = Button(frame,text="Load Command File",fg="blue",command=self.getfile)
		self.fbutton.pack(side=LEFT)

 		self.cbutton = Button(frame,text="Clear Commands",fg="blue",command=self.clearCommand)
		self.cbutton.pack(side=LEFT)

 		self.svbutton = Button(frame,text="Save to File",fg="blue",command=self.savefile)
		self.svbutton.pack(side=LEFT)

 		self.sbutton = Button(frame,text="Send to Server",fg="red",command=self.sendCommands)
		self.sbutton.pack(side=LEFT)
		
		frame = Frame(master)
		frame.pack(expand=YES,fill=BOTH)
		self.sbar = Scrollbar(frame)
		self.backLabel = Text(frame,bg="white", fg="blue")
		self.backLabel.pack(side=LEFT,expand=YES,fill=BOTH)
		self.sbar.pack(side=RIGHT,fill=Y)
		self.backLabel.config(relief=SUNKEN,width=75,height=10,bg="beige")
		self.sbar.config(command=self.backLabel.yview)
		self.backLabel.config(yscrollcommand=self.sbar.set)

		# Bottom window - responses
		frame = Frame(master)
		frame.pack()
		self.clabel = Label(frame,text="CDR Server Response",justify=CENTER,font=("Helvetica",12))
		self.clabel.pack()

		frame = Frame(master)
		frame.pack()
 		self.cbutton = Button(frame,text="Clear Responses",fg="blue",command=self.clearResponses)
		self.cbutton.pack(side=LEFT)

 		self.svbutton = Button(frame,text="Save Responses to File",fg="blue",command=self.saveRfile)
		self.svbutton.pack(side=LEFT)



		frame = Frame(master)
		frame.pack(expand=YES,fill=BOTH)
		self.dsbar = Scrollbar(frame)
		self.backData = Text(frame,bg="white", fg="blue")
		self.backData.pack(side=LEFT,expand=YES,fill=BOTH)
		self.dsbar.pack(side=RIGHT,fill=Y)
		self.backData.config(relief=SUNKEN,width=75,height=15,bg="beige")
		self.dsbar.config(command=self.backData.yview)
		self.backData.config(yscrollcommand=self.dsbar.set)


		frame = Frame(master)
		frame.pack()
		self.qbutton = Button(frame,text="QUIT",fg="red",command=frame.quit)
		self.qbutton.pack(side=RIGHT)

		#old junk
#	def Command(self):
#		self.backLabel.insert("1.0",self.cdrinput.get())

	#---Event handler to clear command window
	def clearCommand(self):
		self.backLabel.delete("1.0",END)		

	#---Event handler to load a command file
	def getfile(self):
		filename = askopenfilename(filetypes=[("CDR command files", ".xml")],
         title="Load CDR Commands")
		if filename:
			file = open(filename, 'r')
			filedata = file.read()
			file.close()
			self.backLabel.insert("1.0",filedata)

	#---Event handler to save a command file
	def savefile(self):
		filename = asksaveasfilename(filetypes=[("CDR command files", ".xml")],
         title="Save CDR Commands")
		if filename:
			if not re.search(".*.xml",filename):
				filename = filename + ".xml"
			print "FILENAME: ",filename						
			file = open(filename, 'w')
			filedata = file.write(self.backLabel.get('1.0',END))
			file.close()

	#---Event handler to save a response file
	def saveRfile(self):
		filename = asksaveasfilename(filetypes=[("Saved CDR Responses", ".xml")],
         title="Save CDR Responses")
		if filename:
			if not re.search(".*.xml",filename):
				filename = filename + ".xml"
			file = open(filename, 'w')
			filedata = file.write(self.backData.get('1.0',END))
			file.close()

	#---Event handler to clear the response text window
	def clearResponses(self):
		self.backData.delete("1.0",END)		

	#---Event handler to send the commands to the server	
	def	sendCommands(self):
		cmds = ""
		cmd = self.backLabel.get("1.0",END)
		cmd = string.rstrip(cmd)
		
		if cmd:
			if self.wrap.get() == 1:
				if re.search("<CdrCommandSet>",cmd) or re.search("<CdrLogon>",cmd):
				    showerror("Send Commands to CDR","Strip CdrCommandSet and CdrLogon commands before using!")
				else:
					init ="<CdrCommandSet>\n <CdrCommand>\n  <CdrLogon>\n   <UserName>"+self.cdrname.get()+"</UserName>\n   <Password>"+self.cdrpw.get()+"</Password>\n  </CdrLogon>\n </CdrCommand>"
					end  ="\n  <CdrCommand>\n   <CdrLogoff/>\n  </CdrCommand>\n</CdrCommandSet>"
					cmds = init+cmd+end
			else:
				cmds = cmd

			if cmds:
				s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				s.connect((self.localhost, self.port))
				s.send(struct.pack('!L', len(cmds)))
				s.send(cmds)

				# 	Read the server's response.
				(rlen,) = struct.unpack('!L', s.recv(4))
				rsp = ''

				while len(rsp) < rlen:
					rsp = rsp + s.recv(rlen - len(rsp))

				rsp = rsp + "\n"
				self.backData.insert(END,rsp)

				s.close()
		else:
			showerror("Bad Command","No Commands were found to send to the server.")				

root = Tk()
root.title('CDR Command Test Utility')
app = App(root)
app.showDialog(root)
root.mainloop()

