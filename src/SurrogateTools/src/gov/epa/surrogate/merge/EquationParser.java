package gov.epa.surrogate.merge;

import gov.epa.surrogate.SurrogateFileInfo;

import java.util.ArrayList;
import java.util.List;

public class EquationParser {

	private String equationString;

	private String line;

	private Equation equation;

	public EquationParser(String equation, String line) {
		this.equationString = equation;
		this.line = line;
	}

	public List<SurrogateFileInfo> parse() throws Exception {
		List<SurrogateFileInfo> infos = new ArrayList<SurrogateFileInfo>();
		if (equationString.contains("+")) {
			twoInfos(infos);
		} else {
			oneInfo(infos);
		}
		return infos;
	}

	private void oneInfo(List<SurrogateFileInfo> infos) throws Exception {
		double factor1 = 0.0;
		String[] parts = equationString.split("\\*");
		factor1 = Double.parseDouble(parts[0]);
		infos.add(new SurrogateFileInfo(strip(parts[1].trim()), line));
		equation = new Equation(factor1);
	}

	private void twoInfos(List<SurrogateFileInfo> infos) throws Exception {
		double factor1 = 0.0;
		double factor2 = 0.0;
		String[] tokens = equationString.split("\\+");
		String[] parts = tokens[0].split("\\*");
		factor1 = Double.parseDouble(parts[0].trim());
		infos.add(new SurrogateFileInfo(strip(parts[1].trim()), line));
	
		parts = tokens[1].split("\\*");
		factor2 = Double.parseDouble(parts[0].trim());
		infos.add(new SurrogateFileInfo(strip(parts[1].trim()), line));
		equation = new Equation(factor1, factor2);
	}

	private String strip(String infoString) throws Exception {
		if (infoString.startsWith("({") && infoString.endsWith("})")) {
			return infoString.substring(2, infoString.length() - 2).trim();
		}
		throw new Exception("Each part of the equation enclosed by '({' and '})'\n" + line);
	}

	public Equation getEquation() {
		return equation;
	}
}
