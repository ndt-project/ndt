import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.JPanel;
import javax.swing.BoxLayout;
import javax.swing.JCheckBox;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JComboBox;
import javax.swing.border.TitledBorder;

import java.util.Collection;
import java.util.Map;
import java.util.HashMap;
import java.util.Vector;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class FilterFrame extends JFrame
{
    private JAnalyze mainWindow;
    private static final long serialVersionUID = 1L;
    private Collection<ResultsContainer> results;

    private final Map<String, Integer> ips = new HashMap<String, Integer>();
    private int mismatchFilter = 2;
    private int cableFaultFilter = 2;
    private int congestionFilter = 2;
    private int duplexFilter = 2;

    public FilterFrame(JAnalyze mainWindow, Collection<ResultsContainer> results) {
        this.mainWindow = mainWindow;
        this.results = results;

        resultsChange();

        setSize(400, 350);
        setVisible(true);
    }

    public Collection<ResultsContainer> getResults() {
        Collection<ResultsContainer> newResults = new Vector<ResultsContainer>();
        for (ResultsContainer result : results) {
            if (mismatchFilter != 2 && result.getMismatch() != mismatchFilter)
                continue;
            if (cableFaultFilter != 2 && result.getCable() != cableFaultFilter)
                continue;
            if (congestionFilter != 2 && result.getCongestion() != congestionFilter)
                continue;
            if (duplexFilter != 2 && result.getDuplex() != duplexFilter)
                continue;
            if (ips.get(result.getIP()).equals(1)) {
                newResults.add(result);
            }
        }
        return newResults;
    }

    public void resultsChange() {
        Container cp = getContentPane();
        cp.removeAll();

        Map<String, Integer> oldIPs = new HashMap<String, Integer>(ips);

        ips.clear();
        for (ResultsContainer result : results) {
            if (!oldIPs.containsKey(result.getIP())) {
                ips.put(result.getIP(), 1);
            }
            else {
                ips.put(result.getIP(), oldIPs.get(result.getIP()));
            }
        }

        cp.setLayout(new BorderLayout());
        JPanel ipsPanel = new JPanel();
        ipsPanel.setBorder(new TitledBorder("Show IPs"));
        ipsPanel.setLayout(new BoxLayout(ipsPanel, BoxLayout.Y_AXIS));
        if (ips.keySet().size() == 0) {
            ipsPanel.add(new JLabel("           "));
        }
        for (String ip : ips.keySet()) {
            JCheckBox checkBox = new JCheckBox(ip, true);
            checkBox.addActionListener( new IPCheckBoxActionListener(checkBox, ip));
            ipsPanel.add(checkBox);
        }
        cp.add(new JScrollPane(ipsPanel), BorderLayout.WEST);

        JPanel applyPanel = new JPanel();
        JButton applyButton = new JButton("Apply");
        applyButton.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                mainWindow.rebuildResultsList();
            }
        });
        applyPanel.add(applyButton);
        cp.add(applyPanel, BorderLayout.SOUTH);

        JPanel optPanel = new JPanel();
        optPanel.setLayout(new BoxLayout(optPanel, BoxLayout.Y_AXIS));
        String[] optStrings = {"no", "yes", "both"};

        JComboBox mismatchBox = new JComboBox(optStrings);
        mismatchBox.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                mismatchFilter = ((JComboBox) e.getSource()).getSelectedIndex();
            }
        });

        mismatchBox.setSelectedIndex(mismatchFilter);
        JPanel horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("Mismatch: "));
        horizontalPanel.add(mismatchBox);
        optPanel.add(horizontalPanel);

        JComboBox cableFaultBox = new JComboBox(optStrings);
        cableFaultBox.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                cableFaultFilter = ((JComboBox) e.getSource()).getSelectedIndex();
            }
        });

        cableFaultBox.setSelectedIndex(cableFaultFilter);
        horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("Cable fault: "));
        horizontalPanel.add(cableFaultBox);
        optPanel.add(horizontalPanel);

        JComboBox congestionBox = new JComboBox(optStrings);
        congestionBox.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                congestionFilter = ((JComboBox) e.getSource()).getSelectedIndex();
            }
        });

        congestionBox.setSelectedIndex(congestionFilter);
        horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("Congestion: "));
        horizontalPanel.add(congestionBox);
        optPanel.add(horizontalPanel);

        JComboBox duplexBox = new JComboBox(optStrings);
        duplexBox.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                duplexFilter = ((JComboBox) e.getSource()).getSelectedIndex();
            }
        });

        duplexBox.setSelectedIndex(duplexFilter);
        horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("Duplex: "));
        horizontalPanel.add(duplexBox);
        optPanel.add(horizontalPanel);

        JPanel tmpPanel = new JPanel();
        tmpPanel.add(new JScrollPane(optPanel));
        cp.add(tmpPanel);

        validate();
        cp.repaint();
    }

    class IPCheckBoxActionListener implements ActionListener {
        private JCheckBox checkBox;
        private String ip;

        IPCheckBoxActionListener(JCheckBox checkBox, String ip) {
            this.checkBox = checkBox;
            this.ip = ip;
        }

        public void actionPerformed(ActionEvent e) {
            if (checkBox.isSelected()) {
                ips.put(ip, 1);
            }
            else {
                ips.put(ip, 0);
            }
        }
    }
}
