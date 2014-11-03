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

package {
  /**
   * Class that defines the NDTP control message types, as expected by the NDT
   * server.
   */
  public class MessageType {
    public static const UNDEF_TYPE:int = -1;
    public static const COMM_FAILURE:int = 0;
    public static const SRV_QUEUE:int = 1;
    public static const MSG_LOGIN:int = 2;
    public static const TEST_PREPARE:int = 3;
    // TODO(tiziana): Add check in Message.sendMessage for length constrains.
    public static const TEST_START:int = 4;
    public static const TEST_MSG:int = 5;
    public static const TEST_FINALIZE:int = 6;
    public static const MSG_ERROR:int = 7;
    public static const MSG_RESULTS:int = 8;
    public static const MSG_LOGOUT:int = 9;
    public static const MSG_WAITING:int = 10;
    public static const MSG_EXTENDED_LOGIN:int = 11;
  }
}

