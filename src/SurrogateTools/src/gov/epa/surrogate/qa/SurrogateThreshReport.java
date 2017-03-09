package gov.epa.surrogate.qa;

import gov.epa.surrogate.Surrogates;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.List;

public class SurrogateThreshReport {

	private Counties counties;

	private String delimiter;

	private String fileName;

	private SrgSummary delegate;
	
	private int minTokens;

	public SurrogateThreshReport(String fileName, Counties counties, String header, 
			Surrogates surrogates, int minTokens, Threshold tt)
			throws Exception {
		this.fileName = fileName;
		this.delimiter = ",";
		this.counties = counties;
		this.minTokens = minTokens;
		PrintWriter writer = new PrintWriter(new FileWriter(new File(fileName)));
		this.delegate = new SrgSummary(writer, delimiter, counties, header, surrogates, tt);
	}

	public void write() throws Exception {
		System.out.println("The output surrogate threshold file: " + fileName);

		int[] countyCodes = counties.allCountyCodes();
		int[] surrogateCodes = counties.allSurrgateCodes();
		int noOfSurrogates = surrogateCodes.length;
		delegate.printHeader();
		delegate.printHoldColumnHeader(surrogateCodes, minTokens);
		for (int i = 0; i < countyCodes.length; i++) {
			County county = counties.getCounty(countyCodes[i]);
			long[] holdGrids = county.allHoldGrids();
			//System.out.println("value: " + atLeastOneBigInARow(county, surrogateCodes));
			if (holdGrids !=null && holdGrids.length > 0) {
				for ( int k = 0; k < holdGrids.length; k++){
					delegate.printValueWithDelimiter(county.getCountyCode());
                    long row = -1;
                    long column =-1; 
                    if (minTokens == 5 ) {
                    	column=holdGrids[k]%1000;
                    	delegate.printValueWithDelimiter(column);
                    	row=(holdGrids[k]-column)/1000;
                    }
                    else
                    	row = holdGrids[k];
                    delegate.printValueWithDelimiter(row);
                    for (int j = 0; j < noOfSurrogates; j++) {
						CountySurrogate surrogate = county.surrogate(surrogateCodes[j]);
						if ( j == noOfSurrogates - 1)
							delegate.printValueWithLineSeparator(value(surrogate,row, column));
						else
							delegate.printValueWithDelimiter(value(surrogate, row, column));
					}
				}
			}
		}
		delegate.close();
		System.out.println("Finished writting Threshold file.");
	}

	private String value(CountySurrogate surrogate, long row, long col) {
		if (surrogate == null) {
			return "";
		}

		StringBuffer sb = new StringBuffer();
		List<SurrogateRow> holdRows = surrogate.getHoldRows();
		SurrogateRow findHoldRow = null;
		for ( int i = 0; i < holdRows.size(); i++){
			SurrogateRow holdRow = holdRows.get(i);
			if ( holdRow.getRow() == row && holdRow.getColumn() == col ){
				findHoldRow = holdRow;
				break;
			}
		}
		
		if ( findHoldRow != null ){
			double value = findHoldRow.getRatio(); 
			sb.append(delegate.format(value));
		}

		return sb.toString();
	}

}
