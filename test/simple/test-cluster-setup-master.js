// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.


var common = require('../common');
var assert = require('assert');
var cluster = require('cluster');
var os = require("os");

if (cluster.isWorker) {
  
  //Just keep the worker alive
  var http = require('http');
  http.Server(function () {

  }).listen(common.PORT, "127.0.0.1", function () {
    cluster.worker.send(process.argv[2]);  
  });
}

else if (cluster.isMaster) {

  var checks = {
    workers: false,
    args: false
  };
  
  var cpus = os.cpus().length;
  
  //Setup master
  cluster.setupMaster({
    workers: (cpus + 1),
    args: ['custom argument']
  });
  
  var correctIn = 0;
  
  cluster.on('online', function lisenter (worker) {
    
    worker.once('message', function(data) {
       correctIn += (data === 'custom argument' ? 1 : 0);
       if (correctIn === (cpus + 1)) {
          checks.args = true;
          process.exit(0);
       }
    });
    
    //All workers are online
    if (cluster.onlineWorkers ===  (cpus + 1)) {
      checks.workers = true;
    }
  });
  
  //Start all workers
  cluster.autoFork();
  
  //Check all values
  process.once('exit', function () {
    assert.ok(checks.workers, 'Not all workers was spawned.');
    assert.ok(checks.args, 'The arguments was noy send to the worker');
  });

}