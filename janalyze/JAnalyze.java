import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.ListSelectionModel;
import javax.swing.UIManager;
import javax.swing.JFileChooser;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.JTextArea;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import java.io.File;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Collection;
import java.util.Vector;
import java.util.StringTokenizer;
import java.util.Properties;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.Statement;
import java.sql.ResultSet;
import java.sql.SQLException;

import de.progra.charting.DefaultChart;
import de.progra.charting.model.EditableChartDataModel;
import de.progra.charting.render.LineChartRenderer;
import de.progra.charting.swing.ChartPanel;

public class JAnalyze extends JFrame
{
	private static final long serialVersionUID = 1L;
	
	JMenuBar menuBar = new JMenuBar();
    JMenu fileMenu = new JMenu("File");
    JMenu optionsMenu = new JMenu("Options");
    JMenu actionsMenu = new JMenu("Actions");
    JMenuItem exitMenuItem = new JMenuItem("Exit");
    JMenuItem loadMenuItem = new JMenuItem("Load");
    JMenuItem loadDBMenuItem = new JMenuItem("Load from DB");
    JMenuItem filterMenuItem = new JMenuItem("Filter");
    JMenuItem snaplogMenuItem = new JMenuItem("Snaplogs");
    JMenuItem dbConfMenuItem = new JMenuItem("DB conf");
    JMenu snaplogsMenu = new JMenu("Snaplogs");
    JMenuItem viewSnaplogMenuItem = new JMenuItem("View");
    JMenuItem plotSnaplogMenuItem = new JMenuItem("Plot");
    JMenuItem plotSnaplogCWNDMenuItem = new JMenuItem("Plot CWND");
    JMenu tcpdumpsMenu = new JMenu("Tcpdumps");
    JMenuItem viewTcpdumpMenuItem = new JMenuItem("View");
    JMenuItem plotTcpdumpMenuItem = new JMenuItem("Plot");
    JMenu cputimesMenu = new JMenu("Cputimes");
    JMenuItem viewCputimeMenuItem = new JMenuItem("View");
    JFileChooser fc;
    final Properties properties = new Properties();
    final File propertyFile = new File(System.getenv("HOME") + "/.jAnalyze.properties");

    FilterFrame filterFrame = null;
    DBConfFrame dbConfFrame = null;
    SnaplogFrame snaplogFrame = null;
    
    JPanel listPanel = new JPanel();
    JPanel infoPanel = new JPanel();
    
    private boolean stopLoading;
    
    private Collection<ResultsContainer> results = new Vector<ResultsContainer>();

