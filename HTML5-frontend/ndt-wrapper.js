/* This is an NDT client, written in javascript.  It speaks the websocket
 * version of the NDT protocol.  The NDT protocol is documented at:
 * https://code.google.com/p/ndt/wiki/NDTProtocol
 */

/*jslint bitwise: true, browser: true, nomen: true, vars: true */
/*global Uint8Array, WebSocket */

'use strict';

function NDTWrapper(server) {
    var _this = this;

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

    if (server) {
        this._hostname = server;
    }
    else {
        this._hostname = location.hostname;
    }

    this._port = 3001;
    this._path = "/ndt_protocol";

    this.reset();
}

NDTWrapper.prototype.reset = function () {
    this._ndt_vars = { 'ClientToServerSpeed': 0, 'ServerToClientSpeed': 0 };
    this._errmsg = "";
    this._status = "notStarted";
    this._diagnosis = "";

    this._client = new NDTjs(this._hostname, this._port, this._path, this.callbacks, 1000);
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

NDTWrapper.prototype.run_test = function () {
    this.reset();
    this._client.start_test();
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
