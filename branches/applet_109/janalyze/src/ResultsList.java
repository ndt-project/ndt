import java.util.Collection;
import java.util.Vector;

import javax.swing.AbstractListModel;


public class ResultsList extends AbstractListModel {

	private static final long serialVersionUID = 1L;
	private Vector<ResultsContainer> results = new Vector<ResultsContainer>();
	
	public ResultsList(Collection<ResultsContainer> results) {
		this.results.addAll(results);
	}
	public Object getElementAt(int arg0) {
		return results.get(arg0);		
	}

	public int getSize() {
		return results.size();
	}

}
