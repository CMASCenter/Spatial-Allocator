package gov.epa.surrogate.qa;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

public class County {

	private int countyCode;
	
	private Map<Integer,CountySurrogate> surrogates;
	private List<Long> holdGridList;

	public County(int countyCode) {
		this.countyCode = countyCode;
		surrogates = new Hashtable<Integer,CountySurrogate>();
		this.holdGridList = new ArrayList<Long>();
	}

	public int getCountyCode(){
		return countyCode;
	}
	
	public void addRow(SurrogateRow row, boolean isBig){
		int surrogateCode = row.getSurrogateCode();
		Integer id = new Integer(surrogateCode);
		CountySurrogate srg = surrogates.get(id);
		if(srg == null){
			srg = new CountySurrogate(surrogateCode, row.getGapFillCode());
			surrogates.put(id,srg);
		}
		
		if ( isBig){
			long gridId = 0;
			if (row.getColumn() == -1 )
				gridId = row.getRow();
			else
				gridId = row.getRow()*1000 + row.getColumn();
			addToHoldGridList(gridId);
		}
		srg.addRatio(row, isBig);
	}

	public CountySurrogate surrogate(int code) {
		return surrogates.get(new Integer(code));
	}
	
	public long[] allHoldGrids() {
		Collections.sort(holdGridList);

		long[] codes = new long[holdGridList.size()];
		for (int i = 0; i < holdGridList.size(); i++) {
			codes[i] = holdGridList.get(i).longValue();
		}
		return codes;
	}
	
	private void addToHoldGridList(long gridId) {
		Long code = new Long(gridId);
		if (!holdGridList.contains(code)) {
			holdGridList.add(code);
		}
	}

}
