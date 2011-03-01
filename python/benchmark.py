# coding=UTF-8
import simplejson
import ujson
import json
import cjson
import time

user = { "userId": 3381293, "username": "johndoe", "fullname": u"John Doe the Second", "isAuthorized": True, "approval": 31.1471, "jobs": [ 1, 2 ], "currJob": None }
friends = [ user, user, user, user, user, user, user, user ]
testObject = [ [user, friends],  [user, friends],  [user, friends],  [user, friends],  [user, friends],  [user, friends]]

decodeData = ""

"""=========================================================================="""

def ujsonEnc():
	x = ujson.encode(testObject)
	
def simplejsonEnc():
	x = simplejson.dumps(testObject)

def jsonEnc():
	x = json.dumps(testObject)

def cjsonEnc():
	x = cjson.encode(testObject)

"""=========================================================================="""

def ujsonDec():
	x = ujson.decode(decodeData)
	
def simplejsonDec():
	x = simplejson.loads(decodeData)

def jsonDec():
	x = json.loads(decodeData)

def cjsonDec():
	x = cjson.encode(decodeData)
	
"""=========================================================================="""
	
	
if __name__ == "__main__":
	import timeit
	
	COUNT = 10000
	
	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,5, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,5, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 5, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 5, COUNT)), )

	decodeData = json.dumps(testObject)
	
	file = open("testdata.txt", "w")
	file.write(decodeData)
	file.close()

	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,5, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,5, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,5, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 5, COUNT)), )

	