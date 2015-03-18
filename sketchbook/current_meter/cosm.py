#
#   Library to send data to a Cosm feed
#
#   Example:
#
#   cosm = Cosm(feed, key)
#
#   info = [ ("0", 1234), ("1", 3456) ]
#   cosm.put(info)

import httplib
import json

#
#   Post data to Cosm feed

class Cosm:

    host = 'api.cosm.com'
    agent = "python cosm 1.0"
 
    def __init__(self, feed, key, https=True):
        self.headers = {
            "Content-Type"  : "application/x-www-form-urlencoded",
            "X-ApiKey"      : key,
            "User-Agent"    : self.agent,
        }
        self.params = "/v2/feeds/" + str(feed)
        if https:
            self.Connection = httplib.HTTPSConnection
        else:
            self.Connection = httplib.HTTPConnection

    def put(self, info):
        items = []

        for key, value in info:
            items.append( { "id" : key, "current_value" : value } )

        data = {
          "version" : "1.0.0",
          "datastreams": items,
        }
        body = json.dumps(data)

        http = self.Connection(self.host)
        http.request("PUT", self.params, body, self.headers)
        response = http.getresponse()
        http.close()
        return response.status, response.reason

# FIN
