import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

/* Class to define Protocol
 * TODO :summarize methods below to give a brief explanation of the Protocol class 
 * */


public class Protocol {
	private InputStream _ctlin;
	private OutputStream _ctlout;

	public Protocol(Socket ctlSocket) throws IOException
	{
		_ctlin = ctlSocket.getInputStream();
		_ctlout = ctlSocket.getOutputStream();
	}

	public void send_msg(byte type, byte toSend) throws IOException
	{
		byte[] tab = new byte[] { toSend };
		send_msg(type, tab);
	}

	public void send_msg(byte type, byte[] tab) throws IOException
	{
		byte[] header = new byte[3];
		header[0] = type;
		header[1] = (byte) (tab.length >> 8);
		header[2] = (byte) tab.length;

		_ctlout.write(header);
		_ctlout.write(tab);
	}

	public int readn(Message msg, int amount) throws IOException
	{
		int read = 0; 
		int tmp;
		msg.body = new byte[amount];
		while (read != amount) {
			tmp = _ctlin.read(msg.body, read, amount - read);
			if (tmp <= 0) {
				return read;
			}
			read += tmp;
		}
		return read;
	}

	public int recv_msg(Message msg) throws IOException
	{
		int length;
		if (readn(msg, 3) != 3) {
			return 1;
		}
		msg.type = msg.body[0];
		length = ((int) msg.body[1] & 0xFF) << 8;
		length += (int) msg.body[2] & 0xFF; 
		if (readn(msg, length) != length) {
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
