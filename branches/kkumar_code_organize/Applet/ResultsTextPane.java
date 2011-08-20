import java.awt.Component;

import javax.swing.JTextPane;
import javax.swing.text.BadLocationException;

/* Class used to extend textPane to  be used to display results
 * of tests
 */
public class ResultsTextPane extends JTextPane
{
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	/*Method to append String into the current document
	 * @param String : Text to be inserted into the document
	 * @return none*/
	public void append(String text)
	{
		try {
			getStyledDocument().insertString(getStyledDocument().getLength(), text, null);
		}
		catch (BadLocationException e) {
			System.out.println("WARNING: failed to append text to the text pane! [" + text + "]");
		}
	}

	/* JTextPane method to insert a component into the document as a replacement for currently selected content; 
	 * if no selection the component is inserted at the current position of the caret.
	 * @param Component : the component to insert
	 * @return none
	 */
	public void insertComponent(Component c)
	{
		/*
		  setSelectionStart(results.getStyledDocument().getLength());
		  setSelectionEnd(results.getStyledDocument().getLength());
		 */
		//change "results" to "this". re-arch
		setSelectionStart(this.getStyledDocument().getLength());
		setSelectionEnd(this.getStyledDocument().getLength()); 
		super.insertComponent(c);
	}
	
}

