var common = require("../common");
var assert = require('assert');
var http = require("http");

var test_res_body = "other stuff!\n";

var server = http.createServer(function(req, res) {
  assert.ok(! ("date" in req.headers), 
    "Request headers contained a Date."
  );
  res.writeHead(200, {
    'Content-Type' : 'text/plain',
  });
  res.end(test_res_body);
});
server.listen(common.PORT);


server.addListener("listening", function() {
  var options = {
    port: common.PORT,
    path: "/",
    method: "GET"
  }
  var req = http.request(options, function (res) {
    assert.ok("date" in res.headers, 
      "Response headers didn't contain a Date."
    );
    res.addListener('end', function () {
      server.close();
      process.exit();
    });    
  });
  req.end();
});