    public JAnalyze()
    {
        // Title
        setTitle("JAnalyze v0.9.1");

        try {
            properties.load(new FileInputStream(propertyFile));
        }
        catch (FileNotFoundException e) {
            // do nothing
        }
        catch (IOException e) {
            // do nothing
        }

        dbConfFrame = new DBConfFrame(this);
        snaplogFrame = new SnaplogFrame(this);
        filterFrame = new FilterFrame(JAnalyze.this, results);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                try {
                    properties.store(new FileOutputStream(propertyFile), "JAnalyze properties file");
                }
                catch (FileNotFoundException e) {
                    // do nothing
                }
                catch (IOException e) {
                    // do nothing
                }
            }
        });

        // Menu        
        loadMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if (fc == null) {
                    fc = new JFileChooser(new File(getProperties().getProperty("loadDir", "/usr/local/ndt/")));
                }
                int returnVal = fc.showOpenDialog(JAnalyze.this);
                getProperties().setProperty("loadDir", fc.getCurrentDirectory().getAbsolutePath());

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    try {
                        File file = fc.getSelectedFile();
                        JFrame loadingFrame = new LoadingFrame(file.getAbsolutePath(), JAnalyze.this); 
                        loadingFrame.setSize(150, 100);
                        loadingFrame.setVisible(true);
                    }
                    catch (Exception exc) {
                        System.out.println("Loading of the web100srv.log failed!");
                        exc.printStackTrace();
                    }
                } 
            }
        });
        fileMenu.add(loadMenuItem);
        loadDBMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFrame loadingFrame = new LoadingDBFrame(JAnalyze.this); 
                loadingFrame.setSize(150, 100);
                loadingFrame.setVisible(true);
            } 
        });
        fileMenu.add(loadDBMenuItem);
        fileMenu.addSeparator();
        exitMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent event) {
                try {
                    properties.store(new FileOutputStream(propertyFile), "JAnalyze properties file");
                }
                catch (FileNotFoundException e) {
                    // do nothing
                }
                catch (IOException e) {
                    // do nothing
                }
                System.exit(0);
            }
        });
        fileMenu.add(exitMenuItem);
        menuBar.add(fileMenu);

        filterMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                filterFrame.setVisible(true);
            }
        });
        optionsMenu.add(filterMenuItem);
        dbConfMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                dbConfFrame.setVisible(true);
            }
        });
        optionsMenu.add(dbConfMenuItem);
        snaplogMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                snaplogFrame.setVisible(true);
            }
        });
        optionsMenu.add(snaplogMenuItem);

        menuBar.add(optionsMenu);

        viewSnaplogMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
                int returnVal = chooser.showOpenDialog(JAnalyze.this);

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = chooser.getSelectedFile();
                    String snaplogData = getSnaplogData(file.getAbsolutePath(), null, 0, false);
                    JTextArea area = new JTextArea(snaplogData);
                    area.setEditable(false);
                    JFrame frame = new JFrame("Snaplog variables");
                    frame.getContentPane().add(new JScrollPane(area), BorderLayout.CENTER);
                    frame.setSize(800, 600);
                    frame.setVisible(true);
                } 
            }
        });
        snaplogsMenu.add(viewSnaplogMenuItem);
        plotSnaplogMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
                int returnVal = chooser.showOpenDialog(JAnalyze.this);

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = chooser.getSelectedFile();
                    plotSnaplog(file.getAbsolutePath());
                } 
            }
        });
        snaplogsMenu.add(plotSnaplogMenuItem);
        plotSnaplogCWNDMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
                int returnVal = chooser.showOpenDialog(JAnalyze.this);

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = chooser.getSelectedFile();
                    plotSnaplogCWND(file.getAbsolutePath());
                } 
            }
        });
        snaplogsMenu.add(plotSnaplogCWNDMenuItem);
        actionsMenu.add(snaplogsMenu);
        viewTcpdumpMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
                int returnVal = chooser.showOpenDialog(JAnalyze.this);

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = chooser.getSelectedFile();
                    String tcpdumpData = getTcpdumpData(file.getAbsolutePath());
                    JTextArea area = new JTextArea(tcpdumpData);
                    area.setEditable(false);
                    JFrame frame = new JFrame("tcpdump trace");
                    frame.getContentPane().add(new JScrollPane(area), BorderLayout.CENTER);
                    frame.setSize(800, 600);
                    frame.setVisible(true);
                } 
            }
        });
        tcpdumpsMenu.add(viewTcpdumpMenuItem);
        plotTcpdumpMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
                int returnVal = chooser.showOpenDialog(JAnalyze.this);

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = chooser.getSelectedFile();
                    plotTcpdumpS(file.getAbsolutePath());
                } 
            }
        });
        tcpdumpsMenu.add(plotTcpdumpMenuItem);
        actionsMenu.add(tcpdumpsMenu);
        viewCputimeMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
                int returnVal = chooser.showOpenDialog(JAnalyze.this);

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = chooser.getSelectedFile();
                    plotCputime(file.getAbsolutePath());
                } 
            }
        });
        cputimesMenu.add(viewCputimeMenuItem);
        actionsMenu.add(cputimesMenu);

        menuBar.add(actionsMenu);

        setJMenuBar(menuBar);
        
        Container cp = getContentPane();
        cp.setLayout(new BorderLayout());
        listPanel.setLayout(new BorderLayout());
        cp.add(listPanel, BorderLayout.WEST);
        infoPanel.setLayout(new BorderLayout());
        cp.add(infoPanel);

        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setSize(900, 770);
        setVisible(true);
    }
    
    protected void loadDB(LoadingDBFrame frame) throws SQLException
    {
        stopLoading = false;
        try {
            Class.forName (dbConfFrame.getDriver()).newInstance ();
        }
        catch (Exception e) {
            System.out.println("Failed to load mysql jdbc driver: " + e.getMessage());
        }
        Connection con = DriverManager.getConnection(dbConfFrame.getDSN(),
                dbConfFrame.getUID(), dbConfFrame.getPWD());

        Statement stmt = con.createStatement();
        ResultSet rs = stmt.executeQuery("SELECT * FROM ndt_test_results;");
        Collection<ResultsContainer> newResults = new Vector<ResultsContainer>();
        while (!stopLoading && rs.next()) {
            ResultsContainer result = new ResultsContainer(this);
            try {
              result.parseDBRow(rs);
            }
            catch (SQLException e) {
                continue;
            }
            result.calculate();
            newResults.add(result);
            frame.loadedResult();
        }
        if (stopLoading) {
        	return;
        }
        results.clear();
        results.addAll(newResults);
        filterFrame.resultsChange();
        rebuildResultsList();
    }

    protected void loadWeb100srvLog(String filename, LoadingFrame frame) throws FileNotFoundException, IOException
    {
    	stopLoading = false;
//        System.out.println("Loading file: " + filename);
        BufferedReader br = new BufferedReader(new FileReader(filename));
        String line;
        ResultsContainer result = new ResultsContainer(this);
        Collection<ResultsContainer> newResults = new Vector<ResultsContainer>();
        while (!stopLoading && (line = br.readLine()) != null) {
            if (line.startsWith("spds")) {
            	result.parseSpds(line);
            	continue;
            }
            if (line.startsWith("Running")) {
            	result.parseRunAvg(line);
            	continue;
            }
            if (line.startsWith("snaplog file:")) {
            	result.parseSnaplogFilename(line);
            	continue;
            }
            if (line.startsWith("c2s_snaplog file:")) {
            	result.parseC2sSnaplogFilename(line);
            	continue;
            }
            if (line.startsWith("cputime trace file:")) {
            	result.parseCputimeFilename(line);
            	continue;
            }
            if (line.contains("port")) {
            	result = new ResultsContainer(this);
            	result.parsePort(line);
            	continue;
            }
            if (line.contains(",")) {
            	if ((result != null) && result.parseWeb100Var(line)) {
            		result.calculate();
            		newResults.add(result);
            		result = null;
            		frame.loadedResult();
            	}
            }
        }
        if (stopLoading) {
        	return;
        }
        results.clear();
        results.addAll(newResults);
        filterFrame.resultsChange();
        rebuildResultsList();
    }

    protected String getSnaplogData(String snaplogFilename, String variables, int numOfLines, boolean trim) {
        if (snaplogFilename == null) {
            return "";
        }
        String[] cmdarray = new String[] {snaplogFrame.getGenplot(), "-tm", variables == null ?
            snaplogFrame.getVariables() : variables,
            snaplogFilename.startsWith("/") ? snaplogFilename :
                snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + snaplogFilename :
                snaplogFrame.getSnaplogs() + "/" +  snaplogFilename};
        Process genplotProcess;
        try {
            genplotProcess = Runtime.getRuntime().exec(cmdarray);
        }
        catch (IOException e) {
            System.out.println(e);
            return null;
        }
        BufferedReader in = new BufferedReader(new InputStreamReader(genplotProcess.getInputStream()));
        StringBuffer toReturn = new StringBuffer();
        try {
            int counter = 1;
            if (trim) {
                in.readLine();in.readLine();in.readLine();
            }
            String line = in.readLine();
            while (line != null && counter != numOfLines) {
                toReturn.append(line + "\n");
                line = in.readLine();
                counter += 1;
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        genplotProcess.destroy();
        return toReturn.toString();
    }

    protected boolean checkTcpDump(String ndttraceFilename) {
        File file = new File(snaplogFrame.getSnaplogs(), ndttraceFilename);
        return file.exists() && file.canRead();
    }

    protected String getTcpdumpData(String ndttraceFilename) {
        if (ndttraceFilename == null) {
            return "";
        }
        String[] cmdarray = new String[] {snaplogFrame.getTcptrace(), "-l",
            ndttraceFilename.startsWith("/") ? ndttraceFilename :
                snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + ndttraceFilename :
                snaplogFrame.getSnaplogs() + "/" +  ndttraceFilename};
        Process tcptraceProcess;
        try {
            tcptraceProcess = Runtime.getRuntime().exec(cmdarray);
        }
        catch (IOException e) {
            System.out.println(e);
            return null;
        }
        BufferedReader in = new BufferedReader(new InputStreamReader(tcptraceProcess.getInputStream()));
        StringBuffer toReturn = new StringBuffer();
        try {
            String line = in.readLine();
            while (line != null) {
                toReturn.append(line + "\n");
                line = in.readLine();
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        tcptraceProcess.destroy();
        return toReturn.toString();

    }

    protected void plotTcpdumpS(String ndttraceFilename) {
        if (ndttraceFilename == null) {
            return;
        }
        String[] cmdarray = new String[] {snaplogFrame.getTcptrace(), "-S",
            ndttraceFilename.startsWith("/") ? ndttraceFilename :
                snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + ndttraceFilename :
                snaplogFrame.getSnaplogs() + "/" +  ndttraceFilename};
        String[] cmdarray2 = new String[] {snaplogFrame.getXplot(), "a2b_tsg.xpl"};
        String[] cmdarray3 = new String[] {snaplogFrame.getXplot(), "b2a_tsg.xpl"};
        try {
            Runtime.getRuntime().exec(cmdarray);
            Thread.sleep(1000);
            Runtime.getRuntime().exec(cmdarray2);
            Runtime.getRuntime().exec(cmdarray3);
        }
        catch (InterruptedException e) {
            System.out.println(e);
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }


    protected void plotSnaplog(String snaplogFilename) {
        if (snaplogFilename == null) {
            return;
        }
        String[] cmdarray = new String[] {snaplogFrame.getGenplot(), "-m", snaplogFrame.getVariables(),
            snaplogFilename.startsWith("/") ? snaplogFilename :
                snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + snaplogFilename :
                snaplogFrame.getSnaplogs() + "/" +  snaplogFilename};
        String[] cmdarray2 = new String[] {snaplogFrame.getXplot(),
        "User Defined" +
            (snaplogFilename.substring(snaplogFilename.lastIndexOf(".")).length() > 6 ?
             snaplogFilename.substring(
                 snaplogFilename.substring(0, snaplogFilename.lastIndexOf(".")).lastIndexOf("."),
                     snaplogFilename.lastIndexOf(".")) :
             snaplogFilename.substring(snaplogFilename.lastIndexOf("."))) + ".xpl"};
        try {
            Runtime.getRuntime().exec(cmdarray);
            Thread.sleep(1000);
            Runtime.getRuntime().exec(cmdarray2);
        }
        catch (InterruptedException e) {
            System.out.println(e);
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }

    protected void plotSnaplogCWND(String snaplogFilename) {
        if (snaplogFilename == null) {
            return;
        }
        String[] cmdarray = new String[] {snaplogFrame.getGenplot(), "-C",
            snaplogFilename.startsWith("/") ? snaplogFilename :
                snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + snaplogFilename :
                snaplogFrame.getSnaplogs() + "/" +  snaplogFilename};
        String[] cmdarray2 = new String[] {snaplogFrame.getXplot(),
        "CurCwnd" +
            (snaplogFilename.substring(snaplogFilename.lastIndexOf(".")).length() > 6 ?
             snaplogFilename.substring(
                 snaplogFilename.substring(0, snaplogFilename.lastIndexOf(".")).lastIndexOf("."),
                     snaplogFilename.lastIndexOf(".")) :
             snaplogFilename.substring(snaplogFilename.lastIndexOf("."))) + ".xpl"};
        try {
            Runtime.getRuntime().exec(cmdarray);
            Thread.sleep(1000);
            Runtime.getRuntime().exec(cmdarray2);
        }
        catch (InterruptedException e) {
            System.out.println(e);
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }

    protected void plotCputime(String cputraceFilename) {
        if (cputraceFilename == null) {
            return;
        }
        String fileName = cputraceFilename.startsWith("/") ? cputraceFilename :
                snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + cputraceFilename :
                snaplogFrame.getSnaplogs() + "/" +  cputraceFilename;
        JFileChooser chooser = new JFileChooser(new File(snaplogFrame.getSnaplogs()));
        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader(fileName));
        }
        catch (FileNotFoundException exc) {
            int returnVal = chooser.showOpenDialog(JAnalyze.this);

            if (returnVal == JFileChooser.APPROVE_OPTION) {
                File file = chooser.getSelectedFile();
                try {
                    br = new BufferedReader(new FileReader(file.getAbsolutePath()));
                }
                catch (Exception ex) {
                    System.out.println("Loading of the cputime file: " + file.getAbsolutePath() + " failed!");
                    ex.printStackTrace();
                    return;
                }
            }
            else {
                return;
            }
        }
        String line;
        Vector<Double> time = new Vector<Double>();
        Vector<Double> userTime = new Vector<Double>();
        Vector<Double> systemTime = new Vector<Double>();
        Vector<Double> cuserTime = new Vector<Double>();
        Vector<Double> csystemTime = new Vector<Double>();
        try {
            while ((line = br.readLine()) != null) {
                StringTokenizer st = new StringTokenizer(line.trim(), " ");
                time.add(Double.parseDouble(st.nextToken()));
                userTime.add(Double.parseDouble(st.nextToken()));
                systemTime.add(Double.parseDouble(st.nextToken()));
                cuserTime.add(Double.parseDouble(st.nextToken()));
                csystemTime.add(Double.parseDouble(st.nextToken()));
            }
            br.close();
        }
        catch (IOException exc) {
            exc.printStackTrace();
            return;
        }
        double[][] model = new double[4][time.size()];
        double[] columns = new double[time.size()];
        for (int i = 0; i < time.size(); ++i) {
            model[0][i] = userTime.elementAt(i);
            model[1][i] = systemTime.elementAt(i);
            model[2][i] = cuserTime.elementAt(i);
            model[3][i] = csystemTime.elementAt(i);
            columns[i] = time.elementAt(i);
        }
        String[] rows = { "user time", "system time",
            "user time of dead children", "system time of dead children" };
        String title = "Cputime usage: " + cputraceFilename;
        EditableChartDataModel data = new EditableChartDataModel(model, columns, rows);
        ChartPanel panel = new ChartPanel(data, title, DefaultChart.LINEAR_X_LINEAR_Y);
        panel.addChartRenderer(new LineChartRenderer(panel.getCoordSystem(), data), 1);

        JFrame frame = new JFrame(title);
        frame.getContentPane().add(panel, BorderLayout.CENTER);
        frame.setSize(800, 600);
        frame.setVisible(true);
    }


    void rebuildResultsList() {
        listPanel.removeAll();
        infoPanel.removeAll();
        fillListPanel();
    }

    private void fillListPanel() {    	
    	final JList list;
        list = new JList(new ResultsList(filterFrame.getResults()));
    	list.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    	list.addMouseListener(new
    	        MouseAdapter() {

    	          public void mousePressed(MouseEvent e) {
    	            int index = list.locationToIndex(e.getPoint());
    	            list.setSelectedIndex(index);    	            
    	          }

    	          public void mouseReleased(MouseEvent e) {
    	        	  int index = list.locationToIndex(e.getPoint());
      	            list.setSelectedIndex(index);
    	          }
    	        });
    	list.addListSelectionListener(new
    			ListSelectionListener() {

    		public void valueChanged(ListSelectionEvent arg0) {
    			showResultInfo((ResultsContainer) list.getSelectedValue()); 
    		}

    		private void showResultInfo(ResultsContainer result) {
    			infoPanel.removeAll();
    			infoPanel.add(result.getInfoPane());
    			infoPanel.revalidate();
    		}
    	});
    	JPanel tmpPanel = new JPanel();
    	tmpPanel.add(new JLabel("Shown results: " + list.getModel().getSize() + "/" + results.size()));
    	listPanel.add(tmpPanel, BorderLayout.NORTH);
    	listPanel.add(new JScrollPane(list));
    	listPanel.revalidate();
    	listPanel.repaint();
    	infoPanel.revalidate();
    	infoPanel.repaint();
	}

	public static void main(String args[])
    {
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception ex) { ex.printStackTrace(); }
        new JAnalyze();
    }

	public void stopLoading() {
		stopLoading = true;
	}

    public Properties getProperties() {
        return properties;
    }
}
