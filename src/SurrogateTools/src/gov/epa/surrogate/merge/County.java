package gov.epa.surrogate.merge;

import java.util.ArrayList;
import java.util.List;

public class County {

	private int countyCode;

	// TODO: save it in a SortedSet
	private List<SurrogateRow> surrogates;

	public County(int countyCode) {
		this.countyCode = countyCode;
		surrogates = new ArrayList<SurrogateRow>();
	}

	public int getCountyCode() {
		return countyCode;
	}

	public void addRow(SurrogateRow row) {
		surrogates.add(row);
	}

	public List<SurrogateRow> getSurrogates() {
		return surrogates;
	}

}
