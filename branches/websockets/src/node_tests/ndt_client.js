/* This is an NDT client, written in node.js.  It speaks the websocket version
 * of the NDT protocol.  The NDT protocol is documented at:
 *   https://code.google.com/p/ndt/wiki/NDTProtocol
 */

/*jslint bitwise: true, node: true */
/*global Uint8Array */

'use strict';

// Constants in use by the entire program, and a live connection to the server.
var COMM_FAILURE = 0,
    SRV_QUEUE = 1,
    MSG_LOGIN = 2,
    TEST_PREPARE = 3,
    TEST_START = 4,
    TEST_MSG = 5,
    TEST_FINALIZE = 6,
    MSG_ERROR = 7,
    MSG_RESULTS = 8,
    MSG_LOGOUT = 9,
    MSG_WAITING = 10,
    MSG_EXTENDED_LOGIN = 11,
    msg_name = ["COMM_FAILURE", "SRV_QUEUE", "MSG_LOGIN", "TEST_PREPARE", "TEST_START", "TEST_MSG", "TEST_FINALIZE", "MSG_ERROR", "MSG_RESULTS", "MSG_LOGOUT", "MSG_WAITING", "MSG_EXTENDED_LOGIN"],
    WebSocket = require('ws'),
    server = process.argv[2],
    port = Number(process.argv[3]),
    test_url = "ws://" + server + ":" + port + "/ndt_after_user_privacy_agreement",
    ws = new WebSocket(test_url, {protocol: 'ndt'});

console.log("Running NDT test to " + server + " on port " + port);

// A helper function that prints an error message and crashes things.
function die(a1, a2, a3, a4) {
    console.log.apply(
        console.log, ["DIED: "].concat(Array.prototype.slice.call(arguments)));
    process.exit(1);
}

// Makes a login message suitable for sending to the server.  The login
// messages specifies the tests to be run.
function make_login_msg(desired_tests) {
    var i = 0,
        message = 'XXX { "msg": "Xv3.5.5" }',
        NDT_LOGIN_MSG = new Uint8Array(message.length);
    NDT_LOGIN_MSG[0] = MSG_EXTENDED_LOGIN;  // MSG_EXTENDED_LOGIN
    NDT_LOGIN_MSG[1] = 0;  // Two bytes to represent packet length
    NDT_LOGIN_MSG[2] = message.length - 3;
    for (i = 3; i < message.length; i += 1) {
        if (message.charAt(i) === 'X') {
            // The mid-message X specifies the tests to run.  We must support
            // TEST_STATUS (16) as a 3.5.5+ client
            NDT_LOGIN_MSG[i] = desired_tests | 16;
        } else {
            NDT_LOGIN_MSG[i] = message.charCodeAt(i);
        }
    }
    return NDT_LOGIN_MSG;
}

// A generic message creation system.  The output is an array of bytes suitable
// for sending on a binary websocket.
function make_ndt_msg(type, msg) {
    var message_body, NDT_MSG, i;
    message_body = '{ "msg": "' + msg + '" } ';
    NDT_MSG = new Uint8Array(message_body.length + 3);
    NDT_MSG[0] = type;
    NDT_MSG[1] = (message_body.length >> 8) & 0xFF;
    NDT_MSG[2] = message_body.length & 0xFF;
    for (i = 0; i < message_body.length; i += 1) {
        NDT_MSG[i + 3] = message_body.charCodeAt(i);
    }
    return NDT_MSG;
}

// Returns a closure that will process all messages for the META NDT test.  The
// closure will return the string "DONE" when the test is complete and the
// closure should no longer be called.
function ndt_meta_test(sock) {
    var state = "WAIT_FOR_TEST_PREPARE";
    return function (type, body) {
        if (state === "WAIT_FOR_TEST_PREPARE" && type === TEST_PREPARE) {
            state = "WAIT_FOR_TEST_START";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_START" && type === TEST_START) {
            // Send one piece of meta data and then an empty meta data packet
            sock.send(make_ndt_msg(TEST_MSG, "client.os.name:CLIWebsockets"), { binary: true, mask: true });
            sock.send(make_ndt_msg(TEST_MSG, ""), { binary: true, mask: true });
            state = "WAIT_FOR_TEST_FINALIZE";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_FINALIZE" && type === TEST_FINALIZE) {
            console.log("ndt_meta_test is done");
            return "DONE";
        }
        die("Bad state and message combo for META test: ", state, type, body.msg);
    };
}

