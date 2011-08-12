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

	//TODO: Make this private and test changes in Protocol class
	byte _yType;
	byte[] _yaBody;
	
	public byte getType() {
		return _yType;
	}
	
	public void setType(byte bParamType) {
		this._yType = bParamType;
	}
	
	public byte[] getBody() {
		return _yaBody;
	}
	
	public void setBody(byte[] baParamBody, int iParamSize) {
		_yaBody = new byte[iParamSize];
		System.arraycopy(baParamBody, 0, _yaBody, 0, iParamSize);
	}
	
	public void initBodySize(int iParamSize) {
		this._yaBody = new byte[iParamSize];
		
	}
	
	
}