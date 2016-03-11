/* This is an NDT client, written in javascript.  It speaks the websocket
 * version of the NDT protocol.  The NDT protocol is documented at:
 * https://code.google.com/p/ndt/wiki/NDTProtocol
 */

/*jslint bitwise: true, browser: true, nomen: true, vars: true, indent: 2 */
/*global Uint8Array, WebSocket */

'use strict';

function NDTjs(server, serverPort, serverProtocol, serverPath, callbacks,
               updateInterval) {
  this.server = server;
  this.serverPort = serverPort || 3001;
  this.serverPath = serverPath || '/ndt_protocol';
  this.serverProtocol = serverProtocol || 'ws';
  this.updateInterval = updateInterval / 1000.0 || false;
  this.results = {
    c2sRate: undefined,
    s2cRate: undefined
  };
  this.SEND_BUFFER_SIZE = 1048576;

  // Someone may want to run this test without callbacks (perhaps for
  // debugging). Since the callbacks are referenced in various places, just
  // create some empty ones if none were specified.
  if (callbacks === undefined) {
    this.callbacks = {
      'onstart': function () { return false; },
      'onstatechange': function () { return false; },
      'onprogress': function () { return false; },
      'onfinish': function () { return false; },
      'onerror': function () { return false; },
    };
  } else {
    this.callbacks = callbacks;
  }

  // Constants used by the entire program.
  this.COMM_FAILURE = 0;
  this.SRV_QUEUE = 1;
  this.MSG_LOGIN = 2;
  this.TEST_PREPARE = 3;
  this.TEST_START = 4;
  this.TEST_MSG = 5;
  this.TEST_FINALIZE = 6;
  this.MSG_ERROR = 7;
  this.MSG_RESULTS = 8;
  this.MSG_LOGOUT = 9;
  this.MSG_WAITING = 10;
  this.MSG_EXTENDED_LOGIN = 11;

  // An array to translate the constants back into strings for debugging.
  this.NDT_MESSAGES = [];
  this.NDT_MESSAGES[this.COMM_FAILURE] = 'COMM_FAILURE';
  this.NDT_MESSAGES[this.SRV_QUEUE] = 'SRV_QUEUE';
  this.NDT_MESSAGES[this.MSG_LOGIN] = 'MSG_LOGIN';
  this.NDT_MESSAGES[this.TEST_PREPARE] = 'TEST_PREPARE';
  this.NDT_MESSAGES[this.TEST_START] = 'TEST_START';
  this.NDT_MESSAGES[this.TEST_MSG] = 'TEST_MSG';
  this.NDT_MESSAGES[this.TEST_FINALIZE] = 'TEST_FINALIZE';
  this.NDT_MESSAGES[this.MSG_ERROR] = 'MSG_ERROR';
  this.NDT_MESSAGES[this.MSG_RESULTS] = 'MSG_RESULTS';
  this.NDT_MESSAGES[this.MSG_LOGOUT] = 'MSG_LOGOUT';
  this.NDT_MESSAGES[this.MSG_WAITING] = 'MSG_WAITING';
  this.NDT_MESSAGES[this.MSG_EXTENDED_LOGIN] = 'MSG_EXTENDED_LOGIN';
}

/**
 * Provide feedback to the console or the DOM.
 * @param {string} logMessage Message to pass to output mechanism.
 * @param {!boolean=} debugging Optional (may be undefined) Determines whether 
 *  to output messages or to operate silently.
 */
NDTjs.prototype.logger = function (logMessage, debugging) {
  debugging = debugging || false;
  if (debugging === true) {
    console.log(logMessage);
  }
};

/**
 * Check that the browser supports the NDT test.
 * @returns {boolean} Browser supports necessary functions for test client.
 */
NDTjs.prototype.checkBrowserSupport = function () {
  if (self.WebSocket === undefined && self.MozWebSocket === undefined) {
    throw this.UnsupportedBrowser('No Websockets');
  }
  return true;
};

/**
 * Makes a login message suitable for sending to the server.  The login
 * messages specifies the tests to be run.
 * @param {number} desiredTests The types of tests requested from the server
 *   signaled based on a bitwise operation of the test ids.
 * @returns {Uint8Array} NDT login message signalling the desired tests.
 */
