# coding=UTF-8
import simplejson
import ujson
import json
import cjson
import time
import sys
import random

user = { "userId": 3381293, "age": 213, "username": "johndoe", "fullname": u"John Doe the Second", "isAuthorized": True, "liked": 31231.31231202, "approval": 31.1471, "jobs": [ 1, 2 ], "currJob": None }
friends = [ user, user, user, user, user, user, user, user ]

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
	x = cjson.decode(decodeData)
	
"""=========================================================================="""
	
	
if __name__ == "__main__":
	import timeit
	
	print "Ready? Configure affinity and priority, starting in 20..."
	time.sleep(20)


	print "Medium complex object:"
	testObject = [ [user, friends],  [user, friends],  [user, friends],  [user, friends],  [user, friends],  [user, friends]]
	COUNT = 5000

	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,10, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,10, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 10, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, COUNT)), )

	decodeData = json.dumps(testObject)
	
	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,10, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,10, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,10, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 3, COUNT)), )

	print "Array with 256 strings:"
	testObject = []
	
	for x in xrange(256):
		testObject.append("A pretty long string which is in a list")
	
	COUNT = 10000

	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,10, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,10, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 10, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, COUNT)), )

	decodeData = json.dumps(testObject)
	
	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,10, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,10, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,10, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 3, COUNT)), )
	
	print "Array with 256 doubles:"
	testObject = []
	
	for x in xrange(256):
		testObject.append(sys.maxint * random.random())
	
	COUNT = 10000

	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,10, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,10, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 10, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, COUNT)), )

	decodeData = json.dumps(testObject)
	
	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,10, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,10, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,10, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 3, COUNT)), )

	print "Array with 256 True values:"
	testObject = []
	
	for x in xrange(256):
		testObject.append(True)
	
	COUNT = 50000

	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,10, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,10, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 10, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, COUNT)), )

	decodeData = json.dumps(testObject)
	
	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,10, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,10, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,10, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 3, COUNT)), )
	

	print "Array with 256 dict{string, int} pairs:"
	testObject = []
	
	for x in xrange(256):
		testObject.append({str(random.random()*20): int(random.random()*1000000)})
	
	COUNT = 5000

	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,10, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,10, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 10, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, COUNT)), )

	decodeData = json.dumps(testObject)
	
	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,10, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,10, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,10, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 3, COUNT)), )


	
	print "Dict with 256 arrays with 256 dict{string, int} pairs:"
	testObject = {}


	for y in xrange(256):
		arrays = []
		for x in xrange(256):
			arrays.append({str(random.random()*20): int(random.random()*1000000)})
	
		testObject[str(random.random()*20)] = arrays
	
	
	COUNT = 50

	print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock,10, COUNT)), )
	print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock,10, COUNT)), )
	print "cjson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonEnc()", "from __main__ import cjsonEnc", time.clock, 10, COUNT)), )
	#print "json encode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, COUNT)), )

	decodeData = json.dumps(testObject)
	
	print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", time.clock,10, COUNT)), )
	print "cjson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("cjsonDec()", "from __main__ import cjsonDec", time.clock,10, COUNT)), )
	print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", time.clock,10, COUNT)), )
	#print "json decode       : %.05f calls/sec" % (COUNT / min(timeit.repeat("jsonDec()", "from __main__ import jsonDec", time.clock, 3, COUNT)), )

	