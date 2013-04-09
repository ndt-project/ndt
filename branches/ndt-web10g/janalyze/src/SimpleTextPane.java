import javax.swing.JTextPane;
import javax.swing.text.BadLocationException;
import javax.swing.text.AttributeSet;

import java.awt.Component;

public class SimpleTextPane extends JTextPane
  {
	private static final long serialVersionUID = 1L;

	public void append(String text)
    {
      try {
        getStyledDocument().insertString(getStyledDocument().getLength(), text, null);
      }
      catch (BadLocationException e) {
        System.out.println("WARNING: failed to append text to the text pane! [" + text + "]");
      }
    }

    public void appendColored(String text, AttributeSet attributes)
    {
      try {
        getStyledDocument().insertString(getStyledDocument().getLength(), text, attributes);
      }
      catch (BadLocationException e) {
        System.out.println("WARNING: failed to append text to the text pane! [" + text + "]");
      }
    }

    public void insertComponent(Component c)
    {
      setSelectionStart(getStyledDocument().getLength());
      setSelectionEnd(getStyledDocument().getLength());
      super.insertComponent(c);
    }
  }
