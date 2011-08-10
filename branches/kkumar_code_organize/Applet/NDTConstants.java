import java.util.Locale;
import java.util.ResourceBundle;

import javax.swing.JOptionPane;

/**
 * 
 */

/**
 * @author kkumar
 * Class to hold constants, mostly those that are "non-protocol" related 
 * The different sections of constants are enlisted under appropriate "sections"
 * If necessary, these sections can be broken off later into different files for
 * better readability
 * TODO: Revisit documentation. Currently also used to record change history
 * 
 * Changed access specifier of constants to public from private. These are static message
 *  types and not data members, and having public access privilege will not affect 
 *  encapsulation
 */
public class NDTConstants {
	
	 //Section: System variables 
	  //used by the META tests
	  public static final String META_CLIENT_OS = "client.os.name";
	  public static final String META_BROWSER_OS = "client.browser.name";
	  public static final String META_CLIENT_KERNEL_VERSION = "client.kernel.version";
	  public static final String META_CLIENT_VERSION = "client.version";
	
	 //TODO Check if version could be moved to some "configurable" or "property" area
	  public static final String VERSION = "3.6.4";
	  
	  //Section: Test type
	  //TODO: TestType class?
	  public static final byte TEST_MID = (1 << 0);
	  public static final byte TEST_C2S = (1 << 1);
	  public static final byte TEST_S2C = (1 << 2);
	  public static final byte TEST_SFW = (1 << 3);
	  public static final byte TEST_STATUS = (1 << 4);
	  public static final byte TEST_META = (1 << 5);
	  
	  //Section: Firewall test status
	  public static final int SFW_NOTTESTED  = 0;
	  public static final int SFW_NOFIREWALL = 1;
	  public static final int SFW_UNKNOWN    = 2;
	  public static final int SFW_POSSIBLE   = 3;

	  public static final double VIEW_DIFF = 0.1;
	  
	  
	  /*Method to initialise a few constants */
	  private static ResourceBundle messages;
	  public static String TCPBW100_MSGS = "Tcpbw100_msgs";
	  
	  public static void initConstants(Locale paramLocale) {	  
		  try {	         
	          messages = ResourceBundle.getBundle( TCPBW100_MSGS, paramLocale);
	          System.out.println("Obtained messages ");
	      } catch (Exception e) {
	          JOptionPane.showMessageDialog(null, "Error while loading language files:\n" + e.getMessage());
	          e.printStackTrace();
	      }
		  
	  } //end method
	  
	  public static void initConstants(String lang, String country) {	  
		  try {	       
			  Locale locale = new Locale(lang, country);
			  messages = ResourceBundle.getBundle("Tcpbw100_msgs", locale);
		  } catch (Exception e) {
	          JOptionPane.showMessageDialog(null, "Error while loading language files:\n" + e.getMessage());
	          e.printStackTrace();
	      }
	  }//end method initconstants
	  
	  
	  public static String getMessageString(String paramStrName) {
		  return messages.getString(paramStrName);
	  }

}
