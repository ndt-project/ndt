/* This is an NDT client, written in javascript.  It speaks the websocket
 * version of the NDT protocol.  The NDT protocol is documented at:
 * https://code.google.com/p/ndt/wiki/NDTProtocol
 */

/*jslint bitwise: true, browser: true, nomen: true, vars: true */
/*global Uint8Array, WebSocket */

'use strict';

function NDTWrapper(server) {
    var _this = this;

    // XXX: include a way to override this
    this.use_web_worker = false;

    /* If they have Web Workers, use it to do the NDT test, unless it's Firefox
     * which doesn't support web workers using websockets. This should be
     * migrated into something more sane (e.g. a function that spawns an inline worker that
     * creates a websocket, and returns true/false if that succeeds */
    var isFirefox = typeof InstallTrigger !== 'undefined';     
    var supportsWebWorkers = !!window.Worker;

    if (supportsWebWorkers) {
      this.use_web_worker = true;
    }

    if (isFirefox) {
      this.use_web_worker = false;
    }

    if (server) {
        this._hostname = server;
    }
    else {
        this._hostname = location.hostname;
    }

    this._port = 3001;
    this._path = "/ndt_protocol";
    this._update_interval = 1000;

    this.reset();
}

NDTWrapper.prototype.reset = function () {
  if (this.worker) {
    this.worker.postMessage({
        'cmd': 'stop'
    });

    this.worker = null;
  }

  if (this.client) {
    this.client = null;
  }

  this._ndt_vars = { 'ClientToServerSpeed': 0, 'ServerToClientSpeed': 0 };
  this._errmsg = "";
  this._status = "notStarted";
  this._diagnosis = "";
};

NDTWrapper.prototype.run_test = function () {
    var _this = this;

    this.reset();

    if (this.use_web_worker) {
      console.log("Generating new worker");
      this.worker = new Worker('ndt-wrapper-ww.js');
      this.worker.addEventListener('message', function (e) {
        var msg = e.data;
        switch (msg.cmd) {
          case 'onstart':
            _this.onstart_cb(msg.server);
            break;
          case 'onchange':
            _this.onchange_cb(msg.state, msg.results);
            break;
          case 'onfinish':
            _this.onfinish_cb(msg.results);
            break;
          case 'onerror':
            _this.onerror_cb(msg.error_message);
            break;
        }
      }, false);

      this.worker.addEventListener('error', function (e) {
        _this.onerror_cb(e.lineno, ' in ', e.filename, ': ', e.message);
      }, false);

      this.worker.postMessage({
        'cmd': 'start',
        'hostname': this._hostname,
        'port': this._port,
        'path': this._path,
        'update_interval': this._update_interval
      });
    }
    else {
      this.callbacks = {
        'onstart': function(server) {
            _this.onstart_cb(server);
        },
        'onchange': function(state, results) {
            _this.onchange_cb(state, results);
        },
        'onfinish': function(passed_results) {
            _this.onfinish_cb(passed_results);
        },
        'onerror': function(error_message) {
            _this.onerror_cb(error_message);
        }
      };

      this.client = new NDTjs(this._hostname, this._port, this._path, this.callbacks, this._update_interval);
      this.client.start_test();
    }
};


NDTWrapper.prototype.get_status = function () {
    return this._status;
};

NDTWrapper.prototype.get_diagnosis = function () {
    return this._diagnosis;
};

NDTWrapper.prototype.get_errmsg = function () {
    return this._errmsg;
};

NDTWrapper.prototype.get_host = function () {
    return this._hostname;
};

NDTWrapper.prototype.getNDTvar = function (variable) {
    return this._ndt_vars[variable];
};

NDTWrapper.prototype.get_PcBuffSpdLimit = function (variable) {
    return Number.NaN;
};

NDTWrapper.prototype.onstart_cb = function (server) {
};

NDTWrapper.prototype.onchange_cb = function (state, results) {
    if (state === 'running_s2c') {
        this._status = 'Inbound';
        this._ndt_vars['ServerToClientSpeed'] = results['s2c_rate'] / 1000;
    }
    else if (state == 'interval_s2c' || state === 'finished_s2c') {
        this._ndt_vars['ServerToClientSpeed'] = results['s2c_rate'] / 1000;
    }
    else if (state === 'running_c2s') {
        this._status = 'Outbound';
    }
    else if (state === 'interval_c2s' || state == 'finished_c2s') {
        this._ndt_vars['ClientToServerSpeed'] = results['c2s_rate'] / 1000;
    }
};

NDTWrapper.prototype.onfinish_cb = function (results) {
    this._errmsg = "Test completed";
    this._ndt_vars = results;
    this._ndt_vars['ServerToClientSpeed'] = results['s2c_rate'] / 1000;
    this._ndt_vars['ClientToServerSpeed'] = results['c2s_rate'] / 1000;
    this._ndt_vars['Jitter'] = results['MaxRTT'] - results['MinRTT'];
    this.build_diagnosis();
};

NDTWrapper.prototype.build_diagnosis = function () {
    this._diagnosis = JSON.stringify(this._ndt_vars, null, "  ");
};

NDTWrapper.prototype.onerror_cb = function (error_message) {
    this._errmsg = "Test failed: "+error_message;
};
