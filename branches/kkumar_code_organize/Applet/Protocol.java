import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

/* Class to define Protocol
 * TODO :summarize methods below to give a brief explanation of the Protocol class 
 * TODO :use setter/getter methods for 
 * */


public class Protocol {
	private InputStream _ctlin;
	private OutputStream _ctlout;
	
    /* Class Constructor 
     * @param Socket: socket used to send the protocol messages over 
     */
	public Protocol(Socket ctlSocketParam) throws IOException
	{
		_ctlin = ctlSocketParam.getInputStream();
		_ctlout = ctlSocketParam.getOutputStream();
	}

	/* Method to send msg. Overloaded
	 * @param bParamType: Ctrl Message Type
	 * @param bParamToSend:  Data value to send
	 * @return none
	 * 
	 * */
	public void send_msg(byte bParamType, byte bParamToSend) throws IOException
	{
		byte[] tab = new byte[] { bParamToSend };
		send_msg(bParamType, tab);
	}

	
	/* Method to send msg. Overloaded
	 * @param bParamType:  Ctrl Message Type
	 * @param bParamToSend[]:  Data value to send
	 * @return none
	 * 
	 * */
	public void send_msg(byte bParamType, byte[] baParamTab) throws IOException
	{
		byte[] header = new byte[3];
		header[0] = bParamType;
		
		//2 bytes are used to hold data length. Thus, max(data length) = 65535
		header[1] = (byte) (baParamTab.length >> 8);	
		header[2] = (byte) baParamTab.length;

		//Write data to outputStream
		_ctlout.write(header);
		_ctlout.write(baParamTab);
	}

	/* Populate Message byte array with specific number of bytes of data
	 * from socket input stream
	 * @param msgParam: Message object to be populated
	 * @param iParamAmount:  specified number of bytes to be read
	 * @return int: Actual number of bytes populated
	 */
	public int readn(Message msgParam, int iParamAmount) throws IOException
	{
		int read = 0; 
		int tmp;
		//msg.body = new byte[amount];
		msgParam.initBodySize(iParamAmount);
		while (read != iParamAmount) {
			//tmp = _ctlin.read(msg.body, read, amount - read);
			tmp = _ctlin.read(msgParam._yaBody, read, iParamAmount - read);
			if (tmp <= 0) {
				return read;
			}
			read += tmp;
		}
		return read;
	}

	/* Receive message at end point of socket
	 * @param Message object
	 * @return : integer with values:
	 *  a) Success: 
	 * 	 value=0 : successfully read expected number of bytes
	 *  b) Error:
	 *   value= 1 : Error reading ctrl-message length and data type itself, since 
	 * 		NDTP-control packet has to be at 
	 * 		the least 3 octets long
	 *   value= 3 : Error, mismatch between "length" field of ctrl-message and 
	 * 		actual data read
	 *  
	 * */
	public int recv_msg(Message msgParam) throws IOException
	{
		int length;
		if (readn(msgParam, 3) != 3) {
			return 1;
		}
		/*
		 * msg.type = msg.body[0];
		length = ((int) msg.body[1] & 0xFF) << 8;
		length += (int) msg.body[2] & 0xFF;
		*/
		byte [] yaMsgBody = msgParam.getBody();
		msgParam.setType(yaMsgBody[0]);
		
		//Get data length
		length = ((int) yaMsgBody[1] & 0xFF) << 8;
		length += (int) yaMsgBody[2] & 0xFF; 

		if (readn(msgParam, length) != length) {
			return 3;
		}
		return 0;
	}

	/* Method to clsoe open Streams
	 * @param none
	 * @return none*/
	public void close()
	{
		try {
			_ctlin.close();
			_ctlout.close();
		}
		catch (IOException e) {
			e.printStackTrace();
		}
	}


}
