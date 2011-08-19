import java.util.Locale;
import java.util.ResourceBundle;

import javax.swing.JOptionPane;

/**
 * 
 */

/**
 *
 * Class to hold constants, mostly those that are "non-protocol" related 
 * The different sections of constants are enlisted under appropriate "sections"
 * If necessary, these sections can be broken off later into different files for
 * better readability
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

	//
	public static String TARGET1 = "U";
	public static String TARGET2 = "H";

	//NDT pre-fixed port ID
	public static final int CONTROL_PORT_DEFAULT = 3001;

	//SRV-QUEUE message status constants
	public static final int SRV_QUEUE_TEST_STARTS_NOW = 0;
	public static final int SRV_QUEUE_SERVER_FAULT = 9977;
	public static final int SRV_QUEUE_SERVER_BUSY = 9988;
	public static final int SRV_QUEUE_HEARTBEAT = 9990 ;
	public static final int SRV_QUEUE_SERVER_BUSY_60s = 9999;

	//Middlebox test related constants
	public static final int MIDDLEBOX_PREDEFINED_MSS = 8192;//8k buffer size
	public static final int ETHERNET_MTU_SIZE = 1456;

	//SFW test related constants
	public static final String SFW_PREDEFINED_TEST_MESSAGE = "Simple firewall test";


	private static ResourceBundle _rscBundleMessages;
	public static String TCPBW100_MSGS = "Tcpbw100_msgs";
	public static int PREDEFINED_BUFFER_SIZE = 8192; //8k buffer size

	//Data rate indicator strings
	public static String T1_STR =  "T1";
	public static String T3_STR =  "T3";
	public static String ETHERNET_STR =  "Ethernet";
	public static String FAST_ETHERNET = "FastE";
	public static String OC_12_STR = "OC-12";
	public static String GIGABIT_ETHERNET_STR =  "GigE";
	public static String OC_48_STR = "OC-48";
	public static String TENGIGABIT_ETHERNET_STR = "10 Gig";
	public static String SYSTEM_FAULT_STR = "systemFault";
	public static String DIALUP_STR = "dialup2";
	public static String RTT_STR = "systemFault"; //round trip time


	//system variables could be declared as strings too
	//half_duplex:, country , etc. F

	/* Method to initialize a few constants
	 * @param Locale: local Locale object
	 * @return none */
	public static void initConstants(Locale paramLocale) {	  
		try {	         
			_rscBundleMessages = ResourceBundle.getBundle( TCPBW100_MSGS, paramLocale);
			System.out.println("Obtained messages ");
		} catch (Exception e) {
			JOptionPane.showMessageDialog(null, "Error while loading language files:\n" + e.getMessage());
			e.printStackTrace();
		}

	} //end method

	/* Overloaded Method to initialize a few constants
	 * @param lang: local Language String
	 * @param country: local country object
	 * @return none */
	public static void initConstants(String lang, String country) {	  
		try {	       
			Locale locale = new Locale(lang, country);
			_rscBundleMessages = ResourceBundle.getBundle("Tcpbw100_msgs", locale);
		} catch (Exception e) {
			JOptionPane.showMessageDialog(null, "Error while loading language files:\n" + e.getMessage());
			e.printStackTrace();
		}
	}//end method initconstants


	/* Getter method for to fetch from resourceBundle
	 * @param: String name of parameter to be fetched
	 * @return: Value of parameter input*/
	public static String getMessageString(String paramStrName) {
		return _rscBundleMessages.getString(paramStrName);
	}

}
