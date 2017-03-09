package gov.epa.surrogate.normalize;

import gov.epa.surrogate.Precision;

import java.util.ArrayList;
import java.util.List;

public class County {

	private int countyID;

	private List<NormalizeSurrogateRow> surrogateRows;

	private Precision precision;

	public County(int countyID, Precision precision) {
		this.countyID = countyID;
		surrogateRows = new ArrayList<NormalizeSurrogateRow>();
		this.precision = precision;
	}

	public int getCountyID() {
		return countyID;
	}

	public NormalizeSurrogateRow[] surrogateRows() {
		return surrogateRows.toArray(new NormalizeSurrogateRow[0]);
	}

	public void addRow(NormalizeSurrogateRow row) {
		surrogateRows.add(row);
	}

	public boolean isTotalOne() {
		return precision.isTotalOne(sum());
	}

	public void normalize() {
		if (!isTotalOne()) {
			normalize(sum(), surrogateRows);
		}
	}

	private void normalize(double sum, List surrogateRows) {
		for (int i = 0; i < surrogateRows.size(); i++) {
			NormalizeSurrogateRow row = (NormalizeSurrogateRow) surrogateRows.get(i);
			double normalizedValue = row.getRatio() / sum;
			row.setRatio(normalizedValue);
		}
	}

	private double sum() {
		double sum = 0.0;
		for (int i = 0; i < surrogateRows.size(); i++) {
			sum += surrogateRows.get(i).getRatio();
		}
		return sum;
	}

}
