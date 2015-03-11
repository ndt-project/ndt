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
  import flash.display.Sprite;
  import flash.events.Event;
  import mx.resources.ResourceBundle;
  import mx.utils.URLUtil;
  /**
   * @author Anant Subramanian
   */

  [ResourceBundle("DisplayMessages")]
  public class Main extends Sprite {
    public static var guiEnabled:Boolean = CONFIG::guiEnabled;
    public static var locale:String = CONFIG::defaultLocale;
    public static var gui:GUI;
    public static var server_hostname:String = NDTConstants.SERVER_HOSTNAME;
    public static var client_application:String = NDTConstants.CLIENT_ID;
    public static var ndt_description:String = NDTConstants.NDT_DESCRIPTION;
    public static var jsonSupport:Boolean = true;
    public static var bad_runtime_action:String = "warn";
    //Options are warn, warn-limit, and error (default)

    public function Main():void {
      if (stage)
        init();
      else
        addEventListener(Event.ADDED_TO_STAGE, init);
    }

    /**
     * Function that is called once the stage is initialized and an instance of
     * this class has been added to it. Sets the locale value according to the
     * SWF environment and creates an NDTPController object to start an NDT test
     */
    private function init(e:Event = null):void {
      removeEventListener(Event.ADDED_TO_STAGE, init);
      var selfUrl:String = URLUtil.getServerName(this.loaderInfo.url);
      if (selfUrl) {
        server_hostname = selfUrl;
      }

      // Set the properties of the SWF from HTML tags.
      NDTUtils.initializeFromHTML(this.root.loaderInfo.parameters);

      //User Agent string is stored in a TestResults var by initFromHTML
      if (UserAgentTools.goodRuntimeCheck(TestResults.ndt_test_results::userAgent)) {
        bad_runtime_action = NDTConstants.ENV_OK;
      } else
        client_application = NDTConstants.LIMITED_CLIENT_ID; 

      var frame:NDTPController = NDTPController.getInstance();

      stage.showDefaultContextMenu = false;
      if (guiEnabled) {
        gui = new GUI(stage.stageWidth, stage.stageHeight, frame);
        this.addChild(gui);
      }
      NDTUtils.addJSCallbacks();
    }

    public static function getHost():String {
      return server_hostname;
    }
    
    /**
     * Function that initializes the NDT server variable set directly through JS.
     */
    public static function setHost(hostname:String):String {
        var js_server_hostname:String = hostname;
        if (js_server_hostname) {
          server_hostname = js_server_hostname;
          TestResults.appendDebugMsg(
            "Initialized server from JavaScript. Server hostname:"
            + Main.server_hostname);
        } else {
	  js_server_hostname = null;
	}
	return js_server_hostname;
    }
  }
}
