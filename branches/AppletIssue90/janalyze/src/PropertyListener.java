import javax.swing.event.DocumentListener;
import javax.swing.event.DocumentEvent;
import javax.swing.text.BadLocationException;

public class PropertyListener implements DocumentListener {
    private JAnalyze mainWindow;
    private String property;

    public PropertyListener(JAnalyze mainWindow, String property) {
        this.mainWindow = mainWindow;
        this.property = property;
    }

    private void updateProperty(DocumentEvent e) {
        try {
            mainWindow.getProperties().setProperty(property,
                    e.getDocument().getText(0, e.getDocument().getLength()));
        }
        catch (BadLocationException exc) {
            // do nothing
        }
    }

    public void changedUpdate(DocumentEvent e) {
        updateProperty(e);
    }
    public void insertUpdate(DocumentEvent e) {
        updateProperty(e);
    }
    public void removeUpdate(DocumentEvent e) {
        updateProperty(e);
    }
}
