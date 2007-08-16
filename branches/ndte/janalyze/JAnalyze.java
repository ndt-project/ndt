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

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

import java.io.File;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.Collection;
import java.util.Vector;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.Statement;
import java.sql.ResultSet;
import java.sql.SQLException;

public class JAnalyze extends JFrame
{
	private static final long serialVersionUID = 1L;
	
	JMenuBar menuBar = new JMenuBar();
    JMenu fileMenu = new JMenu("File");
    JMenu optionsMenu = new JMenu("Options");
    JMenuItem exitMenuItem = new JMenuItem("Exit");
    JMenuItem loadMenuItem = new JMenuItem("Load");
    JMenuItem loadDBMenuItem = new JMenuItem("Load from DB");
    JMenuItem filterMenuItem = new JMenuItem("Filter");
    JMenuItem snaplogMenuItem = new JMenuItem("Snaplogs");
    JMenuItem dbConfMenuItem = new JMenuItem("DB conf");
    final JFileChooser fc = new JFileChooser(new File("/usr/local/ndt/"));

    FilterFrame filterFrame = null;
    DBConfFrame dbConfFrame = new DBConfFrame(this);
    SnaplogFrame snaplogFrame = new SnaplogFrame(this);
    
    JPanel listPanel = new JPanel();
    JPanel infoPanel = new JPanel();
    
    private boolean stopLoading;
    
    private Collection<ResultsContainer> results = new Vector<ResultsContainer>();

    public JAnalyze()
    {
        // Title
        setTitle("JAnalyze v0.6");

        // Menu        
        loadMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int returnVal = fc.showOpenDialog(JAnalyze.this);

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
            public void actionPerformed(ActionEvent e) {
                System.exit(0);
            }
        });
        fileMenu.add(exitMenuItem);
        menuBar.add(fileMenu);

        filterMenuItem.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if (filterFrame == null) {
                    filterFrame = new FilterFrame(JAnalyze.this, results);
                }
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
        if (filterFrame != null) {
            filterFrame.resultsChange();
        }
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
        if (filterFrame != null) {
            filterFrame.resultsChange();
        }
        rebuildResultsList();
    }

    protected String getSnaplogData(String snaplogFilename, String variables, int numOfLines, boolean trim) {
        String[] cmdarray = new String[] {snaplogFrame.getGenplot(), "-tm", variables == null ?
            snaplogFrame.getVariables() : variables,
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
        String[] cmdarray = new String[] {snaplogFrame.getTcptrace(), "-l",
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
        String[] cmdarray = new String[] {snaplogFrame.getTcptrace(), "-S",
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
            // do nothing
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }


    protected void plotSnaplog(String snaplogFilename) {
        String[] cmdarray = new String[] {snaplogFrame.getGenplot(), "-m", snaplogFrame.getVariables(),
            snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + snaplogFilename :
                snaplogFrame.getSnaplogs() + "/" +  snaplogFilename};
        String[] cmdarray2 = new String[] {snaplogFrame.getXplot(),
        "User Defined" + snaplogFilename.substring(snaplogFilename.lastIndexOf(".")) + ".xpl"};
        try {
            Runtime.getRuntime().exec(cmdarray);
            Thread.sleep(1000);
            Runtime.getRuntime().exec(cmdarray2);
        }
        catch (InterruptedException e) {
            // do nothing
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }

    protected void plotSnaplogCWND(String snaplogFilename) {
        String[] cmdarray = new String[] {snaplogFrame.getGenplot(), "-C",
            snaplogFrame.getSnaplogs().endsWith("/") ?
                snaplogFrame.getSnaplogs() + snaplogFilename :
                snaplogFrame.getSnaplogs() + "/" +  snaplogFilename};
        String[] cmdarray2 = new String[] {snaplogFrame.getXplot(),
        "CurCwnd" + snaplogFilename.substring(snaplogFilename.lastIndexOf(".")) + ".xpl"};
        try {
            Runtime.getRuntime().exec(cmdarray);
            Thread.sleep(1000);
            Runtime.getRuntime().exec(cmdarray2);
        }
        catch (InterruptedException e) {
            // do nothing
        }
        catch (IOException e) {
            System.out.println(e);
        }
    }


    void rebuildResultsList() {
        listPanel.removeAll();
        infoPanel.removeAll();
        fillListPanel();
    }

    private void fillListPanel() {    	
    	final JList list;
        if (filterFrame != null) {
            list = new JList(new ResultsList(filterFrame.getResults()));
        }
        else {
            list = new JList(new ResultsList(results));
        }
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
}
