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
import java.io.IOException;
import java.util.Collection;
import java.util.Vector;

public class JAnalyze extends JFrame
{
	private static final long serialVersionUID = 1L;
	
	JMenuBar menuBar = new JMenuBar();
    JMenu fileMenu = new JMenu("File");
    JMenu optionsMenu = new JMenu("Options");
    JMenuItem exitMenuItem = new JMenuItem("Exit");
    JMenuItem loadMenuItem = new JMenuItem("Load");
    JMenuItem filterMenuItem = new JMenuItem("Filter");
    final JFileChooser fc = new JFileChooser(new File("/usr/local/ndt/"));

    FilterFrame filterFrame = null;
    
    JPanel listPanel = new JPanel();
    JPanel infoPanel = new JPanel();
    
    private boolean stopLoading;
    
    private Collection<ResultsContainer> results = new Vector<ResultsContainer>();

    public JAnalyze()
    {
        // Title
        setTitle("JAnalyze v0.3");

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
        menuBar.add(optionsMenu);

        setJMenuBar(menuBar);
        
        Container cp = getContentPane();
        cp.setLayout(new BorderLayout());
        listPanel.setLayout(new BorderLayout());
        cp.add(listPanel, BorderLayout.WEST);
        infoPanel.setLayout(new BorderLayout());
        cp.add(infoPanel);

        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setSize(900, 700);
        setVisible(true);
    }

    protected void loadWeb100srvLog(String filename, LoadingFrame frame) throws FileNotFoundException, IOException
    {
    	stopLoading = false;
//        System.out.println("Loading file: " + filename);
        BufferedReader br = new BufferedReader(new FileReader(filename));
        String line;
        ResultsContainer result = new ResultsContainer();
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
            	result = new ResultsContainer();
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
