package gov.epa.surrogate.gapfill;

import java.util.ArrayList;
import java.util.List;

public class County {

	private int countyID;

	private int surrogateID;

	private List<String> rows;

	public County(int countyID, int surrogateID) {
		this.countyID = countyID;
		this.surrogateID = surrogateID;
		this.rows = new ArrayList<String>();
	}

	public void addRow(String line) {
		rows.add(line);
	}

	public int getCountyID() {
		return countyID;
	}

	public int getSurrogateID() {
		return surrogateID;
	}

	public String[] getRows() {
		return rows.toArray(new String[0]);
	}

}
