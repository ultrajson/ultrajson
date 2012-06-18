# coding=UTF-8
"""
Copyright (c) 2011-2012, ESN Social Software AB and Jonas Tarnstrom
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the ESN Social Software AB nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ESN SOCIAL SOFTWARE AB OR JONAS TARNSTROM BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Portions of code from:
MODP_ASCII - Ascii transformations (upper/lower, etc)
http://code.google.com/p/stringencoders/
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.
"""

import simplejson
import ujson
import sys
try:
    import json
except ImportError:
    json = simplejson
import cjson
from time import time as gettime
import time
import sys
import random

def timeit_compat_fix(timeit):
    if sys.version_info[:2] >=  (2,6):
        return
    default_number = 1000000
    default_repeat = 3
    if sys.platform == "win32":
        # On Windows, the best timer is time.clock()
        default_timer = time.clock
    else:
        # On most other platforms the best timer is time.time()
        default_timer = time.time
    def repeat(stmt="pass", setup="pass", timer=default_timer,
       repeat=default_repeat, number=default_number):
        """Convenience function to create Timer object and call repeat method."""
        return timeit.Timer(stmt, setup, timer).repeat(repeat, number)
    timeit.repeat = repeat

def ujsonDec():
    x = ujson.loads(decodeData)

def simplejsonDec():
    x = simplejson.loads(decodeData)
    
def ujsonEnc():
    x = ujson.dumps(encodeData)

def simplejsonEnc():
    x = simplejson.dumps(encodeData)
        
    
if __name__ == "__main__":
    import timeit
    timeit_compat_fix(timeit)
    
COUNT = 100

# Load file into memory
f = open("sample.json", "rb")
decodeData = f.read()
f.close()

encodeData = simplejson.loads(decodeData)

# Decode 1 million times
print "ujson decode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonDec()", "from __main__ import ujsonDec", gettime,10, COUNT)), )
print "simplejson decode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonDec()", "from __main__ import simplejsonDec", gettime,10, COUNT)), )

print "ujson encode      : %.05f calls/sec" % (COUNT / min(timeit.repeat("ujsonEnc()", "from __main__ import ujsonEnc", gettime,10, COUNT)), )
print "simplejson encode : %.05f calls/sec" % (COUNT / min(timeit.repeat("simplejsonEnc()", "from __main__ import simplejsonEnc", gettime,10, COUNT)), )

