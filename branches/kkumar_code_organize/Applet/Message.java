/* Class to define Message.
 * Messages are composed of a "type" and a body
 * Some examples of message types are : COMM_FAILURE, SRV_QUEUE, MSG_LOGIN, TEST_PREPARE etc
 * @see MessageType.java for more examples
 * Messages are defined to have a "length" field too. Currently, 2 bytes of the the message 
 * "body" byte array are often used to store length (second/third array positions , for example)
 *
 * TODO: It may be worthwhile exploring whether MessageTypes could be merged into here instead of NDTConstants.
 * . For a later release 
 * 
 */
public class Message {

	//TODO: Could make these private and test changes in Protocol class. For later release
	byte _yType;
	byte[] _yaBody;

	/* Getter method to get Message Type
	 *  @param: none
	 *  @return byte indicating Message Type
	 *  */	 
	public byte getType() {
		return _yType;
	}

	/* Setter method to set Message Type
	 *  @return: none
	 *  @param byte indicating Message Type
	 *  */	
	public void setType(byte bParamType) {
		this._yType = bParamType;
	}

	/* Getter method to get Message body
	 *  @return: byte array message body
	 *  @param none
	 *  */	
	public byte[] getBody() {
		return _yaBody;
	}

	/* Setter method to get Message body
	 *  @param iParamSize : byte array size
	 *  @param byte array message body
	 *  @return: none
	 *  */	
	public void setBody(byte[] baParamBody, int iParamSize) {
		_yaBody = new byte[iParamSize];
		System.arraycopy(baParamBody, 0, _yaBody, 0, iParamSize);
	}


	/* Utility method to initialize Message body
	 *  @param iParamSize : byte array size
	 *  @return: none
	 *  */	
	public void initBodySize(int iParamSize) {
		this._yaBody = new byte[iParamSize];	
	}


}