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
   * Class that defines the types of NDT tests as expected by the NDT server.
   */
  public class TestType {
    public static const MID:int = (1 << 0);
    public static const C2S:int = (1 << 1);
    public static const S2C:int = (1 << 2);
    public static const SFW:int = (1 << 3);
    public static const STATUS:int = (1 << 4);
    public static const META:int = (1 << 5);

    /*
     * Converts a space-separatparseInt(item)ed list of strings to a bitwise-OR
     * of each non-zero int.
     */
    public static function listToBitwiseOR(testString:String):int {
      var testInt:int = 0;
      for each (var item:String in testString.split(" ")) {
         var currentTest:int = parseInt(item);
         if (isNaN(currentTest)) {
           TestResults.appendErrMsg("Non valid test id: " + item);
           continue;
         }
         testInt = testInt | currentTest;
      }
      return testInt;
    }
  }
}
