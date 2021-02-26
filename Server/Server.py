#!/usr/bin/python3

# -
# | gba-save-tool Server
# | Manage GBA cart saves without a flashcart on a Nintendo DS
# |
# | Copyright (C) 2021, OctoSpacc
# | Licensed under the AGPLv3
# -

from http.server import BaseHTTPRequestHandler, HTTPServer

DumpSaveContent = ""

# Dumping save from DS
def DumpSave(RequestPath):
	global DumpSaveContent

	RequestPathParts = RequestPath.split("=")

	DumpSavePartIndex = RequestPathParts[1].split("&")[0]
	if DumpSavePartIndex == "1":
		DumpSaveContent = ""

	elif DumpSavePartIndex == "0":
		try:
			with open("SaveDump.sav", "wb") as SaveFile:
				SaveFile.write(bytes.fromhex(DumpSaveContent))

			return "Final part of save dump received, saving to file.".encode("utf-8")

		except:
			return "Final part of save dump received, but an error occourred trying to save to file.".encode("utf-8")

	DumpSaveContent += RequestPathParts[2]

	return ("Part " + str(DumpSavePartIndex) + " of save dump received.").encode("utf-8")

# Reading GET requests and responding accordingly.
def ReadGETParameters(RequestPath):
	if RequestPath == "/":
		return "Welcome to the gba-save-tool server!".encode("utf-8")

	elif RequestPath.startswith("/?DumpSavePart=".lower()):
		return DumpSave(RequestPath)

	return None

# Server main operational class.
class ServerClass(BaseHTTPRequestHandler):
	def SetResponse(self, ResponseCode, ContentType):
		self.send_response(ResponseCode)
		self.send_header("Content-type", ContentType)
		self.end_headers()

	def do_GET(self):
		ResponseContent = ReadGETParameters(self.path.lower())

		if ResponseContent == None:
			self.SetResponse(404, "text/plain")
			ResponseContent = "Error 404: Requested resource not found.".encode("utf-8")
		else:
			self.SetResponse(200, "text/plain")

		if ResponseContent != None:
			self.wfile.write(ResponseContent)

# Main function running the server.
def RunServer():
	ServerPort = 5244
	Server = HTTPServer(("", ServerPort), ServerClass)
	print("Starting HTTP Server at port " + str(ServerPort))

	try:
		Server.serve_forever()

	except KeyboardInterrupt:
		print("Stopping HTTP Server.")
		Server.server_close()

if  __name__ == "__main__":
	RunServer()