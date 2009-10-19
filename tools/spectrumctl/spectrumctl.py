# -*- coding: utf-8 -*-
from twisted.internet import reactor, task
import sys, os
from twisted.internet import reactor
from twisted.names.srvconnect import SRVConnector
from twisted.words.xish import domish
from twisted.words.protocols.jabber import xmlstream, client, jid
from twisted.words.protocols.jabber.client import IQ
from configobj import ConfigObj
import signal

class XMPPClientConnector(SRVConnector):
	def __init__(self, reactor, domain, factory):
		SRVConnector.__init__(self, reactor, 'xmpp-client', domain, factory)


	def pickServer(self):
		#host, port = SRVConnector.pickServer(self)

		if not self.servers and not self.orderedServers:
			# no SRV record, fall back..
			port = 5222

		return self.servers[0], self.servers[1]

class Client(object):
	def __init__(self, client_jid, secret):
		self.jid = client_jid
		self.password = secret
		self.f = client.basicClientFactory(client_jid, secret)
		self.f.addBootstrap(xmlstream.STREAM_CONNECTED_EVENT, self.connected)
		self.f.addBootstrap(xmlstream.STREAM_END_EVENT, self.disconnected)
		self.f.addBootstrap(xmlstream.STREAM_AUTHD_EVENT, self.authenticated)
		connector = XMPPClientConnector(reactor, client_jid.host, self.f)
		connector.servers = [self.jid.host, 5222]
		connector.orderedServers = [self.jid.host, 5222]
		connector.connect()
		self.t = reactor.callLater(10, self.failed)

	def failed(self):
		reactor.stop()

	def rawDataIn(self, buf):
		#self.logs += "RECV: %s\n" % unicode(buf, 'utf-8').encode('ascii', 'replace')
		#print "RECV: %s" % unicode(buf, 'utf-8').encode('ascii', 'replace')
		pass

	def rawDataOut(self, buf):
		#self.logs += "SEND: %s\n" % unicode(buf, 'utf-8').encode('ascii', 'replace')
		#print "SEND: %s" % unicode(buf, 'utf-8').encode('ascii', 'replace')
		pass

	def connected(self, xs):
		self.xmlstream = xs

		# Log all traffic
		xs.rawDataInFn = self.rawDataIn
		xs.rawDataOutFn = self.rawDataOut

	def disconnected(self, xs):
		pass

	def authenticated(self, xs):
		self.getStats(sys.argv[3])

	def getStats(self, jid = "icq.netlab.cz"):
		iq = IQ(self.xmlstream, "get")
		iq['to'] = jid
		iq.addElement(("http://jabber.org/protocol/stats", "query"))

		iq.addCallback(self._statsReceived)
		iq.send()
	
	def _statsReceived(self, el):

		iq = IQ(self.xmlstream, "get")
		iq['to'] = el['from']
		q = iq.addElement(("http://jabber.org/protocol/stats", "query"))

		query = el.firstChildElement()
		for child in query.children:
			s = q.addElement('stat')
			s['name'] = child['name']

		iq.addCallback(self._statsDataReceived)
		iq.send()

	def _statsDataReceived(self, el):
		query = el.firstChildElement()
		for stat in query.elements():
			print stat['name'].replace('/', '_').replace('-',  '_'),stat['value'],stat["units"]
		reactor.stop()

def getConfig():
	filename = sys.argv[1]
	if not os.path.exists(filename):
		if not filename.endswith(".cfg"):
			filename += ".cfg"
		if os.path.exists("/etc/spectrum/" + filename):
			filename = "/etc/spectrum/" + filename
			
	if not os.path.exists(filename):
		print "Config file", filename, "does not exist."
		exit(0)

	config = ConfigObj(filename)
	return config, filename

if not len(sys.argv) in [3,5] or (len(sys.argv) == 3 and not sys.argv[2] in ["start", "stop"]):
	print "Usage: " + sys.argv[0] + " <config file> start"
	print "Usage: " + sys.argv[0] + " <config file> stop"
	print "Usage: " + sys.argv[0] + " <bare JID> <password> <transport hostname> <command> [args...]"
	print "Commands list :"
	print "   \"stats\" - show transport statistics"
	exit(0)

if sys.argv[2] == "start":
	config, filename = getConfig()

	pid_file = "/var/run/spectrum/" + config['service']['jid']
	if config['service'].has_key("pid_file"):
		pid_file = config['service']['pid_file'].replace("$jid", config['service']['jid'])
	try:
		f = open(pid_file, 'r');
		print "There is already running spectrum instance for this jid, please this instance first"
	except:
		f = None
		pass
	if f:
		pid = f.read()[:-1]
		f.close()
		exit(0)

	print "Starting spectrum isntance for jid", config['service']['jid'];
	if os.path.exists(os.path.join(os.getcwd(),"spectrum")):
		os.system(os.path.join(os.getcwd(),"spectrum") + " " + filename)
	else:
		os.system("spectrum" + " " + filename)
	print "Started"
elif sys.argv[2] == "stop":
	config, filename = getConfig()

	pid_file = "/var/run/spectrum/" + config['service']['jid']
	if config['service'].has_key("pid_file"):
		pid_file = config['service']['pid_file'].replace("$jid", config['service']['jid'])
	try:
		f = open(pid_file, 'r');
	except:
		print "There is no running instance for jid", config['service']['jid']
		exit(0)
	pid = f.read()[:-1]
	f.close()
	
	print "Stopping", config['service']['jid'], "(" + str(pid_file) + ")", "(" + str(pid) + ")"
	os.system("rm " + pid_file)
	os.kill(int(pid), signal.SIGTERM)
	print "Stopped"
else:
	if (sys.argv[4] == "stats"):
		Client(jid.JID(sys.argv[1] + "/stats"), sys.argv[2])
		reactor.run()
	else:
		print "Unknown command:", sys.argv[4]