NDTjs.prototype.makeLoginMessage = function (desiredTests) {
  // We must support TEST_STATUS (16) as a 3.5.5+ client, so we make sure
  // test 16 is desired.
  var i,
    loginMessage = 'XXX { "msg": "v3.5.5", "tests": "' + (desiredTests | 16) +
                   '" }',
    loginData = new Uint8Array(loginMessage.length);

  loginData[0] = this.MSG_EXTENDED_LOGIN;
  loginData[1] = 0;  // Two bytes to represent packet length
  loginData[2] = loginMessage.length - 3;

  for (i = 3; i < loginMessage.length; i += 1) {
    loginData[i] = loginMessage.charCodeAt(i);
  }
  return loginData;
};

/**
 * A generic message creation system for NDT.
 * (messageType, message body length [2], message body)
 * @params {number} messageType The type of message according to NDT's 
 *  specification.
 * @params {string} messageContent The message body.
 * @returns {array} An array of bytes suitable for sending on a binary
 *  websocket.
 */
NDTjs.prototype.makeNdtMessage = function (messageType, messageContent) {
  var messageBody, ndtMessage, i;

  messageBody = '{ "msg": "' + messageContent + '" } ';
  ndtMessage = new Uint8Array(messageBody.length + 3);
  ndtMessage[0] = messageType;
  ndtMessage[1] = (messageBody.length >> 8) & 0xFF;
  ndtMessage[2] = messageBody.length & 0xFF;

  for (i = 0; i < messageBody.length; i += 1) {
    ndtMessage[i + 3] = messageBody.charCodeAt(i);
  }
  return ndtMessage;
};

/**
 * Parses messages received from the NDT server.
 * @param {object} buffer The complete message received from the NDT server.
 * @returns {array} Parsed messaged.
 */
NDTjs.prototype.parseNdtMessage = function (buffer) {
  var i,
    response = [],
    bufferArray = new Uint8Array(buffer),
    message =  String.fromCharCode.apply(null,
                                         new Uint8Array(buffer.slice(3)));
  for (i = 0; i < 3; i += 1) {
    response[i] = bufferArray[i];
  }
  response.push(message);
  return response;
};

/**
 * Exception related to low-level connectivity failures.
 * @param {string} message Specific failure messages passed in the course of
 *  receiving the exception.
 */
NDTjs.prototype.ConnectionException = function (message) {
  this.logger(message);
  this.callbacks.onerror(message);
};

/**
 * Exception related to an unsupported browser.
 * @param {string} message Specific failure messages passed in the course of
 *  receiving the exception.
 */
NDTjs.prototype.UnsupportedBrowser = function (message) {
  this.logger(message);
  this.callbacks.onerror(message);
};

/**
 * Exception related to test failures, such as behavior inconsistent with
 * expectations.
 * @param {string} message Specific failure messages passed in the course of
 *  receiving the exception.
 */
NDTjs.prototype.TestFailureException = function (message) {
  this.logger(message);
  this.callbacks.onerror(message);
};

/**
 * A simple helper function to create websockets consistently.
 * @param {string} serverAddress The FQDN or IP of the NDT server.
 * @param {Number} serverPort The port expected for the NDT test.
 * @param {string} urlPath The path of the resource to request from NDT.
 * @param {string} protocol The WebSocket protocol to build for.
 * @returns {Websocket} The WebSocket we created;
 */
NDTjs.prototype.createWebsocket = function (serverProtocol, serverAddress,
                                            serverPort, urlPath, protocol) {
  var createdWebsocket = new WebSocket(serverProtocol + '://' +
                                       serverAddress + ':' + serverPort +
                                       urlPath, protocol);
  createdWebsocket.binaryType = 'arraybuffer';
  return createdWebsocket;
};

/**
 * NDT's Client-to-Server (C2S) Upload Test
 * Serves as a closure that will process all messages for the C2S NDT test.
 * @returns {boolean} The test is complete and the closure should no longer 
 *    be called.
 */
