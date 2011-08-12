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

	public Protocol(Socket ctlSocketParam) throws IOException
	{
		_ctlin = ctlSocketParam.getInputStream();
		_ctlout = ctlSocketParam.getOutputStream();
	}

	public void send_msg(byte bParamType, byte bParamToSend) throws IOException
	{
		byte[] tab = new byte[] { bParamToSend };
		send_msg(bParamType, tab);
	}

	public void send_msg(byte bParamType, byte[] baParamTab) throws IOException
	{
		byte[] header = new byte[3];
		header[0] = bParamType;
		header[1] = (byte) (baParamTab.length >> 8);
		header[2] = (byte) baParamTab.length;

		_ctlout.write(header);
		_ctlout.write(baParamTab);
	}

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
		length = ((int) yaMsgBody[1] & 0xFF) << 8;
		length += (int) yaMsgBody[2] & 0xFF; 
		if (readn(msgParam, length) != length) {
			return 3;
		}
		return 0;
	}

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
