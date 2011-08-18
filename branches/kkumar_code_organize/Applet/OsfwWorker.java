import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;


public class OsfwWorker implements Runnable {
	
	private ServerSocket _srvSocket;
	private int _iTestTime;
	private boolean _iFinalized = false;
	//local TApplet reference
	Tcpbw100 _localTcpAppObj;

	OsfwWorker(ServerSocket srvSocketParam, int iParamTestTime)
	{
		this._srvSocket = srvSocketParam;
		this._iTestTime = iParamTestTime;
	}
	
	OsfwWorker(ServerSocket srvSocketParam, int iParamTestTime, Tcpbw100 _localParam)
	{
		this._srvSocket = srvSocketParam;
		this._iTestTime = iParamTestTime;
		this._localTcpAppObj = _localParam;
	}

	/* Method to make current thread sleep for 1000 ms
	 * @param none
	 * @return none
	 * */
	public void finalize()
	{
		while (!_iFinalized) {
			try {
				Thread.currentThread().sleep(1000);
			}
			catch (InterruptedException e) {
				// do nothing.
			}
		}
	}

	public void run()
	{
		Message msg = new Message();
		Socket sock = null;

		try {
			_srvSocket.setSoTimeout(_iTestTime * 1000);
			try {
				sock = _srvSocket.accept();
			}
			catch (Exception e) {
				e.printStackTrace();
				//s2cResult = NDTConstants.SFW_POSSIBLE;
				this._localTcpAppObj.setS2CTestResults(NDTConstants.SFW_POSSIBLE);
				_srvSocket.close();
				_iFinalized = true;
				return;
			}
			Protocol sfwCtl = new Protocol(sock);
			
			//commented out sections indicate move to outer class	
			if (sfwCtl.recv_msg(msg) != 0) {
				System.out.println("Simple firewall test: unrecognized message");
				//s2cResult = NDTConstants.SFW_UNKNOWN;
				this._localTcpAppObj.setS2CTestResults(NDTConstants.SFW_UNKNOWN);
				sock.close();
				_srvSocket.close();
				_iFinalized = true;
				return;
			}
			if (msg.getType() != MessageType.TEST_MSG) {
				//s2cResult = NDTConstants.SFW_UNKNOWN;
				this._localTcpAppObj.setS2CTestResults(NDTConstants.SFW_UNKNOWN);
				sock.close();
				_srvSocket.close();
				_iFinalized = true;
				return;
			}
			if (! new String(msg.getBody()).equals("Simple firewall test")) {
				System.out.println("Simple firewall test: Improper message");
				//s2cResult = NDTConstants.SFW_UNKNOWN;
				this._localTcpAppObj.setS2CTestResults(NDTConstants.SFW_UNKNOWN);
				sock.close();
				_srvSocket.close();
				_iFinalized = true;
				return;
			}
			//s2cResult = NDTConstants.SFW_NOFIREWALL;
			this._localTcpAppObj.setS2CTestResults(NDTConstants.SFW_NOFIREWALL);
		}
		catch (IOException ex) {
			//s2cResult = NDTConstants.SFW_UNKNOWN;
			this._localTcpAppObj.setS2CTestResults(NDTConstants.SFW_UNKNOWN);
		}
		try {
			sock.close();
			_srvSocket.close();
		}
		catch (IOException e) {
			// do nothing
		}
		_iFinalized = true;
	}
}
