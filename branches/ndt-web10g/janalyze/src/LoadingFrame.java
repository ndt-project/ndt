import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
//import javax.swing.JProgressBar;
import javax.swing.WindowConstants;

public class LoadingFrame extends JFrame {

	private static final long serialVersionUID = 1L;

	private JButton cancelButton = new JButton("Cancel");	
//	private JProgressBar progressBar;	
	private int loaded = 0;
	private JLabel loadedLabel = new JLabel("Loaded results: " + loaded);

	public LoadingFrame(final String filename, final JAnalyze janalyze) {
		super("Loading...");

		final SwingWorker worker = new SwingWorker() {
			public Object construct() {
				try {
					janalyze.loadWeb100srvLog(filename, LoadingFrame.this);
				}
                catch (Exception exc) {
                    System.out.println("Loading of the web100srv.log failed!");
                    exc.printStackTrace();
                }
				return null;
			}
			public void finished() {
				LoadingFrame.this.setVisible(false);
				LoadingFrame.this.dispose();
			}
		};
		
//		progressBar = new JProgressBar();
//		progressBar.setStringPainted(true);
		Container cp = getContentPane();
	    cp.setLayout(new BoxLayout(cp, BoxLayout.PAGE_AXIS)) ;
	    cp.add(loadedLabel);
//	    cp.add(progressBar);
	    cp.add(cancelButton);

	    cancelButton.addActionListener(new ActionListener() {
	    	public void actionPerformed(ActionEvent e) {
	    		worker.interrupt();
	    		janalyze.stopLoading();
	    		LoadingFrame.this.setVisible(false);
	    		LoadingFrame.this.dispose();
	    	}
	    });

	    setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
	    worker.start();
	}
	
	public void loadedResult() {
		loaded++;
		loadedLabel.setText("Loaded results: " + loaded);
	}
}
