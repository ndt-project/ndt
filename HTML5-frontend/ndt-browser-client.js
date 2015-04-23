/* This is an NDT client, written in javascript.  It speaks the websocket
 * version of the NDT protocol.  The NDT protocol is documented at:
 * https://code.google.com/p/ndt/wiki/NDTProtocol
 */

/*jslint bitwise: true, browser: true, nomen: true, vars: true */
/*global Uint8Array, WebSocket */

'use strict';

function NDTjs(server, server_port, server_path, callbacks, update_interval) {

    this.server = server;
    this.server_port = server_port;
    this.server_path = server_path;
    this.results = {
        c2s_rate: undefined,
        s2c_rate: undefined
    };
    this.mlab_server = undefined;
    this.update_interval = update_interval / 1000.0;
    this.SEND_BUFFER_SIZE = 1048576;

    // Someone may want to run this test without callbacks (perhaps for
    // debugging). Since the callbacks are referenced in various places, just
    // create some empty ones if none were specified.
    if (callbacks === undefined) {
        this.callbacks = {
            'onstart': function () { return false; },
            'onchange': function () { return false; },
            'onfinish': function () { return false; },
            'onerror': function () { return false; },
        };
    } else {
        this.callbacks = callbacks;
    }

    // Constants in use by the entire program, and a live connection to the
    // server.  The order of these is important because their equivalent
    // numeric representations correspond to the index number in the array.
    this.NDT_MESSAGES = [
        "COMM_FAILURE",
        "SRV_QUEUE",
        "MSG_LOGIN",
        "TEST_PREPARE",
        "TEST_START",
        "TEST_MSG",
        "TEST_FINALIZE",
        "MSG_ERROR",
        "MSG_RESULTS",
        "MSG_LOGOUT",
        "MSG_WAITING",
        "MSG_EXTENDED_LOGIN"
    ];
}

/**
 * Provide feedback to the console or the DOM.
 * @param {string} log_message Message to pass to output mechanism.
 * @param {!boolean=} debugging Optional (may be undefined) Determines whether 
 *    to output messages or to operate silently.
 */

NDTjs.prototype.logger = function (log_message, debugging) {
    debugging = debugging || false;
    if (debugging === true) {
        console.log(log_message);
    }
};

/**
 * Check that the browser supports the NDT test.
 * @returns {boolean} Browser supports necessary functions for test client.
 */

NDTjs.prototype.check_browser_support = function () {

    if (window.WebSocket == undefined && window.MozWebSocket == undefined) {
        throw this.UnsupportedBrowser("No Websockets");
    }
    return true;
};

/**
 * Make an AJAX request to M-Lab NS for the closest NDT service.
 * @returns {Object} JSON Object containing (city, country, fqdn,
 *    ip [list: ipv6, ipv4], site, url, metro); 
 *    Note: metro is not native to M-Lab NS
 */

NDTjs.prototype.find_ndt_server = function () {

    var mlab_ns_request = new XMLHttpRequest();
    var mlab_ns_url = "http://mlab-ns.appspot.com/ndt?format=json";

    mlab_ns_request.open("GET", mlab_ns_url, false);
    mlab_ns_request.send();

    if (mlab_ns_request.status === 200) {
        this.mlab_server = JSON.parse(mlab_ns_request.responseText);
        this.mlab_server.metro = this.mlab_server.site.slice(0, 3);
        this.logger('M-Lab NS lookup answer:' + this.mlab_server);
    } else {
        this.mlab_server = undefined;
        this.logger('M-Lab NS lookup failed.');
    }

    return this.mlab_server;
};

/**
 * Makes a login message suitable for sending to the server.  The login
 * messages specifies the tests to be run.
 * @param {number} desired_tests The types of tests requested from the server
 *     signaled based on a bitwise operation of the test ids.
 * @returns {Uint8Array} NDT login message signalling the desired tests.
 */

NDTjs.prototype.make_login_message = function (desired_tests) {
    // We must support TEST_STATUS (16) as a 3.5.5+ client, so we make sure
    // test 16 is desired.
    var login_message = 'XXX { "msg": "v3.5.5", "tests": "' +
        (desired_tests | 16) + '" }';
    var login_data = new Uint8Array(login_message.length);
    var i;

    login_data[0] = this.NDT_MESSAGES.indexOf('MSG_EXTENDED_LOGIN');
    login_data[1] = 0;  // Two bytes to represent packet length
    login_data[2] = login_message.length - 3;

    for (i = 3; i < login_message.length; i += 1) {
        login_data[i] = login_message.charCodeAt(i);
    }
    return login_data;
};

