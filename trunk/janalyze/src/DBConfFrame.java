import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JPasswordField;
import java.awt.Container;

public class DBConfFrame extends JFrame
{    
    private static final long serialVersionUID = 1L;
    private JTextField driver = new JTextField(20);
    private JTextField dsn = new JTextField(20);
    private JTextField uid = new JTextField(20);
    private JPasswordField pwd = new JPasswordField(20);

    public DBConfFrame(JAnalyze mainWindow) {

        Container cp = getContentPane();
        cp.setLayout(new BoxLayout(cp, BoxLayout.Y_AXIS));
        JPanel panel = new JPanel();
        JPanel tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("Driver:"));
        driver.setText(mainWindow.getProperties().getProperty("driver", "com.mysql.jdbc.Driver"));
        driver.getDocument().addDocumentListener(new PropertyListener(mainWindow, "driver"));
        panel.add(driver);
        tmpPanel.add(panel);
        cp.add(tmpPanel);
        panel = new JPanel();
        tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("DSN:"));
        dsn.setText(mainWindow.getProperties().getProperty("dsn", "jdbc:mysql://localhost/test"));
        dsn.getDocument().addDocumentListener(new PropertyListener(mainWindow, "dsn"));
        panel.add(dsn);
        tmpPanel.add(panel);
        cp.add(tmpPanel);
        panel = new JPanel();
        tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("UID:"));
        uid.setText(mainWindow.getProperties().getProperty("uid", ""));
        uid.getDocument().addDocumentListener(new PropertyListener(mainWindow, "uid"));
        panel.add(uid);
        tmpPanel.add(panel);
        cp.add(tmpPanel);
        panel = new JPanel();
        tmpPanel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.X_AXIS));
        panel.add(new JLabel("PWD:"));
        pwd.setText(mainWindow.getProperties().getProperty("pwd", ""));
        pwd.getDocument().addDocumentListener(new PropertyListener(mainWindow, "pwd"));
        panel.add(pwd);
        tmpPanel.add(panel);
        cp.add(tmpPanel);

        setSize(270, 150);
    }

    public String getDriver() {
        return driver.getText();
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
