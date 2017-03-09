package gov.epa.surrogate;

public class SystemInfo {
	
	public void print(){//temp_change
		System.out.println("User Name: " + System.getProperty("user.name"));
		System.out.println("Operating System :  " + System.getProperty("os.name").toLowerCase());
	}

}
