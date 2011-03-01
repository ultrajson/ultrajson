# coding=UTF-8
import simplejson
import ujson
import json
import time

user = { "userId": 3381293, "username": "johndoe", "fullname": u"John Doe the Second", "isAuthorized": True, "approval": 31.1471, "jobs": [], "currJob": None }
friends = [ user, user, user, user, user, user, user, user ]
testObject = { "user": user, "friends": friends, "user": user, "friends": friends, "user": user, "friends": friends }

def ujsonEnc():
	x = ujson.encode(testObject)
	
def simplejsonEnc():
	x = simplejson.dumps(testObject)

def jsonEnc():
	x = simplejson.dumps(testObject)

	
if __name__ == "__main__":
	import timeit
	print "ujson encode     : ", timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", time.clock, 3, 100000)
	print "simplejson encode: ", timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", time.clock, 3, 100000)
	print "json encode      : ", timeit.repeat("jsonEnc()", "from __main__ import jsonEnc", time.clock, 3, 100000)
