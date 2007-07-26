import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.JPanel;
import javax.swing.BoxLayout;
import javax.swing.JCheckBox;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JComboBox;
import javax.swing.JTextField;
import javax.swing.JPasswordField;
import javax.swing.border.TitledBorder;

import java.util.Collection;
import java.util.Map;
import java.util.HashMap;
import java.util.Vector;

import java.awt.GridLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class DBConfFrame extends JFrame
{
    private JAnalyze mainWindow;
    private static final long serialVersionUID = 1L;
    private JTextField dsn = new JTextField(20);
    private JTextField uid = new JTextField(20);
    private JPasswordField pwd = new JPasswordField(20);

    public DBConfFrame(JAnalyze mainWindow) {
        this.mainWindow = mainWindow;

        Container cp = getContentPane();
        cp.setLayout(new BoxLayout(cp, BoxLayout.Y_AXIS));
        JPanel panel = new JPanel();
        JPanel tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("DSN:"));
        panel.add(dsn);
        tmpPanel.add(panel);
        cp.add(tmpPanel);
        panel = new JPanel();
        tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("UID:"));
        panel.add(uid);
        tmpPanel.add(panel);
        cp.add(tmpPanel);
        panel = new JPanel();
        tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("PWD:"));
        panel.add(pwd);
        tmpPanel.add(panel);
        cp.add(tmpPanel);

        setSize(260, 130);
    }

    public String getDSN() {
        return dsn.getText();
    }

    public String getUID() {
        return uid.getText();
    }

    public String getPWD() {
        return new String(pwd.getPassword());
    }
}
