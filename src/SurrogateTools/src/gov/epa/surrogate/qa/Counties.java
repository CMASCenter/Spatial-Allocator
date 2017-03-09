package gov.epa.surrogate.qa;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

public class Counties {

	private Map<Integer, County> counties;

	private List<Integer> surrogatesList;

	public Counties() {
		this.counties = new Hashtable<Integer, County>();
		this.surrogatesList = new ArrayList<Integer>();
	}

	public void add(SurrogateRow row, boolean isBig) {
		int countyCode = row.getCountyCode();
		County county = countyCode(countyCode);
		county.addRow(row, isBig);

		addToSurrogateList(row.getSurrogateCode());
	}

	private County countyCode(int countyCode) {
		Integer code = new Integer(countyCode);
		County county = counties.get(code);
		if (county == null) {
			county = new County(countyCode);
			counties.put(code, county);
		}
		return county;
	}

	public int[] allCountyCodes() {
		ArrayList<Integer> list = new ArrayList<Integer>(counties.keySet());
		Collections.sort(list);
		int[] codes = new int[list.size()];
		for (int i = 0; i < list.size(); i++) {
			codes[i] = list.get(i).intValue();
		}
		return codes;
	}

	public County getCounty(int countyCode) {
		County county = counties.get(new Integer(countyCode));
		if (county == null) {
			throw new IllegalArgumentException("The county code '" + countyCode + "' is not in the list of counties");
		}
		return county;

	}

	public int[] allSurrgateCodes() {
		Collections.sort(surrogatesList);

		int[] codes = new int[surrogatesList.size()];
		for (int i = 0; i < surrogatesList.size(); i++) {
			codes[i] = surrogatesList.get(i).intValue();
		}
		return codes;
	}


	private void addToSurrogateList(int surrogateCode) {
		Integer code = new Integer(surrogateCode);
		if (!surrogatesList.contains(code)) {
			surrogatesList.add(code);
		}
	}
	

	public boolean isEmpty() {
		return counties.isEmpty();
	}

}