NDTjs.prototype.ndtC2sTest = function () {
  var serverPort, testConnection, testStart, i,
    dataToSend = new Uint8Array(this.SEND_BUFFER_SIZE),
    that = this,
    state = 'WAIT_FOR_TEST_PREPARE',
    totalSent = 0,
    nextCallback = that.updateInterval,
    keepSendingData;

  for (i = 0; i < dataToSend.length; i += 1) {
    // All the characters must be printable, and the printable range of
    // ASCII is from 32 to 126.  101 is because we need a prime number.
    dataToSend[i] = 32 + (i * 101) % (126 - 32);
  }

  /**
   * The upload function for C2S, encoded as a callback instead of a loop.
   */
  keepSendingData = function () {
    var currentTime = Date.now() / 1000.0;
    // Monitor the buffersize as it sends and refill if it gets too low.
    if (testConnection.bufferedAmount < 8192) {
      testConnection.send(dataToSend);
      totalSent += dataToSend.length;
    }
    if (that.updateInterval && currentTime > (testStart + nextCallback)) {
      that.results.c2sRate = 8 * (totalSent - testConnection.bufferedAmount)
        / 1000 / (currentTime - testStart);
      that.callbacks.onprogress('interval_c2s', that.results);
      nextCallback += that.updateInterval;
      currentTime = Date.now() / 1000.0;
    }

    if (currentTime < testStart + 10) {
      setTimeout(keepSendingData, 0);
    } else {
      return false;
    }
  };

  /**
   * The closure that processes messages on the control socket for the 
   * C2S test.
   */
  return function (messageType, messageContent) {
    that.logger('CALLED C2S with ' + messageType + ' (' +
                that.NDT_MESSAGES[messageType] + ') ' + messageContent.msg +
                ' in state ' + state);
    if (state === 'WAIT_FOR_TEST_PREPARE' &&
        messageType === that.TEST_PREPARE) {
      that.callbacks.onstatechange('preparing_c2s', that.results);
      serverPort = Number(messageContent.msg);
      testConnection = that.createWebsocket(that.serverProtocol, that.server,
                                            serverPort, that.serverPath, 'c2s');
      state = 'WAIT_FOR_TEST_START';
      return false;
    }
    if (state === 'WAIT_FOR_TEST_START' && messageType === that.TEST_START) {
      that.callbacks.onstatechange('running_c2s', that.results);
      testStart = Date.now() / 1000;
      keepSendingData();
      state = 'WAIT_FOR_TEST_MSG';
      return false;
    }
    if (state === 'WAIT_FOR_TEST_MSG' && messageType === that.TEST_MSG) {
      that.results.c2sRate = Number(messageContent.msg);
      that.logger('C2S rate calculated by server: ' + that.results.c2sRate);
      state = 'WAIT_FOR_TEST_FINALIZE';
      return false;
    }
    if (state === 'WAIT_FOR_TEST_FINALIZE' &&
        messageType === that.TEST_FINALIZE) {
      that.callbacks.onstatechange('finished_c2s', that.results);
      state = 'DONE';
      return true;
    }
    that.logger('C2S: State = ' + state + ' type = ' + messageType + '(' +
        that.NDT_MESSAGES[messageType] + ') message = ', messageContent);
  };
};

/**
 * NDT's Server-to-Client (S2C) Download Test
 * Serves as a closure that will process all messages for the S2C NDT test.
 * @param {Websocket} ndtSocket A websocket connection to the NDT server.
 * @returns {boolean} The test is complete and the closure should no longer 
 *    be called.
 */