/**
 * A generic message creation system for NDT.
 * (message_type, message body length [2], message body)
 * @params {number} message_type The type of message according to NDT's 
 *    specification.
 * @params {string} message_content The message body.
 * @returns {array} An array of bytes suitable for sending on a binary
 *    websocket.
 */

NDTjs.prototype.make_ndt_message = function (message_type, message_content) {
    var message_body,
        ndt_message,
        i;

    message_body = '{ "msg": "' + message_content + '" } ';
    ndt_message = new Uint8Array(message_body.length + 3);
    ndt_message[0] = message_type;
    ndt_message[1] = (message_body.length >> 8) & 0xFF;
    ndt_message[2] = message_body.length & 0xFF;

    for (i = 0; i < message_body.length; i += 1) {
        ndt_message[i + 3] = message_body.charCodeAt(i);
    }
    return ndt_message;
};

/**
 * Parses messages received from the NDT server.
 * @param {object} buffer The complete message received from the NDT server.
 * @returns {array} Parsed messaged.
 */

NDTjs.prototype.parse_ndt_message = function (buffer) {
    var response = [];
    var buffer_array = new Uint8Array(buffer);
    var message =  String.fromCharCode.apply(null,
        new Uint8Array(buffer.slice(3)));
    var i;

    for (i = 0; i < 3; i += 1) {
        response[i] = buffer_array[i];
    }

    response.push(message);
    return response;
};

/**
 * Exception related to low-level connectivity failures.
 * @param {string} message Specific failure messages passed in the course of
 *    receiving the exception.
 */

NDTjs.prototype.ConnectionException = function (message) {
    this.logger(message);
    this.callbacks.onerror(message);
};

/**
 * Exception related to an unsupported browser.
 * @param {string} message Specific failure messages passed in the course of
 *    receiving the exception.
 */

NDTjs.prototype.UnsupportedBrowser = function (message) {
    this.logger(message);
    this.callbacks.onerror(message);
};

/**
 * Exception related to test failures, such as behavior inconsistent with
 * expectations.
 * @param {string} message Specific failure messages passed in the course of
 *    receiving the exception.
 */

NDTjs.prototype.TestFailureException = function (message) {
    this.logger(message);
    this.callbacks.onerror(message);
};

/**
 * A simple helper function to create websockets consistently.
 * @param {string} server_address The FQDN or IP of the NDT server.
 * @param {Number} server_port The port expected for the NDT test.
 * @param {string} url_path The path of the resource to request from NDT.
 * @param {string} protocol The WebSocket protocol to build for.
 * @returns {Websocket} The WebSocket we created;
 */

NDTjs.prototype.create_websocket = function (server_address, server_port,
    url_path, protocol) {

    var created_websocket = new WebSocket("ws://" + server_address + ":" +
        server_port + url_path, protocol);
    created_websocket.binaryType = 'arraybuffer';
    return created_websocket;
};

/**
 * NDT's Client-to-Server (C2S) Upload Test
 * Serves as a closure that will process all messages for the C2S NDT test.
 * @returns {boolean} The test is complete and the closure should no longer 
 *    be called.
 */

