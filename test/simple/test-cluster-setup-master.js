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
var os = require('os');

if (cluster.isWorker) {

  // Just keep the worker alive
  process.send(process.argv[2]);

} else if (cluster.isMaster) {

  var checks = {
    workers: false,
    args: false,
    setupEvent: false,
    settingsObject: false,
    forkMode: false
  };

  var cpus = os.cpus().length;

  cluster.once('setup', function() {
    checks.setupEvent = true;

    var settings = cluster.settings;
    if (settings
    && settings.workers === (cpus + 1)
    && settings.args && settings.args[0] === 'custom argument'
    && settings.silent === true
    && settings.exec === process.argv[1]) {
      checks.settingsObject = true;
    }
  });

  // Setup master
  cluster.setupMaster({
    workers: (cpus + 1),
    args: ['custom argument'],
    silent: true
  });

  var correctIn = 0;

  cluster.on('online', function (worker) {

    worker.once('message', function(data) {
      correctIn += (data === 'custom argument' ? 1 : 0);
      if (correctIn === (cpus + 1)) {
        checks.args = true;
        cluster.destroy();
      }
    });

    // All workers are online
    if (cluster.onlineWorkers === (cpus + 1)) {
      checks.workers = true;
    }
  });

  // Start all workers
  cluster.autoFork();

  // forkMode should now be auto
  checks.forkMode = cluster.settings.forkMode === 'auto';

  // Check all values
  process.once('exit', function() {
    assert.ok(checks.workers, 'Not all workers was spawned.');
    assert.ok(checks.args, 'The arguments was noy send to the worker');
    assert.ok(checks.setupEvent, 'The setup event was never emitted');
    assert.ok(checks.settingsObject, 'The settingsObject do not have correct properties');
    assert.ok(checks.forkMode, 'The forkMode was not set to auto after autoFork was executed');
  });

}