// Returns a closure that will process all messages for the S2C NDT test.  The
// closure will return the string "DONE" when the test is complete and the
// closure should no longer be called.
function ndt_s2c_test(sock) {
    var state = "WAIT_FOR_TEST_PREPARE",
        server_port,
        test_connection,
        TRANSMITTED_BYTES = 0,
        test_start,
        test_end;

    // Function called on the opening of the s2c socket.
    function on_open() {
        console.log("OPENED S2C SUCCESFULLY!");
        test_start = Date.now() / 1000;
    }

    // Function called for each message received on the s2c socket.
    function on_msg(message) {
        var hdr_size;
        if (message.length < 126) {
            hdr_size = 2;
        } else if (message.length < 65536) {
            hdr_size = 4;
        } else {
            hdr_size = 10;
        }
        TRANSMITTED_BYTES += (hdr_size + message.length);
    }

    // If there is an error on the s2c socket, then die.
    function on_error(error, num) {
        die(error, num);
    }

    // The closure that processes messages on the control socket for the s2c test.
    return function (type, body) {
        var TEST_DURATION_SECONDS,
            THROUGHPUT_VALUE;
        console.log("CALLED S2C with %d (%s) %s in state %s", type, msg_name[type], body.msg, state);
        if (state === "WAIT_FOR_TEST_PREPARE" && type === TEST_PREPARE) {
            server_port = Number(body.msg);
            // bind a connection to that port
            test_connection = new WebSocket("ws://" + server + ":" + server_port + "/ndt_after_user_privacy_agreement", {protocol: "s2c"});
            test_connection.on('open', on_open);
            test_connection.on('message', on_msg);
            test_connection.on('error', on_error);
            state = "WAIT_FOR_TEST_START";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_START" && type === TEST_START) {
            state = "WAIT_FOR_FIRST_TEST_MSG";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_FIRST_TEST_MSG" && type === TEST_MSG) {
            state = "WAIT_FOR_TEST_MSG_OR_TEST_FINISH";
            if (test_end === undefined) {
                test_end = Date.now() / 1000;
            }
            // All caps because that's how it is in the NDT spec
            TEST_DURATION_SECONDS = test_end - test_start;
            // Calculation per NDT spec
            THROUGHPUT_VALUE = 8 * TRANSMITTED_BYTES / 1000 / TEST_DURATION_SECONDS;
            sock.send(make_ndt_msg(TEST_MSG, String(THROUGHPUT_VALUE)), { binary: true, mask: true });
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_MSG_OR_TEST_FINISH" && type === TEST_MSG) {
            console.log("Got results: ", body.msg);
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_MSG_OR_TEST_FINISH" && type === TEST_FINALIZE) {
            console.log("Test is over! ", body.msg);
            return "DONE";
        }
        die("S2C: State = " + state + " type = " + type + "(" + msg_name[type] + ") message = ", body);
    };
}


function ndt_c2s_test() {
    var state = "WAIT_FOR_TEST_PREPARE",
        server_port,
        test_connection,
        TRANSMITTED_BYTES = 0,
        data_to_send = new Uint8Array(8192 - 4),
        i,
        test_start,
        test_end;

    for (i = 0; i < data_to_send.length; i += 1) {
        // All the characters must be printable, and the printable range of
        // ASCII is from 32 to 126.  101 is because we need a prime number.
        data_to_send[i] = 32 + (i * 101) % (126 - 32);
    }

    // A while loop, encoded as a setTimeout callback.
    function keep_sending_data() {
        test_connection.send(data_to_send);
        TRANSMITTED_BYTES += 8192;
        if (Date.now() / 1000 < test_start + 10) {
            setTimeout(keep_sending_data, 0);
        } else {
            test_end = Date.now() / 1000;
        }
    }

    return function (type, body) {
        console.log("C2S type %d (%s)", type, msg_name[type], body);
        if (state === "WAIT_FOR_TEST_PREPARE" && type === TEST_PREPARE) {
            server_port = Number(body.msg);
            test_connection = new WebSocket("ws://" + server + ":" + server_port + "/ndt_after_user_privacy_agreement", {protocol: "c2s"});
            state = "WAIT_FOR_TEST_START";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_START" && type === TEST_START) {
            test_start = Date.now() / 1000;
            keep_sending_data();
            state = "WAIT_FOR_TEST_MSG";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_MSG" && type === TEST_MSG) {
            state = "WAIT_FOR_TEST_FINALIZE";
            return "KEEP GOING";
        }
        if (state === "WAIT_FOR_TEST_FINALIZE" && type === TEST_FINALIZE) {
            state = "DONE";
            console.log("C2S rate: ", 8 * TRANSMITTED_BYTES / 1000 / (test_end - test_start));
            return "DONE";
        }
        die("C2S: State = " + state + " type = " + type + "(" + msg_name[type] + ") message = ", body);
    };
}