NDTjs.prototype.ndt_c2s_test = function () {
    var server_port,
        test_connection,
        test_start,
        i;
    var data_to_send = new Uint8Array(this.SEND_BUFFER_SIZE);
    var _this = this;

    var state = "WAIT_FOR_TEST_PREPARE";
    var total_sent = 0;
    var next_callback_time = this.update_interval;

    for (i = 0; i < data_to_send.length; i += 1) {
        // All the characters must be printable, and the printable range of
        // ASCII is from 32 to 126.  101 is because we need a prime number.
        data_to_send[i] = 32 + (i * 101) % (126 - 32);
    }

    // A while loop, encoded as a setTimeout callback.
    function keep_sending_data() {
        // Monitor the buffersize as it sends and refill if it gets too low.
        if (test_connection.bufferedAmount < 8192) {
            test_connection.send(data_to_send);
            total_sent += data_to_send.length;
        }
        var curr_time = Date.now() / 1000.0;

        if (_this.update_interval && curr_time > test_start + next_callback_time) {
            var rate = 8 * (total_sent - test_connection.bufferedAmount) / 1000 / (curr_time - test_start);
            _this.results.c2s_rate = rate;
            _this.callbacks.onchange('interval_c2s', _this.results);
            next_callback_time += _this.update_interval;
        }

        if (curr_time < test_start + 10) {
            setTimeout(keep_sending_data, 0);
        } else {
            return false;
        }
    }

    /**
    * The closure that processes messages on the control socket for the 
    * C2S test.
    */
    return function (message_type, message_content) {
        _this.logger('CALLED C2S with ' + message_type + ' (' +
            _this.NDT_MESSAGES[message_type] + ') ' + message_content.msg +
            ' in state ' + state);
        if (state === "WAIT_FOR_TEST_PREPARE" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_PREPARE')) {
            _this.callbacks.onchange('preparing_c2s', _this.results);

            server_port = Number(message_content.msg);
            test_connection = _this.create_websocket(_this.server, server_port,
                _this.server_path, 'c2s');

            state = "WAIT_FOR_TEST_START";
            return false;
        }
        if (state === "WAIT_FOR_TEST_START" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_START')) {
            _this.callbacks.onchange('running_c2s', _this.results);

            test_start = Date.now() / 1000;
            keep_sending_data();

            state = "WAIT_FOR_TEST_MSG";
            return false;
        }
        if (state === "WAIT_FOR_TEST_MSG" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_MSG')) {
            _this.results.c2s_rate = Number(message_content.msg);
            _this.logger("C2S rate calculated by server: " +
                _this.results.c2s_rate);
            state = "WAIT_FOR_TEST_FINALIZE";
            return false;
        }
        if (state === "WAIT_FOR_TEST_FINALIZE" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_FINALIZE')) {
            _this.callbacks.onchange('finished_c2s', _this.results);

            state = "DONE";
            return true;
        }
        _this.logger("C2S: State = " + state + " type = " + message_type + "(" +
            _this.NDT_MESSAGES[message_type] + ") message = ", message_content);
    };
};

/**
 * NDT's Server-to-Client (S2C) Download Test
 * Serves as a closure that will process all messages for the S2C NDT test.
 * @param {Websocket} ndt_socket A websocket connection to the NDT server.
 * @returns {boolean} The test is complete and the closure should no longer 
 *    be called.
 */

NDTjs.prototype.ndt_s2c_test = function (ndt_socket) {
    var server_port,
        test_connection,
        test_start,
        test_end,
        error_message;
    var state = "WAIT_FOR_TEST_PREPARE";
    var received_bytes = 0;
    var next_callback_time = this.update_interval;
    var _this = this;

    /**
    * The closure that processes messages on the control socket for the 
    * C2S test.
    */
    return function (message_type, message_content) {

        _this.logger("CALLED S2C with " + message_type + " (" +
            _this.NDT_MESSAGES[message_type] + ") in state " + state);

        if (state === "WAIT_FOR_TEST_PREPARE" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_PREPARE')) {
            _this.callbacks.onchange('preparing_s2c', _this.results);

            server_port = Number(message_content.msg);
            test_connection = _this.create_websocket(_this.server, server_port,
                _this.server_path, 's2c');

            test_connection.onopen = function () {
                _this.logger("Successfully opened S2C test connection.");
                test_start = Date.now() / 1000;
            };

            test_connection.onmessage = function (response) {
                var response_message_size = response.data.byteLength;
                var hdr_size;
                if (response_message_size < 126) {
                    hdr_size = 2;
                } else if (response_message_size < 65536) {
                    hdr_size = 4;
                } else {
                    hdr_size = 10;
                }
                received_bytes += (hdr_size + response_message_size);

                var curr_time = Date.now() / 1000.0;
                if (_this.update_interval && curr_time > test_start + next_callback_time) {
                    var rate = 8 * received_bytes / 1000 / (curr_time - test_start);
                    _this.results.s2c_rate = rate;
                    _this.callbacks.onchange('interval_s2c', _this.results);
                    next_callback_time += _this.update_interval;
                }
            };

            test_connection.onerror = function (response) {
                error_message = _this.parse_ndt_message(response.data)[3].msg;
                throw _this.TestFailureException(error_message);
            };

            state = "WAIT_FOR_TEST_START";
            return false;
        }
        if (state === "WAIT_FOR_TEST_START" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_START')) {
            _this.callbacks.onchange('running_s2c', _this.results);

            state = "WAIT_FOR_FIRST_TEST_MSG";
            return false;
        }
        if (state === "WAIT_FOR_FIRST_TEST_MSG" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_MSG')) {
            _this.logger('Got message: ' + JSON.stringify(message_content));
            state = "WAIT_FOR_TEST_MSG_OR_TEST_FINISH";

            if (test_end === undefined) {
                test_end = Date.now() / 1000;
            }
            // Calculation per spec, compared between client and server
            // understanding.
            _this.results.s2c_rate = (8 * received_bytes / 1000 /
                (test_end - test_start));
            _this.logger("S2C rate calculated by client: " +
                _this.results.s2c_rate);
            _this.logger("S2C rate calculated by server: " +
                message_content.ThroughputValue);

            ndt_socket.send(
                _this.make_ndt_message(
                    _this.NDT_MESSAGES.indexOf('TEST_MSG'),
                    String(_this.results.s2c_rate)
                )
            );
            return false;
        }
        if (state === "WAIT_FOR_TEST_MSG_OR_TEST_FINISH" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_MSG')) {

            var web100_record = message_content.msg.split(': ');
            var web100_variable = web100_record[0];
            var web100_result = web100_record[1].replace(/(\r\n|\n|\r)/gm, "");
            _this.results[web100_variable] = web100_result;

            return false;
        }
        if (state === "WAIT_FOR_TEST_MSG_OR_TEST_FINISH" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_FINALIZE')) {
            _this.callbacks.onchange('finished_s2c', _this.results);
            _this.logger("NDT S2C test is complete: " +  message_content.msg);
            return true;
        }
        _this.logger("S2C: State = " + state + " type = " + message_type + "(" +
            _this.NDT_MESSAGES[message_type] + ") message = ", message_content);
    };
};

