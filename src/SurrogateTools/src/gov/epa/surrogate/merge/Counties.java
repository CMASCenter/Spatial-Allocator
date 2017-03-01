package gov.epa.surrogate.merge;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

public class Counties {

	private Map<Integer, County> counties;

	private List<Integer> allCounitesList;

	public Counties() {
		this.counties = new Hashtable<Integer, County>();
		this.allCounitesList = new ArrayList<Integer>();
	}

	public void add(SurrogateRow row) {
		int countyCode = row.getCountyCode();
		County county = countyCode(countyCode);
		county.addRow(row);
	}
	
	public void add(County county){
		int countyCode = county.getCountyCode();
		counties.put(countyCode,county);
		Integer code = new Integer(countyCode);
		if(!allCounitesList.contains(code)){
			allCounitesList.add(code);
		}
	}

	private County countyCode(int countyCode) {
		Integer code = new Integer(countyCode);
		County county = counties.get(code);
		if (county == null) {
			county = new County(countyCode);
			counties.put(code, county);
			allCounitesList.add(code);
		}
		return county;
	}

	//TODO: save county codes in a sorted order
	public List<Integer> allCountyCodes() {
		return allCounitesList;
	}

	public County getCounty(int countyCode) throws Exception {
		County county = counties.get(new Integer(countyCode));
		if (county == null) {
			throw new Exception("The county code '" + countyCode + "' is not in the list of counties");
		}
		return county;

	}

	public boolean isEmpty() {
		return counties.isEmpty();
	}

}
