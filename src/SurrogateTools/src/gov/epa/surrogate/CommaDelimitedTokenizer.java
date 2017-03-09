package gov.epa.surrogate;

public class CommaDelimitedTokenizer implements Tokenizer {

    private DelimitedTokenizer delegate;

    private String pattern;

    public CommaDelimitedTokenizer() {
        pattern = "[^,]+";
        delegate = new DelimitedTokenizer(pattern);
        
    }

    public String[] tokens(String input) {
        return delegate.doTokenize(input);
    }

}
