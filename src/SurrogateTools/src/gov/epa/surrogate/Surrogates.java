package gov.epa.surrogate;

import java.util.Hashtable;
import java.util.Map;

public class Surrogates {

	private Map<Integer, String> idNameMap;

	private Map<String, Integer> nameIDMap;

	private String region;

	public Surrogates(String region) {
		this.region = region;
		idNameMap = new Hashtable<Integer, String>();
		nameIDMap = new Hashtable<String, Integer>();
	}

	public void addSurrogate(int surrogateCode, String surrogateName) {
		String name = idNameMap.get(new Integer(surrogateCode));
		if (name == null) {
			Integer code = new Integer(surrogateCode);
			idNameMap.put(code, surrogateName);
			nameIDMap.put(surrogateName, code);
		}
	}

	public String getSurrogateName(int surrogateCode) throws Exception {
		String name = idNameMap.get(new Integer(surrogateCode));
		if (name == null) {
			throw new Exception("Could not find a name for surrogate id '" + surrogateCode + "'");
		}
		return name;
	}

	public int getSurrogateID(String surrogateName) throws Exception {
		Integer code = nameIDMap.get(surrogateName);
		if (code == null) {
			throw new Exception("Could not find an id for the surrogate named '" + surrogateName + "'");
		}
		return code.intValue();
	}

	public String getRegion() {
		return region;
	}
}