/**
 * NDT's META (S2C) Download Test
 * Serves as a closure that will process all messages for the META NDT test, 
 *    which provides additional data to the NDT results.
 * @param {Websocket} ndt_socket A websocket connection to the NDT server.
 * @returns {boolean} The test is complete and the closure should no longer
 *    be called.
 */

NDTjs.prototype.ndt_meta_test = function (ndt_socket) {
    var error_message;
    var _this = this;
    var state = "WAIT_FOR_TEST_PREPARE";

    return function (message_type, message_content) {
        if (state === "WAIT_FOR_TEST_PREPARE" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_PREPARE')) {
            _this.callbacks.onchange('preparing_meta', _this.results);

            state = "WAIT_FOR_TEST_START";
            return false;
        }
        if (state === "WAIT_FOR_TEST_START" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_START')) {
            _this.callbacks.onchange('running_meta', _this.results);

            // Send one piece of meta data and then an empty meta data packet
            ndt_socket.send(
                _this.make_ndt_message(
                    _this.NDT_MESSAGES.indexOf('TEST_MSG'),
                    'client.os.name:NDTjs'
                )
            );
            ndt_socket.send(
                _this.make_ndt_message(
                    _this.NDT_MESSAGES.indexOf('TEST_MSG'),
                    ''
                )
            );

            state = "WAIT_FOR_TEST_FINALIZE";
            return false;
        }
        if (state === "WAIT_FOR_TEST_FINALIZE" &&
                message_type === _this.NDT_MESSAGES.indexOf('TEST_FINALIZE')) {
            _this.callbacks.onchange('finished_meta', _this.results);
            _this.logger("NDT META test complete.");
            return true;
        }
        error_message = "Bad state and message combo for META test: " +
            state + ", " + message_type + ", " + message_content.msg;
        throw _this.TestFailureException(error_message);
    };
};

/**
 * Start a series of NDT tests.
 **/

