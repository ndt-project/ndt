// This class has code taken from
// http://nerds.palmdrive.net/useragent/code.html
// In that page the author of the code states that the code is free for all the
// uses and he believes it is too small to be copyrightable.

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
  import mx.resources.ResourceManager;
  import mx.utils.StringUtil;
  import flash.system.Capabilities;
  /**
   * This class is use to obtain information about who is accessing a web-server.
   * When a web browser accesses a web-server, it usually transmits a "User-Agent"
   * string. This is expected to include the name and versions of the browser
   * and the underlying Operating System. Though the information inside a user-
   * agent string is not restricted to these alone, currently NDT uses this to
   * get Browser OS only.
   */
  public class UserAgentTools {
    private static function isLower(sParam:String):Boolean {
      return (sParam == sParam.toLowerCase());
    }

    private static function isLetter(sParam:String):Boolean {
      return (sParam >= "A" && sParam <= "z");
    }

    private static function isDigit(sParam:String):Boolean {
      return (sParam >= "0" && sParam <= "9");
    }

    public static function getFirstVersionNumber(a_userAgent:String,
                                                 a_position:int,
                                                 numDigits:int):String {
      var ver:String = getVersionNumber(a_userAgent, a_position);

      if (ver == null)
        return "";
      var i:int = 0;
      var res:String = "";
      while (i < ver.length && i < numDigits) {
        res += (ver.charAt(i)).toString();
        i++;
      }
      return res;
    }

    /*
      Example UserAgent String:
      Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)
       Chrome/29.0.1547.2 Safari/537.36
    */
    public static function getVersionNumber(a_userAgent:String,
                                            a_position:int):String {
      if(a_position < 0)
        return "";

      var res:String = new String();
      var status:int = 0;
      while (a_position < a_userAgent.length) {
        var c:String = a_userAgent.charAt(a_position);
        switch(status) {
          case 0: // No valid digits encountered yet
            if (c == ' ' || c == '/')
              break;
            if (c == ';' || c == ')')
              return "";
            status = 1;
          case 1: // Version number in progress
            if (c == ';' || c == '/' || c == ')' || c == '(' || c == '[')
              return StringUtil.trim(res);
            if (c == ' ')
              status = 2;
            res = new String(res + c);
            break;
          case 2: // Space encountered - Might need to end the parsing
            if ((isLetter(c) && isLower(c)) || isDigit(c)) {
              res = new String(res + c);
              status = 1;
            } else return StringUtil.trim(res);
            break;
        }
        a_position++;
      }
      return StringUtil.trim(res);
    }

    public static function getBotName(userAgent:String):Array {
      userAgent = userAgent.toLowerCase();
      var pos:int = 0;
      var res:String = null;
      if ((pos = userAgent.indexOf("help.yahoo.com/")) > -1) {
        res = "Yahoo";
        pos += 7;
      } else if ((pos = userAgent.indexOf("google/")) > -1) {
        res = "Google";
        pos += 7;
      } else if ((pos = userAgent.indexOf("msnbot/")) > -1) {
        res = "MSNBot";
        pos += 7;
      } else if ((pos = userAgent.indexOf("googlebot/")) > -1) {
        res = "Google";
        pos += 10;
      } else if ((pos = userAgent.indexOf("webcrawler/")) > -1) {
        res = "WebCrawler";
        pos += 11;
      } else if ((pos = userAgent.indexOf("inktomi")) > -1) {
        // The following two bots don't have any
        // version number in their User-Agent strings.
        res = "Inktomi";
        pos = -1;
      } else if ((pos = userAgent.indexOf("teoma")) > -1) {
        res = "Teoma";
        pos = -1;
      }
      if(res == null)
        return null;
      return [res, res, res + getVersionNumber(userAgent, pos)];
    }

    public static function getBrowser(userAgent:String):Array {
      if(userAgent == null) {
        return ["?", "?", "?"];
      }
      var botName:Array = new Array();
      if((botName = getBotName(userAgent)) != null)
        return botName;

      var res:Array = null;
      var pos:int;
      if ((pos = userAgent.indexOf("Lotus-Notes/")) > -1) {
        res = ["LotusNotes", "LotusNotes", "LotusNotes"
            + getVersionNumber(userAgent, pos + 12)];
      } else if ((pos = userAgent.indexOf("Opera")) > -1) {
        var ver:String = getVersionNumber(userAgent, pos + 5);
        res = ["Opera",
            "Opera" + getFirstVersionNumber(userAgent, pos + 5, 1),
            "Opera" + ver];
        if ((pos = userAgent.indexOf("Opera Mini/")) > -1) {
          var ver2:String = getVersionNumber(userAgent, pos + 11);
          res = ["Opera", "Opera Mini", "Opera Mini " + ver2];
        } else if ((pos = userAgent.indexOf("Opera Mobi/")) > -1) {
          var vers2:String = getVersionNumber(userAgent, pos + 11);
          res = ["Opera", "Opera Mobi", "Opera Mobi " + vers2];
        }
      } else if (userAgent.indexOf("MSIE") > -1) {
        if ((pos = userAgent.indexOf("MSIE 6.0")) > -1) {
          res = ["MSIE", "MSIE6",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 5.0")) > -1) {
          res = ["MSIE", "MSIE5",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 5.5")) > -1) {
          res = ["MSIE", "MSIE5.5",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 5.")) > -1) {
          res = ["MSIE", "MSIE5.x",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 4")) > -1) {
          res = ["MSIE", "MSIE4",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 7")) > -1
            && userAgent.indexOf("Trident/4.0") < 0) {
          res = ["MSIE", "MSIE7",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 8")) > -1
            || userAgent.indexOf("Trident/4.0") > -1) {
          res = ["MSIE", "MSIE8",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else if ((pos = userAgent.indexOf("MSIE 9")) > -1
            || userAgent.indexOf("Trident/4.0") > -1) {
          res = ["MSIE", "MSIE9",
              "MSIE" + getVersionNumber(userAgent, pos + 4)];
        } else
          res = ["MSIE", "MSIE?", "MSIE?"
                         + getVersionNumber(userAgent,
                      userAgent.indexOf("MSIE") + 4)];
      } else if ((pos = userAgent.indexOf("Gecko/")) > -1) {
        res = ["Gecko", "Gecko",
            "Gecko" + getFirstVersionNumber(userAgent, pos + 5, 4)];
        if ((pos = userAgent.indexOf("Camino/")) > -1) {
          res[1] += "(Camino)";
          res[2] += "(Camino" + getVersionNumber(userAgent, pos + 7)
              + ")";
        } else if ((pos = userAgent.indexOf("Chimera/")) > -1) {
          res[1] += "(Chimera)";
          res[2] += "(Chimera" + getVersionNumber(userAgent, pos + 8)
              + ")";
        } else if ((pos = userAgent.indexOf("Firebird/")) > -1) {
          res[1] += "(Firebird)";
          res[2] += "(Firebird" + getVersionNumber(userAgent, pos + 9)
              + ")";
        } else if ((pos = userAgent.indexOf("Phoenix/")) > -1) {
          res[1] += "(Phoenix)";
          res[2] += "(Phoenix" + getVersionNumber(userAgent, pos + 8)
              + ")";
        } else if ((pos = userAgent.indexOf("Galeon/")) > -1) {
          res[1] += "(Galeon)";
          res[2] += "(Galeon" + getVersionNumber(userAgent, pos + 7)
              + ")";
        } else if ((pos = userAgent.indexOf("Firefox/")) > -1) {
          res[1] += "(Firefox)";
          res[2] += "(Firefox" + getVersionNumber(userAgent, pos + 8)
              + ")";
        } else if ((pos = userAgent.indexOf("Netscape/")) > -1) {
          if ((pos = userAgent.indexOf("Netscape/6")) > -1) {
            res[1] += "(NS6)";
            res[2] += "(NS" + getVersionNumber(userAgent, pos + 9)
                + ")";
          } else if ((pos = userAgent.indexOf("Netscape/7")) > -1) {
            res[1] += "(NS7)";
            res[2] += "(NS" + getVersionNumber(userAgent, pos + 9)
                + ")";
          } else if ((pos = userAgent.indexOf("Netscape/8")) > -1) {
            res[1] += "(NS8)";
            res[2] += "(NS" + getVersionNumber(userAgent, pos + 9)
                + ")";
          } else if ((pos = userAgent.indexOf("Netscape/9")) > -1) {
            res[1] += "(NS9)";
            res[2] += "(NS" + getVersionNumber(userAgent, pos + 9)
                + ")";
          } else {
            res[1] += "(NS?)";
            res[2] += "(NS?"
                + getVersionNumber(userAgent,
                    userAgent.indexOf("Netscape/") + 9) + ")";
          }
        }
      } else if ((pos = userAgent.indexOf("Netscape/")) > -1) {
        if ((pos = userAgent.indexOf("Netscape/4")) > -1) {
          res = ["NS", "NS4",
              "NS" + getVersionNumber(userAgent, pos + 9)];
        } else
          res = ["NS", "NS?",
              "NS?" + getVersionNumber(userAgent, pos + 9)];
      } else if ((pos = userAgent.indexOf("Chrome/")) > -1) {
        res = ["KHTML", "KHTML(Chrome)", "KHTML(Chrome"
            + getVersionNumber(userAgent, pos + 6) + ")"];
      } else if ((pos = userAgent.indexOf("Safari/")) > -1) {
        res = ["KHTML", "KHTML(Safari)", "KHTML(Safari"
            + getVersionNumber(userAgent, pos + 6) + ")"];
      } else if ((pos = userAgent.indexOf("Konqueror/")) > -1) {
        res = ["KHTML", "KHTML(Konqueror)", "KHTML(Konqueror"
            + getVersionNumber(userAgent, pos + 9) + ")"];
      } else if ((pos = userAgent.indexOf("KHTML")) > -1) {
        res = ["KHTML", "KHTML?",
            "KHTML?(" + getVersionNumber(userAgent, pos + 5) + ")"];
      } else if ((pos = userAgent.indexOf("NetFront")) > -1) {
        res = ["NetFront", "NetFront", "NetFront "
            + getVersionNumber(userAgent, pos + 8)];
      } else if ((pos = userAgent.indexOf("BlackBerry")) > -1) {
        pos = userAgent.indexOf("/", pos + 2);
        res = ["BlackBerry", "BlackBerry", "BlackBerry"
            + getVersionNumber(userAgent, pos + 1)];
      } else if (userAgent.indexOf("Mozilla/4.") == 0
          && userAgent.indexOf("Mozilla/4.0") < 0
          && userAgent.indexOf("Mozilla/4.5 ") < 0) {
        // We will interpret Mozilla/4.x as Netscape Communicator is and only if
        // x is not 0 or 5
        res = ["Communicator", "Communicator", "Communicator"
            + getVersionNumber(userAgent, pos + 8)];
      } else
        return ["?", "?", "?"];
      return res;
    }
    public static function goodRuntimeCheck(userAgent:String):Boolean {
       if (getBrowser(userAgent)[1] == 'KHTML(Chrome)') {
         return true;
       }
       var os:String = flash.system.Capabilities.os.substr(0, 3);
       if (os == "Win") {
         return true;
       }
       else 
         return false;
    }
  }
}

