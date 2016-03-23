/* This is an NDT client, written in javascript.  It speaks the websocket
 * version of the NDT protocol.  The NDT protocol is documented at:
 * https://code.google.com/p/ndt/wiki/NDTProtocol
 */

/*jslint bitwise: true, browser: true, nomen: true, vars: true */
/*global Uint8Array, WebSocket */

'use strict';

importScripts('ndt-browser-client.js');

self.addEventListener('message', function (e) {
  var msg = e.data;
  switch (msg.cmd) {
    case 'start':
      startNDT(msg.hostname, msg.port, msg.protocol, msg.path,
               msg.update_interval);
      break;
    case 'stop':
      self.close();
      break;
    default:
      // self.postMessage('Unknown command: ' + data.msg);
      break;
  };
}, false);

function startNDT(hostname, port, protocol, path, update_interval) {
  var callbacks = {
    'onstart': function(server) {
      self.postMessage({
        'cmd': 'onstart',
        'server': server
      });
    },
    'onstatechange': function(state, results) {
      self.postMessage({
        'cmd': 'onstatechange',
        'state': state,
        'results': results,
      });
    },
    'onprogress': function(state, results) {
      self.postMessage({
        'cmd': 'onprogress',
        'state': state,
        'results': results,
      });
    },
    'onfinish': function(results) {
      self.postMessage({
        'cmd': 'onfinish',
        'results': results
      });
    },
    'onerror': function(error_message) {
      self.postMessage({
        'cmd': 'onerror',
        'error_message': error_message
      });
    }
  };

  var client = new NDTjs(hostname, port, protocol, path, callbacks,
                         update_interval);
  client.startTest();
}
