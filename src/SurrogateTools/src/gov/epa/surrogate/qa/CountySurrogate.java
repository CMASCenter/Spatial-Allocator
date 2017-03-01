package gov.epa.surrogate.qa;

import gov.epa.surrogate.qa.SurrogateRow;

import java.util.ArrayList;
import java.util.List;

//represnts a cell in the county surrogate table
public class CountySurrogate {

	private int surrogateID;

	private double ratio;

	private int gapFillCode;
	
	private List<SurrogateRow> holdSurrogates;

	public CountySurrogate(int surrogateID, int gapFillCode) {
		this.surrogateID = surrogateID;
		this.gapFillCode = gapFillCode;
		holdSurrogates = new ArrayList<SurrogateRow>();
		this.ratio = 0.0;
	}

	public int getSurrogateID() {
		return surrogateID;
	}

	public void addRatio(SurrogateRow row, boolean isBig) {
		if (isBig)    holdSurrogates.add(row);
		this.ratio += row.getRatio();
	}

	public double getSum() {
		return ratio;
	}

	public boolean isGapFillted() {
		return !(gapFillCode == -1);
	}

	public int gapFillCode() {
		return gapFillCode;
	}
	
	public List<SurrogateRow> getHoldRows(){
		return holdSurrogates;
	}
}
