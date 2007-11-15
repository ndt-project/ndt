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
import java.util.HashSet;
import java.util.StringTokenizer;

import java.awt.Dimension;
import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class FilterFrame extends JFrame
{
    private JAnalyze mainWindow;
    private static final long serialVersionUID = 1L;
    private Collection<ResultsContainer> results;
    private Collection<String> disabled = new HashSet<String>();

    private final Map<String, Integer> ips = new HashMap<String, Integer>();
    private int mismatchFilter;
    private int cableFaultFilter = 2;
    private int congestionFilter = 2;
    private int duplexFilter = 2;
    private int newCongestionFilter = 2;
    private int initialPeakSpeedFilter = 4;

    public FilterFrame(JAnalyze mainWindow, Collection<ResultsContainer> results) {
        this.mainWindow = mainWindow;
        this.results = results;

        resultsChange();

        StringTokenizer st = new StringTokenizer(mainWindow.getProperties().getProperty("disabled", ""), ",");
        while (st.hasMoreTokens()) {
            disabled.add(st.nextToken());
        }

        setSize(400, 350);
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
            if (newCongestionFilter != 2 && result.getNewCongestion() != newCongestionFilter)
                continue;
            if (initialPeakSpeedFilter != 4 && result.getInitialPeakSpeedEquality() != initialPeakSpeedFilter)
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
                ips.put(result.getIP(), disabled.contains(result.getIP()) ? 0 : 1);
            }
            else {
                ips.put(result.getIP(), oldIPs.get(result.getIP()));
            }
        }

        cp.setLayout(new BorderLayout());
        JPanel leftPanel = new JPanel();
        leftPanel.setLayout(new BorderLayout());
        final JPanel ipsPanel = new JPanel();
        ipsPanel.setBorder(new TitledBorder("Show IPs"));
        ipsPanel.setLayout(new BoxLayout(ipsPanel, BoxLayout.Y_AXIS));
        if (ips.keySet().size() == 0) {
            ipsPanel.add(new JLabel("           "));
        }
        else {
            JPanel tmpPanel = new JPanel();
            tmpPanel.setBorder(new TitledBorder("select IPs"));
            JButton allButton = new JButton("all");
            allButton.addActionListener( new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    for (Component component : ipsPanel.getComponents()) {
                        JCheckBox checkBox = (JCheckBox) component;
                        checkBox.setSelected(true);
                    }
                }
            });
            allButton.setPreferredSize(new Dimension(55, 20));
            JButton noneButton = new JButton("none");
            noneButton.addActionListener( new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    for (Component component : ipsPanel.getComponents()) {
                        JCheckBox checkBox = (JCheckBox) component;
                        checkBox.setSelected(false);
                    }
                }
            });
            noneButton.setPreferredSize(new Dimension(70, 20));
            tmpPanel.add(allButton);
            tmpPanel.add(noneButton);
            leftPanel.add(tmpPanel, BorderLayout.SOUTH);
        }
        for (String ip : ips.keySet()) {
            JCheckBox checkBox = new JCheckBox(ip, ips.get(ip) == 1) {
				private static final long serialVersionUID = 1L;

				public void setSelected(boolean value) {
                    super.setSelected(value);
                    fireActionPerformed(new ActionEvent(FilterFrame.this,
                                ActionEvent.ACTION_PERFORMED,
                                ""));
                }
            };
            checkBox.addActionListener( new IPCheckBoxActionListener(checkBox, ip, disabled));
            ipsPanel.add(checkBox);
        }
        leftPanel.add(new JScrollPane(ipsPanel));
        cp.add(leftPanel, BorderLayout.WEST);

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
                mainWindow.getProperties().setProperty("mismatchFilter", Integer.toString(mismatchFilter));
            }
        });

        mismatchFilter = Integer.parseInt(mainWindow.getProperties().getProperty("mismatchFilter", "2"));
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
                mainWindow.getProperties().setProperty("cableFaultFilter", Integer.toString(cableFaultFilter));
            }
        });

        cableFaultFilter = Integer.parseInt(mainWindow.getProperties().getProperty("cableFaultFilter", "2"));
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
                mainWindow.getProperties().setProperty("congestionFilter", Integer.toString(congestionFilter));
            }
        });

        congestionFilter = Integer.parseInt(mainWindow.getProperties().getProperty("congestionFilter", "2"));
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
                mainWindow.getProperties().setProperty("duplexFilter", Integer.toString(duplexFilter));
            }
        });

        duplexFilter = Integer.parseInt(mainWindow.getProperties().getProperty("duplexFilter", "2"));
        duplexBox.setSelectedIndex(duplexFilter);
        horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("Duplex: "));
        horizontalPanel.add(duplexBox);
        optPanel.add(horizontalPanel);

        JComboBox newCongestionBox = new JComboBox(optStrings);
        newCongestionBox.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                newCongestionFilter = ((JComboBox) e.getSource()).getSelectedIndex();
                mainWindow.getProperties().setProperty("newCongestionFilter",
                    Integer.toString(newCongestionFilter));
            }
        });

        newCongestionFilter=Integer.parseInt(mainWindow.getProperties().getProperty("newCongestionFilter","2"));
        newCongestionBox.setSelectedIndex(newCongestionFilter);
        horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("New congestion: "));
        horizontalPanel.add(newCongestionBox);
        optPanel.add(horizontalPanel);

        JComboBox initialPeakSpeedBox = new JComboBox(new String[] {"n/a", "equal", "greater", "less", "all"});
        initialPeakSpeedBox.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                initialPeakSpeedFilter = ((JComboBox) e.getSource()).getSelectedIndex();
                mainWindow.getProperties().setProperty("initialPeakSpeedFilter",
                    Integer.toString(initialPeakSpeedFilter));
            }
        });

        initialPeakSpeedFilter=Integer.parseInt(mainWindow.getProperties().getProperty("initialPeakSpeedFilter","4"));
        initialPeakSpeedBox.setSelectedIndex(initialPeakSpeedFilter);
        horizontalPanel = new JPanel();
        horizontalPanel.setLayout(new BoxLayout(horizontalPanel, BoxLayout.X_AXIS));
        horizontalPanel.add(new JLabel("Initial peak speed: "));
        horizontalPanel.add(initialPeakSpeedBox);
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
        private Collection<String> disabled;

        IPCheckBoxActionListener(JCheckBox checkBox, String ip, Collection<String> disabled) {
            this.checkBox = checkBox;
            this.ip = ip;
            this.disabled = disabled;
        }

        public void actionPerformed(ActionEvent e) {
            if (checkBox.isSelected()) {
                ips.put(ip, 1);
                disabled.remove(ip);
            }
            else {
                ips.put(ip, 0);
                if (!disabled.contains(ip)) {
                    disabled.add(ip);
                }
            }
            StringBuffer newDisabled = new StringBuffer();
            boolean first = true;
            for (String ip : disabled) {
                if (!first) {
                    newDisabled.append(",");
                }
                newDisabled.append(ip);
                first = false;
            }
            mainWindow.getProperties().setProperty("disabled", newDisabled.toString());
        }
    }
}