function ndt_coordinator(sock) {
    var state = "",
        active_test,
        tests_to_run = [];

    function on_open() {
        console.log("OPENED CONNECTION");
        // Sign up for every test except for TEST_MID and TEST_SFW - browsers can't
        // open server sockets, which makes those tests impossible, because they
        // require the server to open a connection to a port on the client.
        sock.send(make_login_msg(2 | 4 | 32), { binary: true, mask: true });
        state = "LOGIN_SENT";
    }

    function on_message(message) {
        var type = message[0],
            body = JSON.parse(message.slice(3)),
            i,
            tests;
        console.log("type = %d (%s) body = '%s'", type, msg_name[type], body.msg);
        if (active_test === undefined && tests_to_run.length > 0) {
            active_test = tests_to_run.pop();
        }
        if (active_test !== undefined) {
            // Pass the message to the sub-test
            console.log("Calling a subtest");
            if (active_test(type, body) === "DONE") {
                active_test = undefined;
                console.log("Subtest complete");
            }
            return;
        }
        // If there is an active test, hand off control to the test
        // Otherwise, move the coordinator state forwards.
        if (state === "LOGIN_SENT") {
            // Response to NDT_LOGIN should be SRV_QUEUE messages until we get SRV_QUEUE("0")
            if (type === SRV_QUEUE) {
                if (body.msg === "9990") {    // special keepalive message
                    sock.send(make_ndt_msg(MSG_WAITING, ""), { binary: true, mask: true });
                } else if (body.msg === "9977") {    // Test failed
                    die("server terminated test with SRV_QUEUE 9977");
                }
                console.log("Got SRV_QUEUE.    Ignoring and waiting for MSG_LOGIN");
            } else if (type === MSG_LOGIN) {
                if (body.msg[0] !== "v") { die("Bad msg '%s'", body.msg); }
                state = "WAIT_FOR_TEST_IDS";
            } else {
                die("Bad type when we wanted a srv_queue or msg_login ({%d, %d} vs %d)", SRV_QUEUE, MSG_LOGIN, message[0]);
            }
        } else if (state === "WAIT_FOR_TEST_IDS" && type === MSG_LOGIN) {
            tests = body.msg.split(" ");
            for (i = tests.length - 1; i >= 0; i -= 1) {
                if (tests[i] === "2") {
                    tests_to_run.push(ndt_c2s_test());
                } else if (tests[i] === "4") {
                    tests_to_run.push(ndt_s2c_test(sock));
                } else if (tests[i] === "32") {
                    tests_to_run.push(ndt_meta_test(sock));
                } else if (tests[i] !== '') {
                    die("Unknown test type: %d", tests[i], tests, body.msg);
                }
            }
            state = "WAIT_FOR_MSG_RESULTS";
        } else if (state === "WAIT_FOR_MSG_RESULTS" && type === MSG_RESULTS) {
            console.log(body);
        } else if (state === "WAIT_FOR_MSG_RESULTS" && type === MSG_LOGOUT) {
            sock.close();
            console.log("TESTS FINISHED SUCCESSFULLY!");
            process.exit(0);
        } else {
            die("Didn't know what to do with message type %d in state %s", message[0], state);
        }
    }

    sock.on('open', on_open);
    sock.on('message', on_message);
    sock.on('error', function (err_msg, code) {
        console.error("Error: %s (%d)", err_msg, code);
        process.exit(1);
    });
}

ndt_coordinator(ws);
