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
  import flash.errors.EOFError;
  import flash.errors.IOError;
  import flash.external.ExternalInterface;
  import flash.globalization.LocaleID;
  import flash.system.Capabilities;
  import flash.system.Security;
  import flash.net.Socket;
  import flash.utils.ByteArray;
  import mx.resources.ResourceManager;

  public class NDTUtils {
    /**
     * Function that calls a JS function through the ExternalInterface class if
     * it exists by the name specified in the parameter.
     * @param {String} functionName The name of the JS function to call.
     * @param {...} args A variable length array that contains the parameters to
     *     pass to the JS function
     */
    public static function callExternalFunction(
        functionName:String, ... args):void {
      if (!ExternalInterface.available)
        return;
      try {
        switch (args.length) {
          case 0: ExternalInterface.call(functionName);
                  break;
          case 1: ExternalInterface.call(functionName, args[0]);
                  break;
          case 2: ExternalInterface.call(functionName, args[0], args[1]);
                  break;
        }
      } catch (e:Error) {
        // TODO(tiziana): Find out why ExternalInterface.available does not work
        // in some cases and this exception is raised.
      }
    }
    /**
     * Function that reads the HTML parameter tags for the SWF file and
     * initializes the variables in the SWF accordingly.
     */
    public static function initializeFromHTML(paramObject:Object):void {
      if (NDTConstants.HTML_LOCALE in paramObject) {
        Main.locale = paramObject[NDTConstants.HTML_LOCALE];
        TestResults.appendDebugMsg("Initialized locale from HTML. Locale: "
                                   + Main.locale);
      } else {
        initializeLocale();
      }

      if (NDTConstants.HTML_SERVER_HOSTNAME in paramObject) {
        Main.server_hostname = paramObject[NDTConstants.HTML_SERVER_HOSTNAME];
        TestResults.appendDebugMsg("Initialized server from HTML. Server: "
                                   + Main.server_hostname);
      }
      // else keep the default value (NDTConstants.SERVER_HOSTNAME).

      if (!ExternalInterface.available)
        return;

      try {
        TestResults.ndt_test_results::userAgent = ExternalInterface.call(
            "window.navigator.userAgent.toString");
        TestResults.appendDebugMsg(
            "Initialized useragent from JavaScript. Useragent:"
            + TestResults.ndt_test_results::userAgent);
      } catch(e:Error) {
        // TODO(tiziana): Find out why ExternalInterface.available does not work
        // in some cases and this exception is raised.
      }

      try {
        var js_server_hostname:String = ExternalInterface.call("getNDTServer");
        if (js_server_hostname) {
          Main.server_hostname = js_server_hostname;
          TestResults.appendDebugMsg(
            "Initialized server from JavaScript. Server hostname:"
            + Main.server_hostname);
        }
      } catch(e:Error) {
        TestResults.appendDebugMsg("Bad Flash permissions: No "
          + "access to javascript.");
      }

      try {
        var js_client_application:String =
	  ExternalInterface.call("getClientApplication");
        if (js_client_application) {
          Main.client_application = js_client_application;
          TestResults.appendDebugMsg(
            "Initialized client application from JavaScript. " +
	    "Client application: "
            + Main.client_application);
        }
      } catch(e:Error) {
        TestResults.appendDebugMsg("Bad Flash permissions: No "
          + "access to javascript.");
      }

      try {
        Main.ndt_description = ExternalInterface.call("getNDTDescription");
        TestResults.appendDebugMsg(
            "Initialized NDT description from JavaScript:"
            + Main.ndt_description);
      } catch(e:Error) {
        Main.ndt_description = NDTConstants.NDT_DESCRIPTION;
      }
    }

    /**
     * Initializes the locale used by the tool to match the environment of the
     * SWF.
     */
    public static function initializeLocale():void {
      var localeId:LocaleID = new LocaleID(Capabilities.language);
      var lang:String = localeId.getLanguage();
      var region:String = localeId.getRegion();
      if (lang != null && region != null
          && (ResourceManager.getInstance().getResourceBundle(
                lang + "_" + region, NDTConstants.BUNDLE_NAME) != null)) {
        // Bundle for specified locale found, change value of locale.
        Main.locale = new String(lang + "_" + region);
        TestResults.appendDebugMsg(
            "Initialized locale from Flash config. Locale: " + Main.locale);
      } else {
        TestResults.appendDebugMsg(
            "Not found ResourceBundle for locale requested in Flash config. " +
            "Using default locale: " + CONFIG::defaultLocale);
      }
    }

    /**
     * Function that adds the callbacks to allow data access from, and to allow
     * data to be sent to JavaScript.
     */
    public static function addJSCallbacks():void {
      if (!ExternalInterface.available)
        return;
      // TODO(tiziana): Restrict domain to the M-Lab website/server.
      Security.allowDomain("*");
      try {
        ExternalInterface.addCallback(
            "run_test", NDTPController.getInstance().startNDTTest);
        ExternalInterface.addCallback(
            "get_status", TestResults.getTestStatus);
        ExternalInterface.addCallback(
            "getDebugOutput", TestResults.getDebugMsg);
        ExternalInterface.addCallback(
            "get_diagnosis", TestResults.getResultDetails);
        ExternalInterface.addCallback(
            "get_errmsg", TestResults.getErrStatus);
        ExternalInterface.addCallback("get_host", Main.getHost);
        ExternalInterface.addCallback(
            "getNDTvar", TestResultsUtils.getNDTVariable);
      } catch (e:Error) {
        // TODO(tiziana): Find out why ExternalInterface.available does not work
        // in some cases and this exception is raised.
      } catch (e:SecurityError) {
        TestResults.appendErrMsg("Security error when adding callbacks: " + e);
      }
    }

    /**
     * Reads bytes from a socket into a ByteArray and returns the number of
     * successfully read bytes.
     * @param {Socket} socket Socket object to read from.
     * @param {ByteArray} bytes ByteArray where to store the read bytes.
     * @param {uint} offset Position of the ByteArray from where to start
                            storing the read values.
     * @param {uint} byteToRead Number of bytes to read.
     * @return {int} Number of successfully read bytes.
     */
    public static function readBytes(socket:Socket, bytes:ByteArray,
                                     offset:uint, bytesToRead:uint):int {
      var bytesRead:int = 0;
      while (socket.bytesAvailable && bytesRead < bytesToRead) {
        try {
          bytes[bytesRead + offset] = socket.readByte();
        } catch (e:IOError) {
          TestResults.appendErrMsg("Error reading byte from socket: " + e);
          break;
        } catch(error:EOFError) {
          // No more data to read from the socket.
          break;
        }
        bytesRead++;
      }
      return bytesRead;
    }

    public static function readAllBytesAndDiscard(socket:Socket):int {
      var bytesCount:int = 0;
      while (socket.bytesAvailable) {
        try {
          socket.readByte();
          bytesCount++;
        } catch (e:IOError) {
          TestResults.appendErrMsg("Error reading byte from socket: " + e);
          break;
        } catch(error:EOFError) {
          // No more data to read from the socket.
          break;
        }
      }
      return bytesCount;
    }
  }
}