NDTjs.prototype.ndtS2cTest = function (ndtSocket) {
  var serverPort, testConnection, testStart, testEnd, errorMessage,
    state = 'WAIT_FOR_TEST_PREPARE',
    receivedBytes = 0,
    nextCallback = this.updateInterval,
    that = this;

  /**
  * The closure that processes messages on the control socket for the 
  * C2S test.
  */
  return function (messageType, messageContent) {
    that.logger('CALLED S2C with ' + messageType + ' (' +
                that.NDT_MESSAGES[messageType] + ') in state ' + state);
    if (state === 'WAIT_FOR_TEST_PREPARE' &&
        messageType === that.TEST_PREPARE) {
      that.callbacks.onstatechange('preparing_s2c', that.results);
      serverPort = Number(messageContent.msg);
      testConnection = that.createWebsocket(that.serverProtocol, that.server,
                                            serverPort, that.serverPath, 's2c');
      testConnection.onopen = function () {
        that.logger('Successfully opened S2C test connection.');
        testStart = Date.now() / 1000;
      };
      testConnection.onmessage = function (response) {
        var hdrSize,
          currentTime;
        if (response.data.byteLength < 126) {
          hdrSize = 2;
        } else if (response.data.byteLength < 65536) {
          hdrSize = 4;
        } else {
          hdrSize = 10;
        }
        receivedBytes += (hdrSize + response.data.byteLength);
        currentTime = Date.now() / 1000.0;
        if (that.updateInterval && currentTime > (testStart + nextCallback)) {
          that.results.s2cRate = 8 * receivedBytes / 1000
            / (currentTime - testStart);
          that.callbacks.onprogress('interval_s2c', that.results);
          nextCallback += that.updateInterval;
          currentTime = Date.now() / 1000.0;
        }
      };

      testConnection.onerror = function (response) {
        errorMessage = that.parseNdtMessage(response.data)[3].msg;
        throw that.TestFailureException(errorMessage);
      };

      state = 'WAIT_FOR_TEST_START';
      return false;
    }
    if (state === 'WAIT_FOR_TEST_START' && messageType === that.TEST_START) {
      that.callbacks.onstatechange('running_s2c', that.results);

      state = 'WAIT_FOR_FIRST_TEST_MSG';
      return false;
    }
    if (state === 'WAIT_FOR_FIRST_TEST_MSG' && messageType === that.TEST_MSG) {
      that.logger('Got message: ' + JSON.stringify(messageContent));
      state = 'WAIT_FOR_TEST_MSG_OR_TEST_FINISH';

      if (testEnd === undefined) {
        testEnd = Date.now() / 1000;
      }
      // Calculation per spec, compared between client and server
      // understanding.
      that.results.s2cRate = 8 * receivedBytes / 1000 / (testEnd - testStart);
      that.logger('S2C rate calculated by client: ' + that.results.s2cRate);
      that.logger('S2C rate calculated by server: ' +
                  messageContent.ThroughputValue);
      ndtSocket.send(that.makeNdtMessage(that.TEST_MSG,
                                         String(that.results.s2cRate)));
      return false;
    }
    if (state === 'WAIT_FOR_TEST_MSG_OR_TEST_FINISH' &&
        messageType === that.TEST_MSG) {
      var web100Record = messageContent.msg.split(': '),
        web100Variable = web100Record[0],
        web100Result = web100Record[1].trim();
      that.results[web100Variable] = web100Result;
      return false;
    }
    if (state === 'WAIT_FOR_TEST_MSG_OR_TEST_FINISH' &&
        messageType === that.TEST_FINALIZE) {
      that.callbacks.onstatechange('finished_s2c', that.results);
      that.logger('NDT S2C test is complete: ' +  messageContent.msg);
      return true;
    }
    that.logger('S2C: State = ' + state + ' type = ' + messageType + '(' +
      that.NDT_MESSAGES[messageType] + ') message = ', messageContent);
  };
};

/**
 * NDT's META (S2C) Download Test
 * Serves as a closure that will process all messages for the META NDT test, 
 *    which provides additional data to the NDT results.
 * @param {Websocket} ndtSocket A websocket connection to the NDT server.
 * @returns {boolean} The test is complete and the closure should no longer
 *    be called.
 */
NDTjs.prototype.ndtMetaTest = function (ndtSocket) {
  var errorMessage,
    that = this,
    state = 'WAIT_FOR_TEST_PREPARE';

  return function (messageType, messageContent) {
    if (state === 'WAIT_FOR_TEST_PREPARE' &&
        messageType === that.TEST_PREPARE) {
      that.callbacks.onstatechange('preparing_meta', that.results);
      state = 'WAIT_FOR_TEST_START';
      return false;
    }
    if (state === 'WAIT_FOR_TEST_START' && messageType === that.TEST_START) {
      that.callbacks.onstatechange('running_meta', that.results);
      // Send one piece of meta data and then an empty meta data packet
      ndtSocket.send(that.makeNdtMessage(that.TEST_MSG,
                                         'client.os.name:NDTjs'));
      ndtSocket.send(that.makeNdtMessage(that.TEST_MSG, ''));
      state = 'WAIT_FOR_TEST_FINALIZE';
      return false;
    }
    if (state === 'WAIT_FOR_TEST_FINALIZE' &&
        messageType === that.TEST_FINALIZE) {
      that.callbacks.onstatechange('finished_meta', that.results);
      that.logger('NDT META test complete.');
      return true;
    }
    errorMessage = 'Bad state and message combo for META test: ' +
      state + ', ' + messageType + ', ' + messageContent.msg;
    throw that.TestFailureException(errorMessage);
  };
};

/**
 * Start a series of NDT tests.
 **/
