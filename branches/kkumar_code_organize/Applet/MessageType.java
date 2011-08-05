/* Class to define the NDTP control message types 
 * 
 * Moved out of Tcpbw100.java
 * Changed access specifier of constants to public from private. These are static message
 *  types and not data members, and having public access privilege will not affect encapsulation
 *  TODO: Revisit documentation. Currently used also to hold record change history
 * 
 * */

public class MessageType {
	
	  public static final byte COMM_FAILURE  = 0;
	  public static final byte SRV_QUEUE     = 1;
	  public static final byte MSG_LOGIN     = 2;
	  public static final byte TEST_PREPARE  = 3;
	  public static final byte TEST_START    = 4;
	  public static final byte TEST_MSG      = 5;
	  public static final byte TEST_FINALIZE = 6;
	  public static final byte MSG_ERROR     = 7;
	  public static final byte MSG_RESULTS   = 8;
	  public static final byte MSG_LOGOUT    = 9;
	  public static final byte MSG_WAITING   = 10;

}