NDTjs.prototype.start_test = function () {
    var ndt_socket,
        active_test,
        state,
        error_message,
        i;
    var tests_to_run = [];
    var _this = this;

    this.logger('Test started.  Waiting for connection to server...');
    this.callbacks.onstart(this.server);

    ndt_socket = this.create_websocket(this.server, this.server_port,
        this.server_path, 'ndt');

    ndt_socket.onopen = function () {
        _this.logger("Opened connection on port " + _this.server_port);
        /**
        * Sign up for every test except for TEST_MID and TEST_SFW - 
        * browsers can't open server sockets, which makes those tests 
        * impossible, because they require the server to open a connection 
        * to a port on the client.
        */
        ndt_socket.send(_this.make_login_message(2 | 4 | 32));
        state = "LOGIN_SENT";
    };

    ndt_socket.onmessage = function (response) {
        var tests;
        var message = _this.parse_ndt_message(response.data);
        var message_type = message[0];
        var message_content = JSON.parse(message[3]);

        _this.logger('type = ' + message_type + ' (' +
            _this.NDT_MESSAGES[message_type] + ") body = '" +
            message_content.msg + "'");
        if (active_test === undefined && tests_to_run.length > 0) {
            active_test = tests_to_run.pop();
        }
        if (active_test !== undefined) {
            // Pass the message to the sub-test
            _this.logger("Calling a subtest");
            if (active_test(message_type, message_content) === true) {
                active_test = undefined;
                _this.logger("Subtest complete");
            }
            return;
        }

        // If there is an active test, hand off control to the test
        // Otherwise, move the coordinator state forwards.
        if (state === "LOGIN_SENT") {
            // Response to NDT_LOGIN should be SRV_QUEUE messages until we
            // get SRV_QUEUE("0")
            if (message_type === _this.NDT_MESSAGES.indexOf('SRV_QUEUE')) {
                if (message_content.msg === "9990") {
                    // Connection keepalive message
                    ndt_socket.send(
                        _this.make_ndt_message(
                            _this.NDT_MESSAGES.indexOf('MSG_WAITING'),
                            ''
                        )
                    );
                } else if (message_content.msg === "9977") {
                    // Test failed, leave now.
                    throw _this.TestFailureException('Server terminated test ' +
                        'with SRV_QUEUE 9977');
                }
                _this.logger('Got SRV_QUEUE. Ignoring and waiting for ' +
                    'MSG_LOGIN.');
            } else if (message_type ===
                    _this.NDT_MESSAGES.indexOf('MSG_LOGIN')) {
                if (message_content.msg[0] !== "v") {
                    _this.logger("Bad msg " + message_content.msg);
                }
                state = "WAIT_FOR_TEST_IDS";
            } else {
                error_message = 'Expected type 1 (SRV_QUEUE) or 2 (MSG_LOGIN)' +
                    'but got ' + message_type + ' (' +
                    _this.NDT_MESSAGES[message_type] + ')';
                throw _this.TestFailureException(error_message);
            }
        } else if (state === "WAIT_FOR_TEST_IDS" &&
                    message_type === _this.NDT_MESSAGES.indexOf('MSG_LOGIN')) {
            tests = message_content.msg.split(" ");
            for (i = tests.length - 1; i >= 0; i -= 1) {
                if (tests[i] === "2") {
                    tests_to_run.push(_this.ndt_c2s_test());
                } else if (tests[i] === "4") {
                    tests_to_run.push(_this.ndt_s2c_test(ndt_socket));
                } else if (tests[i] === "32") {
                    tests_to_run.push(_this.ndt_meta_test(ndt_socket));
                } else if (tests[i] !== '') {
                    error_message = "Unknown test type: " + tests[i];
                    throw _this.TestFailureException(error_message);
                }
            }
            state = "WAIT_FOR_MSG_RESULTS";
        } else if (state === "WAIT_FOR_MSG_RESULTS" &&
                message_type === _this.NDT_MESSAGES.indexOf('MSG_RESULTS')) {
            var lines = message_content.msg.split('\n');
            for (i = 0; i < lines.length; i++) {
                var line = lines[i];
                var record = line.split(': ');
                var variable = record[0];
                var result = record[1];
                _this.results[variable] = result;
            }
            _this.logger(message_content);
        } else if (state === "WAIT_FOR_MSG_RESULTS" &&
                message_type === _this.NDT_MESSAGES.indexOf('MSG_LOGOUT')) {
            ndt_socket.close();

            _this.callbacks.onchange('finished_all', _this.results);
            _this.callbacks.onfinish(_this.results);
            _this.logger("All tests successfully completed.");
        } else {
            error_message = "No handler for message " + message_type +
                " in state " + state;
            throw _this.TestFailureException(error_message);
        }
    };

    ndt_socket.onerror = function (response) {
        if (response.data) {
          error_message = _this.parse_ndt_message(response.data)[3].msg;
        }
        else {
          error_message = "unknown error";
        }

        throw _this.TestFailureException(error_message);
    };
};
