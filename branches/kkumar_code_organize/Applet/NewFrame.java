import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import javax.swing.JFrame;


/*
 * Class that defines a new Frame with a window closing functionality
 * 
 */
class NewFrame extends JFrame {
	/**
	 * 
	 */
	private static final long serialVersionUID = 8990839319520684317L;
	
	/*Constructor 
	 * @param none*/

	public NewFrame() {
		addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent event) {
				// System.err.println("Handling window closing event");
				dispose();
			}
		});
		// System.err.println("Extended Frame class - RAC9/15/03");
	}
	
} // class: NewFrame