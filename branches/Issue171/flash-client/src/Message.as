// Copyright 2013 M-Lab
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package  {
  import flash.errors.IOError;
  import flash.net.Socket;
  import flash.utils.ByteArray;

  /**
   * Class that defines an NDT message. A message is characterized by a type,
   * a length and a body. All the types are defined in the class MessageType.
   * All the valid types are defined in the class MessageType.
   * The client and server exchange messages as sequence of bytes, where
   * - the 1st byte contains the type of a message,
   * - the 2nd and 3rd bytes contain the length of the message.
   */
  public class Message {
    private var _type:int;
    private var _body:ByteArray;
    private var _length:int;

    public function Message(
        type:int=MessageType.UNDEF_TYPE, body:ByteArray=null,
        jsonFormat:Boolean = false):void {
      _type = type;
      _body = new ByteArray();
      if (body != null) {
        if (jsonFormat) {
          _body.writeUTFBytes(NDTUtils.createJsonFromSingleValue(
                              String(body)));
        } else {
          _body.writeBytes(body);
        }
      } else if(Main.jsonSupport) {
        _body.writeUTFBytes(NDTUtils.createJsonFromSingleValue(""));
      }
      _length = _body.length;
    }

    public function get type():int {
      return _type;
    }

    public function get length():int {
      return _length;
    }

    public function get body():ByteArray {
      return _body;
    }

    public function readHeader(socket:Socket):Boolean {
      if (socket.bytesAvailable < NDTConstants.MSG_HEADER_LENGTH)
        return false;

      var header:ByteArray = new ByteArray();
      var bytesRead:int = NDTUtils.readBytes(
          socket, header, 0, NDTConstants.MSG_HEADER_LENGTH);
      if(bytesRead != NDTConstants.MSG_HEADER_LENGTH) {
        // This case should never occurr.
        TestResults.appendErrMsg("Error reading header from socket. Only "
                                 + bytesRead + " bytes read");
        return false;
      }
      _type = header[0]
      _length = (int(header[1]) & 0xFF) << 8;
      _length += int(header[2]) & 0xFF;
      return true;
    }

    public function readBody(socket:Socket, bytesToRead:int):Boolean {
      if (socket.bytesAvailable < bytesToRead)
        return false;

      _body = new ByteArray();
      var bytesRead:int = NDTUtils.readBytes(socket, _body, 0, bytesToRead);
      if(bytesRead != bytesToRead) {
        // This case should never occurr.
        TestResults.appendErrMsg("Error reading body from socket. Only "
                                 + bytesRead + " bytes read.");
        return false;
      }
      return true;
    }

    public function sendMessage(socket:Socket):Boolean {
      var header:ByteArray = new ByteArray();
      header[0] = _type;
      header[1] = (_length >> 8);
      header[2] = _length;
	  
      try {
        socket.writeBytes(header);
      } catch(e:IOError) {
        TestResults.appendErrMsg("Error writing header to socket: " + e);
        return false;
      }
      try {
        socket.writeBytes(_body);
      } catch(e:IOError) {
        TestResults.appendErrMsg("Error writing body to socket: " + e);
        return false;
      }
      try {
        socket.flush();
      } catch(e:IOError) {
        TestResults.appendErrMsg("Error flushing socket: " + e);
        return false;
      }
      return true;
    }
  }
}