NDTjs.prototype.startTest = function () {
  var ndtSocket, activeTest, state, errorMessage, i,
    testsToRun = [],
    that = this;

  this.checkBrowserSupport();
  this.logger('Test started.  Waiting for connection to server...');
  this.callbacks.onstart(this.server);
  ndtSocket = this.createWebsocket(this.serverProtocol, this.server,
                                   this.serverPort, this.serverPath, 'ndt');

  /** When the NDT control socket is opened, send a message requesting a
   * TEST_S2C, TEST_C2S, and TEST_META.
   */
  ndtSocket.onopen = function () {
    that.logger('Opened connection on port ' + that.serverPort);
    // Sign up for every test except for TEST_MID and TEST_SFW - browsers can't
    // open server sockets, which makes those tests impossible, because they
    // require the server to open a connection to a port on the client.
    ndtSocket.send(that.makeLoginMessage(2 | 4 | 32));
    state = 'LOGIN_SENT';
  };

  /** Process a message received on the NDT control socket.  The message should
   * be handed off to the active test (if any), or processed directly if there
   * is no active test.
   */
  ndtSocket.onmessage = function (response) {
    var tests,
      message = that.parseNdtMessage(response.data),
      messageType = message[0],
      messageContent = JSON.parse(message[3]);

    that.logger('type = ' + messageType + ' (' +
                that.NDT_MESSAGES[messageType] + ') body = "' +
                messageContent.msg + '"');
    if (activeTest === undefined && testsToRun.length > 0) {
      activeTest = testsToRun.pop();
    }
    if (activeTest !== undefined) {
      // Pass the message to the sub-test
      that.logger('Calling a subtest');
      if (activeTest(messageType, messageContent) === true) {
        activeTest = undefined;
        that.logger('Subtest complete');
      }
      return;
    }

    // If there is an active test, hand off control to the test
    // Otherwise, move the coordinator state forwards.
    if (state === 'LOGIN_SENT') {
      // Response to NDT_LOGIN should be SRV_QUEUE messages until we
      // get SRV_QUEUE('0')
      if (messageType === that.SRV_QUEUE) {
        if (messageContent.msg === '9990') {
          // Connection keepalive message
          ndtSocket.send(that.makeNdtMessage(that.MSG_WAITING, ''));
        } else if (messageContent.msg === '9977') {
          // Test failed, leave now.
          throw that.TestFailureException('Server terminated test ' +
            'with SRV_QUEUE 9977');
        }
        that.logger('Got SRV_QUEUE. Ignoring and waiting for ' +
          'MSG_LOGIN.');
      } else if (messageType === that.MSG_LOGIN) {
        if (messageContent.msg[0] !== 'v') {
          that.logger('Bad msg ' + messageContent.msg);
        }
        state = 'WAIT_FOR_TEST_IDS';
      } else {
        errorMessage = 'Expected type 1 (SRV_QUEUE) or 2 (MSG_LOGIN)' +
            'but got ' + messageType + ' (' +
            that.NDT_MESSAGES[messageType] + ')';
        throw that.TestFailureException(errorMessage);
      }
    } else if (state === 'WAIT_FOR_TEST_IDS' &&
               messageType === that.MSG_LOGIN) {
      tests = messageContent.msg.split(' ');
      for (i = tests.length - 1; i >= 0; i -= 1) {
        if (tests[i] === '2') {
          testsToRun.push(that.ndtC2sTest());
        } else if (tests[i] === '4') {
          testsToRun.push(that.ndtS2cTest(ndtSocket));
        } else if (tests[i] === '32') {
          testsToRun.push(that.ndtMetaTest(ndtSocket));
        } else if (tests[i] !== '') {
          errorMessage = 'Unknown test type: ' + tests[i];
          throw that.TestFailureException(errorMessage);
        }
      }
      state = 'WAIT_FOR_MSG_RESULTS';
    } else if (state === 'WAIT_FOR_MSG_RESULTS' &&
               messageType === that.MSG_RESULTS) {
      that.logger(messageContent);
      var lines = messageContent.msg.split('\n');
      for (i = 0; i < lines.length; i++) {
          var line = lines[i];
          var record = line.split(': ');
          var variable = record[0];
          var result = record[1];
          that.results[variable] = result;
      }
    } else if (state === 'WAIT_FOR_MSG_RESULTS' &&
               messageType === that.MSG_LOGOUT) {
      ndtSocket.close();
      that.callbacks.onstatechange('finished_all', that.results);
      that.callbacks.onfinish(that.results);
      that.logger('All tests successfully completed.');
    } else {
      errorMessage = 'No handler for message ' + messageType +
          ' in state ' + state;
      throw that.TestFailureException(errorMessage);
    }
  };

  ndtSocket.onerror = function (response) {
    errorMessage = that.parseNdtMessage(response.data)[3].msg;
    throw that.TestFailureException(errorMessage);
  };
};
