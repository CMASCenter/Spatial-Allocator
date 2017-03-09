package gov.epa.surrogate.gapfill;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

public class Counties {

	private Map<Integer, County> counties;

	public Counties() {
		this.counties = new Hashtable<Integer,County>();
	}

	public void add(int countyCode, County county) {
		counties.put(new Integer(countyCode), county);
	}

	public boolean exist(int countyCode) {
		return counties.containsKey(new Integer(countyCode));
	}
	
	public County[] getCounties(){
		Set<Integer> ids = counties.keySet();
		List<Integer> idsList = new ArrayList<Integer>(new TreeSet<Integer>(ids));
		County[] allCounty = new County[ids.size()]; 
		for (int i = 0; i < idsList.size(); i++) {
			allCounty[i] = counties.get(idsList.get(i));
		}
		return allCounty;
	}
}
