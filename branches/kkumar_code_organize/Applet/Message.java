/* Class to define Message.
 * Messages are composed of a "type" and a body
 * Some examples of message types are : COMM_FAILURE, SRV_QUEUE, MSG_LOGIN, TEST_PREPARE etc
 * @see MessageType.java for more examples
 * TODO: NDTP messages are defined to have a "length" field in wiki. Not defined here. Why?
 * Looks like all of the tests (MID, SFW, C2S, s2c, META are using this message type..so, 
 * no specific "mesage type" which does not contain length exists
 * TODO: Is it a good idea to merge MessageTypes into this class?
 * 
 */
public class Message {

	byte type;
	byte[] body;
}